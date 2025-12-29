// server/src/service/onevn_service.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "service/onevn_service.h"
#include "service/commands.h"
#include "service/protocol.h"
#include "service/dispatcher.h"
#include "service/session_manager.h"
#include "dao/dao_rooms.h"
#include "dao/dao_onevn.h"
#include "dao/dao_question.h"
#include "dao/dao_stats.h"
#include "utils/json.h"
#include "utils/timer.h"

// Game state structure (in-memory, per session)
typedef struct OneVNGameState {
    int64_t session_id;
    int64_t room_id;
    int current_round;
    int total_rounds;
    int easy_count;
    int medium_count;
    int hard_count;
    int easy_done;
    int medium_done;
    int hard_done;
    char current_difficulty[16];
    Question current_question;
    int64_t *player_ids;
    int player_count;
    int *player_scores;
    int *player_consecutive_correct;
    int *player_eliminated;
    int64_t *player_answered_round;  // Track which round each player last answered
    int timer_id;  // Timer ID for current round
    // Track used question IDs to prevent duplicates
    int64_t *used_question_ids;
    int used_question_count;
    int used_question_capacity;
} OneVNGameState;

// Forward declarations
static void send_next_question(OneVNGameState *state);
static void end_game(OneVNGameState *state, int64_t winner_id);
static void round_timeout_callback(int64_t context_id, void *user_data);
static void delayed_question_callback(int64_t context_id, void *user_data);

// Global game states (in production, use hash table or better structure)
#define MAX_GAMES 100
static OneVNGameState *game_states[MAX_GAMES];
static int game_count = 0;
static void round_timeout_callback(int64_t context_id, void *user_data);

// Helper: Calculate score for a question
static int calculate_score(const char *difficulty, double time_left_percent, int consecutive_correct) {
    int base_score = 0;
    if (strcmp(difficulty, "EASY") == 0) {
        base_score = 1000;
    } else if (strcmp(difficulty, "MEDIUM") == 0) {
        base_score = 1500;
    } else if (strcmp(difficulty, "HARD") == 0) {
        base_score = 2000;
    }

    // Score = base_score * time_percent + bonus
    int score = (int)(base_score * time_left_percent / 100.0);

    // Bonus for consecutive correct
    if (consecutive_correct >= 5) {
        score += 500;
    } else if (consecutive_correct == 4) {
        score += 200;
    } else if (consecutive_correct == 3) {
        score += 100;
    }

    return score;
}

// Helper: Get game state by session_id
static OneVNGameState *get_game_state(int64_t session_id) {
    for (int i = 0; i < game_count; i++) {
        if (game_states[i] && game_states[i]->session_id == session_id) {
            return game_states[i];
        }
    }
    return NULL;
}

// Helper: Get game state by room_id
static OneVNGameState *get_game_state_by_room(int64_t room_id) {
    for (int i = 0; i < game_count; i++) {
        if (game_states[i] && game_states[i]->room_id == room_id) {
            return game_states[i];
        }
    }
    return NULL;
}

// Helper: Free game state
static void free_game_state(OneVNGameState *state) {
    if (!state) return;
    if (state->player_ids) free(state->player_ids);
    if (state->player_scores) free(state->player_scores);
    if (state->player_consecutive_correct) free(state->player_consecutive_correct);
    if (state->player_eliminated) free(state->player_eliminated);
    if (state->player_answered_round) free(state->player_answered_round);
    if (state->used_question_ids) free(state->used_question_ids);
    free(state);
}

