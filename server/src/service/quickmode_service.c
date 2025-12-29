// server/src/service/quickmode_service.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include "dao/dao_question.h"
#include "dao/dao_stats.h"
#include "service/quickmode_service.h"
#include "service/commands.h"
#include "service/protocol.h"
#include "service/client_session.h"
#include "utils/json.h"

// Quickmode không lưu vào DB, chỉ lưu trong memory
// Chơi xong là xong, không update stats

// Static linked list để quản lý active sessions
static QuickModeSession *active_sessions = NULL;

// ============================================================
// SESSION MANAGEMENT
// ============================================================

static QuickModeSession *quickmode_session_new(int64_t user_id, ClientSession *client_sess) {
	if (!client_sess || user_id <= 0) return NULL;

	QuickModeSession *sess = calloc(1, sizeof(QuickModeSession));
	if (!sess) return NULL;

	sess->user_id = user_id;
	sess->client_session = client_sess;
	sess->session_id = (int64_t)time(NULL) * 1000 + (rand() % 1000); // Simple ID generation
	sess->current_round = 0;
	sess->score = 0;
	sess->start_time = time(NULL);
	sess->game_status = 0; // playing
	sess->lifeline_5050_used = 0;
	sess->lifeline_5050_rounds[0] = 0;
	sess->lifeline_5050_rounds[1] = 0;

	// Initialize arrays
	for (int i = 0; i < 15; i++) {
		sess->answers[i] = '\0';
		sess->correct[i] = 0;
	}

	// Add to linked list
	sess->next = active_sessions;
	active_sessions = sess;

	return sess;
}

static void quickmode_session_free(QuickModeSession *sess) {
	if (!sess) return;

	// Remove from linked list
	if (active_sessions == sess) {
		active_sessions = sess->next;
	} else {
		QuickModeSession *prev = active_sessions;
		while (prev && prev->next != sess) {
			prev = prev->next;
		}
		if (prev) {
			prev->next = sess->next;
		}
	}

	free(sess);
}

static QuickModeSession *quickmode_session_find(int64_t user_id) {
	QuickModeSession *sess = active_sessions;
	while (sess) {
		if (sess->user_id == user_id) return sess;
		sess = sess->next;
	}
	return NULL;
}

static QuickModeSession *quickmode_session_find_by_id(int64_t session_id) {
	QuickModeSession *sess = active_sessions;
	while (sess) {
		if (sess->session_id == session_id) return sess;
		sess = sess->next;
	}
	return NULL;
}

// ============================================================
// QUESTION LOADING
// ============================================================

// Helper function to check if question_id is already in the loaded_ids array
static int is_question_duplicate(int64_t question_id, const int64_t *loaded_ids, int loaded_count) {
	for (int j = 0; j < loaded_count; j++) {
		if (loaded_ids[j] == question_id) {
			return 1;
		}
	}
	return 0;
}

// Helper function to validate question data
static int is_question_valid(const Question *q) {
	if (!q) return 0;
	
	// Check question_id
	if (q->question_id <= 0) {
		fprintf(stderr, "[QM] Invalid question_id: %lld\n", q->question_id);
		return 0;
	}
	
	// Check content
	if (!q->content || strlen(q->content) == 0) {
		fprintf(stderr, "[QM] Empty question content for question_id %lld\n", q->question_id);
		return 0;
	}
	
	// Check all options
	if (!q->op_a || strlen(q->op_a) == 0 ||
	    !q->op_b || strlen(q->op_b) == 0 ||
	    !q->op_c || strlen(q->op_c) == 0 ||
	    !q->op_d || strlen(q->op_d) == 0) {
		fprintf(stderr, "[QM] Empty option(s) for question_id %lld\n", q->question_id);
		return 0;
	}
	
	// Check correct_op
	if (!q->correct_op || (q->correct_op[0] != 'A' && q->correct_op[0] != 'B' && 
	                       q->correct_op[0] != 'C' && q->correct_op[0] != 'D' &&
	                       q->correct_op[0] != 'a' && q->correct_op[0] != 'b' &&
	                       q->correct_op[0] != 'c' && q->correct_op[0] != 'd')) {
		fprintf(stderr, "[QM] Invalid correct_op: %c (0x%02x) for question_id %lld\n",
			q->correct_op[0], (unsigned char)q->correct_op[0], q->question_id);
		return 0;
	}
	
	return 1;
}

