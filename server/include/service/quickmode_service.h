// server/include/service/quickmode_service.h
#ifndef QUICKMODE_SERVICE_H
#define QUICKMODE_SERVICE_H

#include <stdint.h>
#include <time.h>
#include "service/client_session.h"
#include "dao/dao_question.h"

// QuickMode Session - Game state in memory
typedef struct QuickModeSession {
	int64_t user_id;
	int64_t session_id;
	ClientSession *client_session;
	int current_round;              // Current round (1-15)
	int score;                      // Number of correct answers (0-15)
	Question questions[15];         // 15 questions loaded at start
	char answers[15];               // Answers submitted ('A'/'B'/'C'/'D' or '\0')
	int correct[15];                // 1=correct, 0=wrong
	time_t start_time;              // Game start time
	int game_status;                // 0=playing, 1=won, 2=lost
	
	// Lifeline 50-50 tracking (max 2 uses per session)
	int lifeline_5050_used;         // Number of times used (0-2)
	int lifeline_5050_rounds[2];    // Rounds where lifeline was used
	
	struct QuickModeSession *next;  // For linked list
} QuickModeSession;

// Hàm test: tạo session + 1 round và in câu hỏi ra console
int qm_debug_start(int64_t user_id);

// Dispatch quickmode-related commands
void quickmode_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);

// Cleanup quickmode session when client disconnects
void quickmode_cleanup_user(int64_t user_id);

#endif