// Helper: Initialize game state
static OneVNGameState *init_game_state(int64_t session_id, int64_t room_id, 
                                       int easy_count, int medium_count, int hard_count,
                                       int64_t *player_ids, int player_count) {
    OneVNGameState *state = calloc(1, sizeof(OneVNGameState));
    if (!state) return NULL;

    state->session_id = session_id;
    state->room_id = room_id;
    state->easy_count = easy_count;
    state->medium_count = medium_count;
    state->hard_count = hard_count;
    state->total_rounds = easy_count + medium_count + hard_count;
    state->current_round = 0;
    state->easy_done = 0;
    state->medium_done = 0;
    state->hard_done = 0;
    state->player_count = player_count;
    state->current_round = 0;  // Will be incremented when first question is sent

    state->player_ids = malloc(sizeof(int64_t) * player_count);
    state->player_scores = calloc(player_count, sizeof(int));
    state->player_consecutive_correct = calloc(player_count, sizeof(int));
    state->player_eliminated = calloc(player_count, sizeof(int));
    state->player_answered_round = calloc(player_count, sizeof(int64_t));
    state->timer_id = -1;
    
    // Initialize duplicate question tracking
    state->used_question_capacity = state->total_rounds + 10;  // Extra capacity
    state->used_question_ids = calloc(state->used_question_capacity, sizeof(int64_t));
    state->used_question_count = 0;

    if (!state->player_ids || !state->player_scores || 
        !state->player_consecutive_correct || !state->player_eliminated ||
        !state->player_answered_round || !state->used_question_ids) {
        free_game_state(state);
        return NULL;
    }

    for (int i = 0; i < player_count; i++) {
        state->player_ids[i] = player_ids[i];
        state->player_scores[i] = 0;
        state->player_consecutive_correct[i] = 0;
        state->player_eliminated[i] = 0;
        state->player_answered_round[i] = -1;
    }

    return state;
}

// Helper: Select next question difficulty
static const char *select_next_difficulty(OneVNGameState *state) {
    // Random selection based on remaining questions
    int total_remaining = (state->easy_count - state->easy_done) +
                         (state->medium_count - state->medium_done) +
                         (state->hard_count - state->hard_done);
    
    if (total_remaining == 0) return NULL;

    // Simple round-robin: prioritize easy, then medium, then hard
    if (state->easy_done < state->easy_count) {
        return "EASY";
    } else if (state->medium_done < state->medium_count) {
        return "MEDIUM";
    } else {
        return "HARD";
    }
}

// Helper: Build leaderboard JSON
static char *build_leaderboard_json(OneVNGameState *state) {
    // Create array of players sorted by score
    int *indices = malloc(sizeof(int) * state->player_count);
    if (!indices) return NULL;

    for (int i = 0; i < state->player_count; i++) {
        indices[i] = i;
    }

    // Sort by score (descending)
    for (int i = 0; i < state->player_count - 1; i++) {
        for (int j = i + 1; j < state->player_count; j++) {
            if (state->player_scores[indices[i]] < state->player_scores[indices[j]]) {
                int tmp = indices[i];
                indices[i] = indices[j];
                indices[j] = tmp;
            }
        }
    }

    size_t cap = 512;
    size_t used = 0;
    char *json = malloc(cap);
    if (!json) { free(indices); return NULL; }
    used += snprintf(json + used, cap - used, "[");

    for (int i = 0; i < state->player_count; i++) {
        int idx = indices[i];
        int rank = i + 1;
        // No elimination - all players continue, eliminated always false
        int need = snprintf(NULL, 0, 
            "{\"rank\":%d,\"user_id\":%ld,\"score\":%d,\"eliminated\":false}",
            rank, state->player_ids[idx], state->player_scores[idx]);
        
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(json, cap);
            if (!tmp) { free(json); free(indices); return NULL; }
            json = tmp;
        }

        if (i > 0) used += snprintf(json + used, cap - used, ",");
        used += snprintf(json + used, cap - used,
            "{\"rank\":%d,\"user_id\":%ld,\"score\":%d,\"eliminated\":false}",
            rank, state->player_ids[idx], state->player_scores[idx]);
    }

    used += snprintf(json + used, cap - used, "]");
    free(indices);
    return json;
}

// Helper: Check game end conditions
static int check_game_end(OneVNGameState *state, int64_t *winner_id) {
    // All questions done -> highest score wins (no elimination, all players finish)
    if (state->current_round >= state->total_rounds) {
        int max_score = -1;
        int winner_idx = -1;
        for (int i = 0; i < state->player_count; i++) {
            if (state->player_scores[i] > max_score) {
                max_score = state->player_scores[i];
                winner_idx = i;
            }
        }
        if (winner_idx >= 0) {
            *winner_id = state->player_ids[winner_idx];
        } else {
            *winner_id = 0;
        }
        return 1;
    }

    return 0;
}