static int quickmode_load_questions(QuickModeSession *sess) {
	if (!sess) return -1;

	// Array to track loaded question IDs to prevent duplicates
	int64_t loaded_ids[15];
	int loaded_count = 0;

	// Load 5 EASY questions
	for (int i = 0; i < 5; i++) {
		int retries = 0;
		const int MAX_RETRIES = 100;
		int success = 0;

		while (retries < MAX_RETRIES) {
			if (dao_question_get_random("EASY", &sess->questions[i]) != 0) {
				fprintf(stderr, "[QM] Failed to load EASY question %d\n", i);
				return -1;
			}

			// Validate question data
			if (!is_question_valid(&sess->questions[i])) {
				fprintf(stderr, "[QM] Invalid EASY question %d data, retrying...\n", i);
				retries++;
				continue;
			}

			// Check for duplicate
			if (!is_question_duplicate(sess->questions[i].question_id, loaded_ids, loaded_count)) {
				loaded_ids[loaded_count++] = sess->questions[i].question_id;
				success = 1;
				break;
			}

			retries++;
		}

		if (!success) {
			fprintf(stderr, "[QM] Failed to load unique EASY question %d after %d retries\n", i, MAX_RETRIES);
			return -1;
		}
	}

	// Load 5 MEDIUM questions
	for (int i = 0; i < 5; i++) {
		int retries = 0;
		const int MAX_RETRIES = 100;
		int success = 0;

		while (retries < MAX_RETRIES) {
			if (dao_question_get_random("MEDIUM", &sess->questions[i + 5]) != 0) {
				fprintf(stderr, "[QM] Failed to load MEDIUM question %d\n", i);
				return -1;
			}

			// Validate question data
			if (!is_question_valid(&sess->questions[i + 5])) {
				fprintf(stderr, "[QM] Invalid MEDIUM question %d data, retrying...\n", i);
				retries++;
				continue;
			}

			// Check for duplicate
			if (!is_question_duplicate(sess->questions[i + 5].question_id, loaded_ids, loaded_count)) {
				loaded_ids[loaded_count++] = sess->questions[i + 5].question_id;
				success = 1;
				break;
			}

			retries++;
		}

		if (!success) {
			fprintf(stderr, "[QM] Failed to load unique MEDIUM question %d after %d retries\n", i, MAX_RETRIES);
			return -1;
		}
	}

	// Load 5 HARD questions
	for (int i = 0; i < 5; i++) {
		int retries = 0;
		const int MAX_RETRIES = 100;
		int success = 0;

		while (retries < MAX_RETRIES) {
			if (dao_question_get_random("HARD", &sess->questions[i + 10]) != 0) {
				fprintf(stderr, "[QM] Failed to load HARD question %d\n", i);
				return -1;
			}

			// Validate question data
			if (!is_question_valid(&sess->questions[i + 10])) {
				fprintf(stderr, "[QM] Invalid HARD question %d data, retrying...\n", i);
				retries++;
				continue;
			}

			// Check for duplicate
			if (!is_question_duplicate(sess->questions[i + 10].question_id, loaded_ids, loaded_count)) {
				loaded_ids[loaded_count++] = sess->questions[i + 10].question_id;
				success = 1;
				break;
			}

			retries++;
		}

		if (!success) {
			fprintf(stderr, "[QM] Failed to load unique HARD question %d after %d retries\n", i, MAX_RETRIES);
			return -1;
		}
	}

	return 0;
}

// ============================================================
// HANDLERS
// ============================================================

// Forward declarations
static void quickmode_handle_game_over(QuickModeSession *sess);

static void quickmode_handle_start(ClientSession *sess, const char *payload) {
	(void)payload; // Payload có thể empty

	// Validate user đã login
	if (!sess || sess->user_id <= 0) {
		protocol_send_error(sess, CMD_NOTIFY_GAME_START, "USER_NOT_LOGGED_IN");
		return;
	}

	// Check user chưa có active session
	QuickModeSession *existing = quickmode_session_find(sess->user_id);
	if (existing) {
		protocol_send_error(sess, CMD_NOTIFY_GAME_START, "SESSION_ALREADY_EXISTS");
		return;
	}

	// Create new session
	QuickModeSession *qm_sess = quickmode_session_new(sess->user_id, sess);
	if (!qm_sess) {
		protocol_send_error(sess, CMD_NOTIFY_GAME_START, "SESSION_CREATE_FAILED");
		return;
	}

	// Load 15 questions
	if (quickmode_load_questions(qm_sess) != 0) {
		quickmode_session_free(qm_sess);
		protocol_send_error(sess, CMD_NOTIFY_GAME_START, "LOAD_QUESTIONS_FAILED");
		return;
	}

	// Send game start notification
	char json[256];
	snprintf(json, sizeof(json),
		"{\"session_id\": %lld, \"total_rounds\": 15, \"time_per_question\": 15}",
		(long long)qm_sess->session_id);

	protocol_send_response(sess, CMD_NOTIFY_GAME_START, json, strlen(json));
}

static void quickmode_handle_get_question(ClientSession *sess, const char *payload) {
	if (!sess || !payload) {
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "INVALID_REQUEST");
		return;
	}

	// Parse session_id and round
	long long session_id = 0;
	int round = 0;
	long long round_ll = 0;
	util_json_get_int64(payload, "session_id", &session_id);
	util_json_get_int64(payload, "round", &round_ll);
	round = (int)round_ll;

	if (session_id == 0 || round < 1 || round > 15) {
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "INVALID_PARAMETERS");
		return;
	}

	// Find session
	QuickModeSession *qm_sess = quickmode_session_find_by_id(session_id);
	if (!qm_sess || qm_sess->user_id != sess->user_id) {
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "SESSION_NOT_FOUND");
		return;
	}

	// Validate round
	if (round > qm_sess->current_round + 1) {
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "INVALID_ROUND");
		return;
	}

	// Get question
	Question *q = &qm_sess->questions[round - 1];
	
	// Validate question data
	if (q->question_id <= 0) {
		fprintf(stderr, "[QM] Invalid question_id: %lld for round %d\n", q->question_id, round);
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "INVALID_QUESTION_DATA");
		return;
	}
	
	if (!q->content || strlen(q->content) == 0) {
		fprintf(stderr, "[QM] Empty question content for round %d, question_id %lld\n", round, q->question_id);
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "INVALID_QUESTION_DATA");
		return;
	}
	
	if (!q->op_a || strlen(q->op_a) == 0 ||
	    !q->op_b || strlen(q->op_b) == 0 ||
	    !q->op_c || strlen(q->op_c) == 0 ||
	    !q->op_d || strlen(q->op_d) == 0) {
		fprintf(stderr, "[QM] Empty option(s) for round %d, question_id %lld\n", round, q->question_id);
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "INVALID_QUESTION_DATA");
		return;
	}
	
	if (!q->correct_op || (q->correct_op[0] != 'A' && q->correct_op[0] != 'B' && 
	                       q->correct_op[0] != 'C' && q->correct_op[0] != 'D' &&
	                       q->correct_op[0] != 'a' && q->correct_op[0] != 'b' &&
	                       q->correct_op[0] != 'c' && q->correct_op[0] != 'd')) {
		fprintf(stderr, "[QM] Invalid correct_op: %c (0x%02x) for round %d, question_id %lld\n",
			q->correct_op[0], (unsigned char)q->correct_op[0], round, q->question_id);
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "INVALID_QUESTION_DATA");
		return;
	}

	// Determine difficulty
	const char *difficulty;
	if (round <= 5) difficulty = "EASY";
	else if (round <= 10) difficulty = "MEDIUM";
	else difficulty = "HARD";

	// Escape strings for JSON
	char *esc_content = util_json_escape(q->content);
	char *esc_a = util_json_escape(q->op_a);
	char *esc_b = util_json_escape(q->op_b);
	char *esc_c = util_json_escape(q->op_c);
	char *esc_d = util_json_escape(q->op_d);

	if (!esc_content || !esc_a || !esc_b || !esc_c || !esc_d) {
		if (esc_content) free(esc_content);
		if (esc_a) free(esc_a);
		if (esc_b) free(esc_b);
		if (esc_c) free(esc_c);
		if (esc_d) free(esc_d);
		protocol_send_error(sess, CMD_NOTIFY_QUESTION, "JSON_BUILD_FAILED");
		return;
	}

	// Build JSON
	char json[2048];
	snprintf(json, sizeof(json),
		"{"
		"\"session_id\": %lld, "
		"\"round\": %d, "
		"\"question_id\": %" PRId64 ", "
		"\"content\": \"%s\", "
		"\"options\": {"
		"\"A\": \"%s\", "
		"\"B\": \"%s\", "
		"\"C\": \"%s\", "
		"\"D\": \"%s\""
		"}, "
		"\"difficulty\": \"%s\", "
		"\"time_limit\": 15"
		"}",
		(long long)session_id, round, q->question_id,
		esc_content, esc_a, esc_b, esc_c, esc_d, difficulty);

	free(esc_content);
	free(esc_a);
	free(esc_b);
	free(esc_c);
	free(esc_d);

	protocol_send_response(sess, CMD_NOTIFY_QUESTION, json, strlen(json));
}