// Start 1vN game
static void handle_start_game(ClientSession *sess, const char *payload, uint32_t payload_len) {
    (void)payload_len;
    
    long long room_id_ll = 0;
    util_json_get_int64(payload, "room_id", &room_id_ll);
    int64_t room_id = (int64_t)room_id_ll;
    if (room_id == 0) {
        protocol_send_error(sess, CMD_RES_START_GAME, "INVALID_ROOM_ID");
        return;
    }

    // Check if user is owner
    int64_t owner_id = 0;
    if (dao_rooms_get_owner(room_id, &owner_id) != 0 || owner_id != sess->user_id) {
        protocol_send_error(sess, CMD_RES_START_GAME, "NOT_OWNER");
        return;
    }

    // Get room members
    void *members_json = NULL;
    if (dao_rooms_get_members(room_id, &members_json) != 0) {
        protocol_send_error(sess, CMD_RES_START_GAME, "GET_MEMBERS_FAILED");
        return;
    }

    // Parse members JSON to get actual member list using improved parser
    int64_t player_ids_temp[32];
    int member_count = util_json_parse_user_id_array((const char *)members_json, player_ids_temp, 32);
    
    // Debug: Print to both stderr and include in error message
    printf("[ONEVN] Room %lld has %d members. Members JSON: %s\n", 
           (long long)room_id, member_count, (const char *)members_json);
    fflush(stdout);

    // Get room config
    int easy_count = 0, medium_count = 0, hard_count = 0;
    if (dao_rooms_get_config(room_id, &easy_count, &medium_count, &hard_count) != 0) {
        printf("[ONEVN] WARNING: Could not get room config, using defaults (5,5,5)\n");
        easy_count = 5;
        medium_count = 5;
        hard_count = 5;
    }
    
    // If config is all zeros, use defaults and update database
    if (easy_count == 0 && medium_count == 0 && hard_count == 0) {
        printf("[ONEVN] WARNING: Room config is (0,0,0), updating to defaults (5,5,5)\n");
        easy_count = 5;
        medium_count = 5;
        hard_count = 5;
        // Update room with default config in database
        dao_rooms_update_config(room_id, easy_count, medium_count, hard_count);
    }
    
    printf("[ONEVN] Room %lld config: easy=%d, medium=%d, hard=%d (total=%d)\n",
           (long long)room_id, easy_count, medium_count, hard_count,
           easy_count + medium_count + hard_count);
    fflush(stdout);

    // Check minimum 2 players
    if (member_count < 2) {
        printf("[ONEVN] ERROR: NOT_ENOUGH_PLAYERS: member_count=%d, members_json=%s\n",
               member_count, (const char *)members_json);
        fflush(stdout);
        free(members_json);
        protocol_send_error(sess, CMD_RES_START_GAME, "NOT_ENOUGH_PLAYERS");
        return;
    }

    // Create 1vN session
    int64_t session_id = 0;
    if (dao_onevn_create_session(room_id, &session_id) != 0) {
        free(members_json);
        protocol_send_error(sess, CMD_RES_START_GAME, "CREATE_SESSION_FAILED");
        return;
    }

    // Update room status
    dao_rooms_update_status(room_id, ROOM_STATUS_IN_PROGRESS);

    // Copy player IDs
    int64_t *player_ids = malloc(sizeof(int64_t) * member_count);
    if (!player_ids) {
        free(members_json);
        protocol_send_error(sess, CMD_RES_START_GAME, "MEMORY_ERROR");
        return;
    }
    
    for (int i = 0; i < member_count; i++) {
        player_ids[i] = player_ids_temp[i];
    }
    
    int idx = member_count;

    OneVNGameState *state = init_game_state(session_id, room_id, 
                                            easy_count, medium_count, hard_count,
                                            player_ids, idx);
    if (!state) {
        free(player_ids);
        free(members_json);
        protocol_send_error(sess, CMD_RES_START_GAME, "INIT_GAME_FAILED");
        return;
    }

    // Store game state
    if (game_count < MAX_GAMES) {
        game_states[game_count++] = state;
    } else {
        free_game_state(state);
        free(player_ids);
        free(members_json);
        protocol_send_error(sess, CMD_RES_START_GAME, "MAX_GAMES_REACHED");
        return;
    }

    // Initialize players JSONB in database
    size_t players_init_cap = 512;
    char *players_init = malloc(players_init_cap);
    if (players_init) {
        int used = snprintf(players_init, players_init_cap, "[");
        for (int i = 0; i < idx; i++) {
            if (i > 0) used += snprintf(players_init + used, players_init_cap - used, ",");
            used += snprintf(players_init + used, players_init_cap - used,
                "{\"user_id\":%ld,\"score\":0,\"eliminated\":false}", player_ids[i]);
        }
        snprintf(players_init + used, players_init_cap - used, "]");
        dao_onevn_update_players(session_id, players_init);
        free(players_init);
    }

    // Update sessions' room_id BEFORE broadcasting (important!)
    for (int i = 0; i < idx; i++) {
        ClientSession *player_sess = session_manager_get_by_user_id(player_ids[i]);
        if (player_sess) {
            session_manager_set_room(player_sess, room_id);
            printf("[ONEVN] Set room_id=%lld for user_id=%lld\n",
                   (long long)room_id, (long long)player_ids[i]);
            fflush(stdout);
        } else {
            printf("[ONEVN] WARNING: Could not find session for user_id=%lld\n",
                   (long long)player_ids[i]);
            fflush(stdout);
        }
    }
    
    // Broadcast start game notification to all players in room
    char response[256];
    snprintf(response, sizeof(response), 
        "{\"session_id\":%ld,\"room_id\":%ld,\"total_rounds\":%d}",
        session_id, room_id, state->total_rounds);
    session_manager_broadcast_to_room(room_id, CMD_NOTIFY_GAME_START_1VN, response, strlen(response));
    
    // Also send response to owner
    protocol_send_response(sess, CMD_RES_START_GAME, response, strlen(response));
    
    // Send first question after a short delay (in production, use async)
    // For now, we'll send it immediately
    send_next_question(state);

    free(player_ids);
    free(members_json);
}

// Submit answer in 1vN mode
static void handle_submit_answer_1vn(ClientSession *sess, const char *payload, uint32_t payload_len) {
    (void)payload_len;

    long long session_id_ll = 0;
    long long round_ll = 0;
    char answer[2] = {0};
    double time_left = 0.0;

    util_json_get_int64(payload, "session_id", &session_id_ll);
    util_json_get_int64(payload, "round", &round_ll);
    int64_t session_id = (int64_t)session_id_ll;
    int64_t round = (int64_t)round_ll;
    char *answer_str = util_json_get_string(payload, "answer");
    util_json_get_double(payload, "time_left", &time_left);

    if (answer_str && strlen(answer_str) > 0) {
        answer[0] = answer_str[0];
    }
    if (answer_str) free(answer_str);

    OneVNGameState *state = get_game_state(session_id);
    if (!state) {
        protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER_1VN, "SESSION_NOT_FOUND");
        return;
    }

    // Find player index
    int player_idx = -1;
    for (int i = 0; i < state->player_count; i++) {
        if (state->player_ids[i] == sess->user_id) {
            player_idx = i;
            break;
        }
    }

    if (player_idx < 0) {
        protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER_1VN, "NOT_IN_GAME");
        return;
    }

    // Check if already eliminated
    if (state->player_eliminated[player_idx]) {
        protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER_1VN, "ALREADY_ELIMINATED");
        return;
    }

    // Validate round matches current round - use server's current_round as source of truth
    if (round != state->current_round) {
        printf("[ONEVN] WARNING: Round mismatch - client sent round=%ld, server current_round=%d, using server round\n",
               (long)round, state->current_round);
        fflush(stdout);
    }
    // Always use server's current_round to ensure consistency
    int64_t server_round = state->current_round;
    
    // Check if already answered this round
    if (state->player_answered_round[player_idx] == server_round) {
        protocol_send_error(sess, CMD_RES_SUBMIT_ANSWER_1VN, "ALREADY_ANSWERED");
        return;
    }

    // Check answer
    int is_correct = (answer[0] == state->current_question.correct_op[0]);
    
    if (is_correct) {
        // Calculate score
        double time_percent = (time_left / 15.0) * 100.0;  // 15 seconds total
        if (time_percent < 0) time_percent = 0;
        if (time_percent > 100) time_percent = 100;

        int score = calculate_score(state->current_difficulty, time_percent, 
                                   state->player_consecutive_correct[player_idx]);
        state->player_scores[player_idx] += score;
        state->player_consecutive_correct[player_idx]++;
    } else {
        // Wrong answer -> no points, reset consecutive correct
        // No elimination - players continue playing
        state->player_consecutive_correct[player_idx] = 0;
    }

    // Use server's current_round, not round from request
    state->player_answered_round[player_idx] = server_round;

    // Update database
    char *leaderboard = build_leaderboard_json(state);
    if (leaderboard) {
        // Update players JSONB (simplified)
        dao_onevn_update_players(session_id, leaderboard);
        free(leaderboard);
    }

    // Send response
    char response[512];
    snprintf(response, sizeof(response),
        "{\"correct\":%s,\"score\":%d,\"total_score\":%d,\"eliminated\":false}",
        is_correct ? "true" : "false",
        is_correct ? calculate_score(state->current_difficulty, (time_left / 15.0) * 100.0, 
                                    state->player_consecutive_correct[player_idx] - 1) : 0,
        state->player_scores[player_idx]);
    protocol_send_response(sess, CMD_RES_SUBMIT_ANSWER_1VN, response, strlen(response));
    
    // Check if all players have answered (no elimination check - all players continue)
    // Use state->current_round as source of truth
    int all_answered = 1;
    printf("[ONEVN] ========== Checking if all players answered round %d ==========\n", state->current_round);
    printf("[ONEVN] Player who just answered: user_id=%ld, player_idx=%d\n", 
           (long)sess->user_id, player_idx);
    for (int i = 0; i < state->player_count; i++) {
        int has_answered = (state->player_answered_round[i] == state->current_round);
        printf("[ONEVN]   Player[%d] user_id=%ld: answered_round=%ld, current_round=%d, has_answered=%s\n",
               i, (long)state->player_ids[i], (long)state->player_answered_round[i], 
               state->current_round, has_answered ? "YES" : "NO");
        if (!has_answered) {
            all_answered = 0;
        }
    }
    printf("[ONEVN] ========== All answered: %s ==========\n", all_answered ? "YES" : "NO");
    fflush(stdout);
    
    // If all players answered, end round and send next question
    if (all_answered) {
        printf("[ONEVN] ========== ALL PLAYERS ANSWERED - Scheduling next question in 2 seconds ==========\n");
        fflush(stdout);
        // Cancel timer
        if (state->timer_id >= 0) {
            game_timer_cancel(state->timer_id);
            state->timer_id = -1;
        }
        
        // IMPORTANT: Broadcast leaderboard FIRST before sending next question
        // This ensures clients see the results before the next question arrives
        char *leaderboard = build_leaderboard_json(state);
        if (leaderboard) {
            char leaderboard_notify[2048];
            snprintf(leaderboard_notify, sizeof(leaderboard_notify),
                "{\"leaderboard\": %s}", leaderboard);
            session_manager_broadcast_to_room(state->room_id, CMD_NOTIFY_ROOM_UPDATE,
                                             leaderboard_notify, strlen(leaderboard_notify));
            free(leaderboard);
        }
        
        // Schedule next question after 3 seconds delay
        // This ensures the last player to answer has at least 2 seconds to see their score notification
        // All players will receive the next question at the same time (3 seconds after all answered)
        state->timer_id = game_timer_create(3, state->session_id, delayed_question_callback, state);
        if (state->timer_id < 0) {
            printf("[ONEVN] ERROR: Failed to create delay timer, sending question immediately\n");
            fflush(stdout);
            int64_t winner_id = 0;
            if (check_game_end(state, &winner_id)) {
                end_game(state, winner_id);
            } else {
                send_next_question(state);
            }
        }
    } else {
        printf("[ONEVN] Not all players answered yet, waiting for more answers...\n");
        fflush(stdout);
    }
}