static void quickmode_handle_submit_answer(ClientSession *sess, const char *payload) {
	if (!sess || !payload) {
		protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER, "INVALID_REQUEST");
		return;
	}

	// Parse
	long long session_id = 0;
	int round = 0;
	long long round_ll = 0;
	char *answer_str = util_json_get_string(payload, "answer");

	util_json_get_int64(payload, "session_id", &session_id);
	util_json_get_int64(payload, "round", &round_ll);
	round = (int)round_ll;

	if (session_id == 0 || round < 1 || round > 15 || !answer_str || strlen(answer_str) != 1) {
		if (answer_str) free(answer_str);
		protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER, "INVALID_PARAMETERS");
		return;
	}

	char answer = answer_str[0];
	free(answer_str);

	if (answer != 'A' && answer != 'B' && answer != 'C' && answer != 'D') {
		protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER, "INVALID_ANSWER");
		return;
	}

	// Find session
	QuickModeSession *qm_sess = quickmode_session_find_by_id(session_id);
	if (!qm_sess || qm_sess->user_id != sess->user_id) {
		protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER, "SESSION_NOT_FOUND");
		return;
	}

	// Check round already answered
	if (qm_sess->answers[round - 1] != '\0') {
		protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER, "ROUND_ALREADY_ANSWERED");
		return;
	}

	// Get question
	Question *q = &qm_sess->questions[round - 1];
	int is_correct = (answer == q->correct_op[0]) ? 1 : 0;

	// Save answer
	qm_sess->answers[round - 1] = answer;
	qm_sess->correct[round - 1] = is_correct;

	if (is_correct) {
		qm_sess->score++;
	} else {
		qm_sess->game_status = 2; // lost
	}

	qm_sess->current_round = round;

	// Check win condition
	if (round == 15 && is_correct) {
		qm_sess->game_status = 1; // won
	}

	// Build response
	char json[512];
	int next_round = (round < 15 && is_correct) ? round + 1 : 0;
	int game_over = (qm_sess->game_status != 0) ? 1 : 0;

	snprintf(json, sizeof(json),
		"{"
		"\"session_id\": %lld, "
		"\"round\": %d, "
		"\"correct\": %s, "
		"\"correct_answer\": \"%c\", "
		"\"score\": %d, "
		"\"next_round\": %d, "
		"\"game_over\": %s"
		"}",
		(long long)session_id, round,
		is_correct ? "true" : "false",
		q->correct_op[0],
		qm_sess->score,
		next_round,
		game_over ? "true" : "false");

	protocol_send_response(sess, CMD_RES_SUBMIT_ANSWER, json, strlen(json));

	// Send answer result notification
	char json_result[256];
	snprintf(json_result, sizeof(json_result),
		"{\"round\": %d, \"correct\": %s, \"correct_answer\": \"%c\"}",
		round, is_correct ? "true" : "false", q->correct_op[0]);
	protocol_send_response(sess, CMD_NOTIFY_ANSWER_RESULT, json_result, strlen(json_result));

	// Handle game over
	if (game_over) {
		quickmode_handle_game_over(qm_sess);
	}
}