// Timer callback for delayed question sending (2 seconds after all answered or timeout)
static void delayed_question_callback(int64_t context_id, void *user_data) {
    OneVNGameState *state = (OneVNGameState *)user_data;
    if (!state || state->session_id != context_id) {
        printf("[ONEVN] WARNING: Delayed question callback called with invalid state or context_id mismatch\n");
        fflush(stdout);
        return;
    }
    
    // Clear timer_id
    state->timer_id = -1;
    
    printf("[ONEVN] ========== DELAYED QUESTION CALLBACK - Sending next question after 3 seconds ==========\n");
    fflush(stdout);
    
    // Check if game should end
    int64_t winner_id = 0;
    if (check_game_end(state, &winner_id)) {
        printf("[ONEVN] Game should end, winner_id=%ld\n", (long)winner_id);
        fflush(stdout);
        end_game(state, winner_id);
    } else {
        // Send next question after 3 seconds delay
        printf("[ONEVN] Sending next question after 3 seconds delay...\n");
        fflush(stdout);
        send_next_question(state);
    }
}

// Timer callback for round timeout
static void round_timeout_callback(int64_t context_id, void *user_data) {
    OneVNGameState *state = (OneVNGameState *)user_data;
    if (!state || state->session_id != context_id) {
        printf("[ONEVN] WARNING: Timeout callback called with invalid state or context_id mismatch\n");
        fflush(stdout);
        return;
    }
    
    // CRITICAL: Cancel timer immediately to prevent callback from being called again
    // Also check if timer is still valid (might have been cancelled already)
    if (state->timer_id < 0) {
        printf("[ONEVN] WARNING: Timeout callback called but timer_id is already invalid (round might have ended)\n");
        fflush(stdout);
        return;
    }
    
    // Cancel timer first to prevent race conditions
    int saved_timer_id = state->timer_id;
    state->timer_id = -1;  // Mark as invalid before processing
    game_timer_cancel(saved_timer_id);
    
    printf("[ONEVN] ========== TIMEOUT CALLBACK TRIGGERED for round %d ==========\n", state->current_round);
    fflush(stdout);
    
    // Check if all players have already answered (race condition: all answered just before timeout)
    int all_answered = 1;
    for (int i = 0; i < state->player_count; i++) {
        if (state->player_answered_round[i] != state->current_round) {
            all_answered = 0;
            break;
        }
    }
    
    if (all_answered) {
        printf("[ONEVN] WARNING: Timeout callback called but all players already answered (race condition)\n");
        printf("[ONEVN] Round already ended, ignoring timeout callback\n");
        fflush(stdout);
        return;
    }
    
    // Mark all players who haven't answered as no points (but not eliminated)
    // Also send timeout notification to each player who didn't answer
    for (int i = 0; i < state->player_count; i++) {
        if (state->player_answered_round[i] != state->current_round) {
            // Timeout - no points for this round, reset consecutive correct
            state->player_consecutive_correct[i] = 0;
            // Mark as answered to prevent double processing
            state->player_answered_round[i] = state->current_round;
            
            // Send timeout notification to this player
            ClientSession *player_sess = session_manager_get_by_user_id(state->player_ids[i]);
            if (player_sess) {
                char timeout_response[512];
                snprintf(timeout_response, sizeof(timeout_response),
                    "{\"correct\":false,\"score\":0,\"total_score\":%d,\"eliminated\":false,\"timeout\":true}",
                    state->player_scores[i]);
                protocol_send_response(player_sess, CMD_RES_SUBMIT_ANSWER_1VN, 
                                      timeout_response, strlen(timeout_response));
                printf("[ONEVN] Sent timeout notification to user_id=%ld (player_idx=%d)\n",
                       (long)state->player_ids[i], i);
                fflush(stdout);
            } else {
                printf("[ONEVN] WARNING: Could not find session for user_id=%ld (player_idx=%d) to send timeout notification\n",
                       (long)state->player_ids[i], i);
                fflush(stdout);
            }
        }
    }
    
    // Update database with current scores
    char *leaderboard = build_leaderboard_json(state);
    if (leaderboard) {
        dao_onevn_update_players(state->session_id, leaderboard);
    }
    
    // IMPORTANT: Broadcast leaderboard FIRST before sending next question
    if (leaderboard) {
        char leaderboard_notify[2048];
        snprintf(leaderboard_notify, sizeof(leaderboard_notify),
            "{\"leaderboard\": %s}", leaderboard);
        session_manager_broadcast_to_room(state->room_id, CMD_NOTIFY_ROOM_UPDATE,
                                         leaderboard_notify, strlen(leaderboard_notify));
        free(leaderboard);
    }
    
    // End round and check game end
    int64_t winner_id = 0;
    if (check_game_end(state, &winner_id)) {
        printf("[ONEVN] Game should end after timeout, winner_id=%ld\n", (long)winner_id);
        fflush(stdout);
        end_game(state, winner_id);
    } else {
        // Schedule next question after 3 seconds delay (same as when all answered)
        // This ensures all players have time to see their score notification (2 seconds minimum)
        printf("[ONEVN] Scheduling next question after 3 seconds delay (timeout)...\n");
        fflush(stdout);
        state->timer_id = game_timer_create(3, state->session_id, delayed_question_callback, state);
        if (state->timer_id < 0) {
            printf("[ONEVN] ERROR: Failed to create delay timer, sending question immediately\n");
            fflush(stdout);
            send_next_question(state);
        }
    }
}

// Get next question (called by server after round ends)
static void send_next_question(OneVNGameState *state) {
    printf("[ONEVN] send_next_question called: current_round=%d, total_rounds=%d\n",
           state->current_round, state->total_rounds);
    fflush(stdout);
    
    if (state->current_round >= state->total_rounds) {
        // Game over - check winner
        printf("[ONEVN] Game over: current_round >= total_rounds\n");
        fflush(stdout);
        int64_t winner_id = 0;
        check_game_end(state, &winner_id);
        end_game(state, winner_id);
        return;
    }

    const char *difficulty = select_next_difficulty(state);
    if (!difficulty) {
        // No more questions
        printf("[ONEVN] No more questions available\n");
        fflush(stdout);
        int64_t winner_id = 0;
        check_game_end(state, &winner_id);
        end_game(state, winner_id);
        return;
    }

    strncpy(state->current_difficulty, difficulty, 15);
    state->current_round++;

    // Get random question (avoid duplicates)
    printf("[ONEVN] Getting random question for difficulty: %s\n", difficulty);
    fflush(stdout);
    
    int retries = 0;
    const int MAX_RETRIES = 100;
    int success = 0;
    
    while (retries < MAX_RETRIES) {
        if (dao_question_get_random(difficulty, &state->current_question) != 0) {
            // Failed to get question - end game
            printf("[ONEVN] ERROR: Failed to get random question for difficulty %s\n", difficulty);
            fflush(stdout);
            int64_t winner_id = 0;
            check_game_end(state, &winner_id);
            end_game(state, winner_id);
            return;
        }
        
        // Check if this question ID was already used
        int is_duplicate = 0;
        for (int i = 0; i < state->used_question_count; i++) {
            if (state->used_question_ids[i] == state->current_question.question_id) {
                is_duplicate = 1;
                break;
            }
        }
        
        if (!is_duplicate) {
            // Add to used list
            if (state->used_question_count >= state->used_question_capacity) {
                // Expand capacity
                state->used_question_capacity *= 2;
                int64_t *new_list = realloc(state->used_question_ids, 
                                           state->used_question_capacity * sizeof(int64_t));
                if (new_list) {
                    state->used_question_ids = new_list;
                } else {
                    // Memory error, but continue with current question
                    printf("[ONEVN] WARNING: Failed to expand used_question_ids list\n");
                    fflush(stdout);
                }
            }
            state->used_question_ids[state->used_question_count++] = state->current_question.question_id;
            success = 1;
            break;
        }
        
        retries++;
    }
    
    if (!success) {
        printf("[ONEVN] ERROR: Failed to get unique question after %d retries\n", MAX_RETRIES);
        fflush(stdout);
        int64_t winner_id = 0;
        check_game_end(state, &winner_id);
        end_game(state, winner_id);
        return;
    }
    
    printf("[ONEVN] Got question ID: %ld, content: %s\n", 
           state->current_question.question_id,
           state->current_question.content ? state->current_question.content : "(null)");
    fflush(stdout);

    // Update counters
    if (strcmp(difficulty, "EASY") == 0) {
        state->easy_done++;
    } else if (strcmp(difficulty, "MEDIUM") == 0) {
        state->medium_done++;
    } else if (strcmp(difficulty, "HARD") == 0) {
        state->hard_done++;
    }

    // Reset answered round for all players
    for (int i = 0; i < state->player_count; i++) {
        state->player_answered_round[i] = -1;
    }

    // Build question JSON
    char question_json[2048];
    char *esc_content = util_json_escape(state->current_question.content);
    char *esc_a = util_json_escape(state->current_question.op_a);
    char *esc_b = util_json_escape(state->current_question.op_b);
    char *esc_c = util_json_escape(state->current_question.op_c);
    char *esc_d = util_json_escape(state->current_question.op_d);

    snprintf(question_json, sizeof(question_json),
        "{\"round\":%d,\"total_rounds\":%d,\"difficulty\":\"%s\","
        "\"question_id\":%ld,\"content\":\"%s\","
        "\"options\":{\"A\":\"%s\",\"B\":\"%s\",\"C\":\"%s\",\"D\":\"%s\"},"
        "\"time_limit\":15}",
        state->current_round, state->total_rounds, difficulty,
        state->current_question.question_id,
        esc_content ? esc_content : "",
        esc_a ? esc_a : "", esc_b ? esc_b : "", esc_c ? esc_c : "", esc_d ? esc_d : "");

    if (esc_content) free(esc_content);
    if (esc_a) free(esc_a);
    if (esc_b) free(esc_b);
    if (esc_c) free(esc_c);
    if (esc_d) free(esc_d);

    // Broadcast to all players in room
    printf("[ONEVN] Broadcasting question round %d to room %lld (total players: %d)\n",
           state->current_round, (long long)state->room_id, state->player_count);
    printf("[ONEVN] Question JSON: %s\n", question_json);
    fflush(stdout);
    int broadcast_count = session_manager_broadcast_to_room(state->room_id, CMD_NOTIFY_QUESTION_1VN, 
                                      question_json, strlen(question_json));
    printf("[ONEVN] Question broadcast sent to %d sessions\n", broadcast_count);
    fflush(stdout);
    
    // Start timer for 15 seconds
    if (state->timer_id >= 0) {
        game_timer_cancel(state->timer_id);
    }
    state->timer_id = game_timer_create(15, state->session_id, round_timeout_callback, state);
    
    printf("[ONEVN] Question sent successfully, timer started\n");
    fflush(stdout);
}