static void quickmode_handle_use_lifeline(ClientSession *sess, const char *payload) {
	if (!sess || !payload) {
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INVALID_REQUEST");
		return;
	}

	// Parse
	long long session_id = 0;
	int round = 0;
	long long round_ll = 0;
	util_json_get_int64(payload, "session_id", &session_id);
	util_json_get_int64(payload, "round", &round_ll);
	round = (int)round_ll;

	if (session_id == 0 || round < 1 || round > 15) {
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INVALID_PARAMETERS");
		return;
	}

	// Find session
	QuickModeSession *qm_sess = quickmode_session_find_by_id(session_id);
	if (!qm_sess || qm_sess->user_id != sess->user_id) {
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "SESSION_NOT_FOUND");
		return;
	}

	// Validate lifeline usage
	if (qm_sess->lifeline_5050_used >= 2) {
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "LIFELINE_EXHAUSTED");
		return;
	}

	// Check round already answered
	if (qm_sess->answers[round - 1] != '\0') {
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "ROUND_ALREADY_ANSWERED");
		return;
	}

	// Get question
	Question *q = &qm_sess->questions[round - 1];
	char correct_op = q->correct_op[0];
	
	// Normalize correct_op to uppercase and validate
	if (correct_op >= 'a' && correct_op <= 'd') {
		correct_op = correct_op - 'a' + 'A';
	}
	if (correct_op < 'A' || correct_op > 'D') {
		fprintf(stderr, "[QM] Invalid correct_op: %c (0x%02x) for round %d, question_id %lld\n", 
			correct_op, (unsigned char)correct_op, round, q->question_id);
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INVALID_QUESTION_DATA");
		return;
	}

	// Build array of wrong options
	char wrong_options[3];
	// Explicitly initialize each element to ensure no garbage values
	wrong_options[0] = '\0';
	wrong_options[1] = '\0';
	wrong_options[2] = '\0';
	
	int wrong_count = 0;
	for (char opt = 'A'; opt <= 'D'; opt++) {
		if (opt != correct_op) {
			if (wrong_count >= 3) {
				fprintf(stderr, "[QM] Error: wrong_count overflow for round %d\n", round);
				protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
				return;
			}
			wrong_options[wrong_count] = opt;
			wrong_count++;
		}
	}

	// Validate wrong_count
	if (wrong_count != 3) {
		fprintf(stderr, "[QM] Invalid wrong_count: %d (expected 3) for round %d, correct_op=%c (0x%02x)\n", 
			wrong_count, round, correct_op, (unsigned char)correct_op);
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
		return;
	}
	
	// Validate wrong_options array before use
	for (int i = 0; i < 3; i++) {
		if (wrong_options[i] < 'A' || wrong_options[i] > 'D') {
			fprintf(stderr, "[QM] Invalid wrong_options[%d]: %c (0x%02x) for round %d, correct_op=%c\n",
				i, wrong_options[i], (unsigned char)wrong_options[i], round, correct_op);
			protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
			return;
		}
	}

	// Random pick one wrong option to keep
	// Use time-based seed for better randomness
	time_t now = time(NULL);
	int random_idx = (int)(now + qm_sess->session_id) % wrong_count;
	
	// Ensure random_idx is positive (handle negative modulo)
	if (random_idx < 0) {
		random_idx = (-random_idx) % wrong_count;
	}
	
	if (random_idx < 0 || random_idx >= wrong_count) {
		fprintf(stderr, "[QM] Invalid random_idx: %d (wrong_count=%d) for round %d, time=%ld, session_id=%lld\n", 
			random_idx, wrong_count, round, (long)now, (long long)qm_sess->session_id);
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
		return;
	}
	
	char keep_wrong = wrong_options[random_idx];
	
	// Validate keep_wrong immediately after assignment
	if (keep_wrong < 'A' || keep_wrong > 'D') {
		fprintf(stderr, "[QM] Invalid keep_wrong after assignment: %c (0x%02x) for round %d\n"
			"  correct_op=%c (0x%02x)\n"
			"  wrong_options=[%c (0x%02x), %c (0x%02x), %c (0x%02x)]\n"
			"  random_idx=%d, wrong_count=%d\n",
			keep_wrong, (unsigned char)keep_wrong, round,
			correct_op, (unsigned char)correct_op,
			wrong_options[0], (unsigned char)wrong_options[0],
			wrong_options[1], (unsigned char)wrong_options[1],
			wrong_options[2], (unsigned char)wrong_options[2],
			random_idx, wrong_count);
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
		return;
	}
	
	// Normalize keep_wrong to uppercase (should already be uppercase, but ensure it)
	if (keep_wrong >= 'a' && keep_wrong <= 'd') {
		keep_wrong = keep_wrong - 'a' + 'A';
	}
	
	// Final validation
	if (keep_wrong < 'A' || keep_wrong > 'D') {
		fprintf(stderr, "[QM] Invalid keep_wrong after normalize: %c (0x%02x) for round %d\n", 
			keep_wrong, (unsigned char)keep_wrong, round);
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
		return;
	}

	// Determine remaining options - initialize explicitly
	char remaining[2];
	remaining[0] = correct_op;
	remaining[1] = keep_wrong;

	// Determine removed options - initialize explicitly
	char removed[2] = {'\0', '\0'};  // Initialize to null characters
	int removed_idx = 0;
	for (char opt = 'A'; opt <= 'D'; opt++) {
		if (opt != correct_op && opt != keep_wrong) {
			if (removed_idx >= 2) {
				fprintf(stderr, "[QM] Error: removed_idx overflow for round %d\n", round);
				protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
				return;
			}
			removed[removed_idx++] = opt;
		}
	}
	
	// Validate removed array is complete
	if (removed_idx != 2) {
		fprintf(stderr, "[QM] Invalid removed_idx: %d (expected 2) for round %d\n", removed_idx, round);
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
		return;
	}
	
	// Final validation: ensure all values are valid before sending
	if (remaining[0] < 'A' || remaining[0] > 'D' ||
	    remaining[1] < 'A' || remaining[1] > 'D' ||
	    removed[0] < 'A' || removed[0] > 'D' ||
	    removed[1] < 'A' || removed[1] > 'D') {
		fprintf(stderr, "[QM] Invalid option values: remaining=[%c,%c] removed=[%c,%c] for round %d\n",
			remaining[0], remaining[1], removed[0], removed[1], round);
		protocol_send_error(sess, CMD_RES_USE_LIFELINE, "INTERNAL_ERROR");
		return;
	}

	// Update lifeline usage
	qm_sess->lifeline_5050_used++;
	qm_sess->lifeline_5050_rounds[qm_sess->lifeline_5050_used - 1] = round;

	// Build response
	char json[512];
	int remaining_count = 2 - qm_sess->lifeline_5050_used;

	snprintf(json, sizeof(json),
		"{"
		"\"session_id\": %lld, "
		"\"round\": %d, "
		"\"lifeline_type\": \"50-50\", "
		"\"remaining_options\": [\"%c\", \"%c\"], "
		"\"removed_options\": [\"%c\", \"%c\"], "
		"\"lifeline_remaining\": %d"
		"}",
		(long long)session_id, round,
		remaining[0], remaining[1],
		removed[0], removed[1],
		remaining_count);

	protocol_send_response(sess, CMD_RES_USE_LIFELINE, json, strlen(json));
}