// End game and send final results
static void end_game(OneVNGameState *state, int64_t winner_id) {
    if (!state) return;
    
    // Cancel timer if active
    if (state->timer_id >= 0) {
        game_timer_cancel(state->timer_id);
        state->timer_id = -1;
    }
    
    // End session in database
    dao_onevn_end_session(state->session_id, winner_id);
    dao_rooms_update_status(state->room_id, ROOM_STATUS_FINISHED);

    // Build final leaderboard
    char *final_leaderboard = build_leaderboard_json(state);
    char final_response[2048];
    snprintf(final_response, sizeof(final_response),
        "{\"winner_id\":%ld,\"leaderboard\":%s}",
        winner_id, final_leaderboard ? final_leaderboard : "[]");
    
    // Broadcast final results to all players
    session_manager_broadcast_to_room(state->room_id, CMD_NOTIFY_GAME_OVER_1VN,
                                     final_response, strlen(final_response));
    
    if (final_leaderboard) free(final_leaderboard);

    // Update stats for all players
    dao_stats_update_onevn_game(winner_id, state->player_ids, state->player_scores,
                                state->player_eliminated, state->player_count);

    // Remove game state
    for (int i = 0; i < game_count; i++) {
        if (game_states[i] == state) {
            free_game_state(state);
            game_states[i] = NULL;
            // Shift remaining games
            for (int j = i; j < game_count - 1; j++) {
                game_states[j] = game_states[j + 1];
            }
            game_count--;
            break;
        }
    }
}

// Main dispatcher
void onevn_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    switch (cmd) {
        case CMD_REQ_START_GAME:
            // Start game is handled in room dispatcher, but we can also handle it here
            handle_start_game(sess, payload, payload_len);
            break;

        case CMD_REQ_SUBMIT_ANSWER_1VN:
            handle_submit_answer_1vn(sess, payload, payload_len);
            break;

        default:
            protocol_send_error(sess, cmd, "UNKNOWN_1VN_CMD");
            break;
    }
}