static void quickmode_handle_game_over(QuickModeSession *sess) {
	if (!sess || !sess->client_session) return;

	int won = (sess->game_status == 1) ? 1 : 0;

	char json[512];
	snprintf(json, sizeof(json),
		"{"
		"\"session_id\": %lld, "
		"\"final_score\": %d, "
		"\"total_rounds\": 15, "
		"\"correct_answers\": %d, "
		"\"status\": \"%s\", "
		"\"win\": %s"
		"}",
		(long long)sess->session_id,
		sess->score,
		sess->score,
		won ? "won" : "lost",
		won ? "true" : "false");

	protocol_send_response(sess->client_session, CMD_NOTIFY_GAME_OVER, json, strlen(json));

	// Update stats in database
	dao_stats_update_quickmode_game(sess->user_id, won);

	// Cleanup
	quickmode_session_free(sess);
}

// ============================================================
// CLEANUP
// ============================================================

void quickmode_cleanup_user(int64_t user_id) {
	QuickModeSession *sess = quickmode_session_find(user_id);
	if (sess) {
		quickmode_session_free(sess);
	}
}

// ============================================================
// DISPATCHER
// ============================================================

void quickmode_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
	(void)payload_len;

	if (!sess) {
		protocol_send_error(sess, cmd, "NO_SESSION");
		return;
	}

	switch (cmd) {
		case CMD_REQ_START_QUICKMODE:
			quickmode_handle_start(sess, payload);
			break;

		case CMD_REQ_GET_QUESTION:
			quickmode_handle_get_question(sess, payload);
			break;

		case CMD_REQ_SUBMIT_ANSWER:
			quickmode_handle_submit_answer(sess, payload);
			break;

		case CMD_REQ_USE_LIFELINE:
			quickmode_handle_use_lifeline(sess, payload);
			break;

		default:
			protocol_send_error(sess, cmd, "UNKNOWN_QUICKMODE_CMD");
			break;
	}
}

// ============================================================
// DEBUG FUNCTION (keep for compatibility)
// ============================================================

int qm_debug_start(int64_t user_id) {
	(void)user_id;
	Question q;
	if (dao_question_get_random("EASY", &q) != 0) {
		fprintf(stderr, "[QM] get_random question failed\n");
		return -1;
	}

	printf("=== QUICKMODE DEBUG ===\n");
	printf("User: %" PRId64 "\n", user_id);
	printf("Question %" PRId64 "\n", q.question_id);
	printf("Q: %s\n", q.content);
	printf("A: %s\n", q.op_a);
	printf("B: %s\n", q.op_b);
	printf("C: %s\n", q.op_c);
	printf("D: %s\n", q.op_d);
	printf("Correct: %s\n", q.correct_op);

	return 0;
}
