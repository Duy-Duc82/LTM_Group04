// server/src/test/test_quickmode.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "db.h"
#include "service/quickmode_service.h"
#include "dao/dao_question.h"
#include "utils/timer.h"

#define ANSWER_TIMEOUT 5  // 5 seconds

int main(void) {
    const char *conninfo = getenv("DB_CONN");
    if (!conninfo) {
        fprintf(stderr, "Set DB_CONN env first\n");
        return 1;
    }
    if (db_connect(conninfo) != 0) return 1;

    // giả sử đã có user 'alice'
    int64_t user_id = 1;   // tùy DB, có thể query trước, tạm hard-code để test

    // Quickmode không lưu vào DB nữa, chỉ test game logic trong memory
    printf("Quickmode test (no DB storage): user_id=%lld\n", (long long)user_id);

    int correct = 0;
    
    for (int round = 1; round <= 15; ++round) {
        const char *difficulty = (round <= 5) ? "EASY" : (round <= 10) ? "MEDIUM" : "HARD";
        Question q;
        if (dao_question_get_random(difficulty, &q) != 0) {
            fprintf(stderr, "QM: failed to get question for difficulty=%s\n", difficulty);
            break;
        }

        printf("\n=== ROUND %d (%s) ===\n", round, difficulty);
        printf("Q: %s\n", q.content);
        printf("A: %s\n", q.op_a);
        printf("B: %s\n", q.op_b);
        printf("C: %s\n", q.op_c);
        printf("D: %s\n", q.op_d);
        printf("\n⏱️  Time: %d seconds\n", ANSWER_TIMEOUT);
        printf("Your answer (A/B/C/D): ");
        fflush(stdout);
        
        // Tạo timer 5 giây
        Timer answer_timer = timer_init(ANSWER_TIMEOUT);
        
        // Countdown display loop
        char ans = 0;
        int last_remaining = ANSWER_TIMEOUT + 1;
        
        while (!timer_is_expired(&answer_timer)) {
            int remaining = timer_get_remaining(&answer_timer);
            
            // Chỉ print khi thời gian thay đổi
            if (remaining != last_remaining) {
                printf("[%d] ", remaining);
                fflush(stdout);
                last_remaining = remaining;
            }
            
            // Read input (non-blocking check)
            // Thử đọc character
            int c = scanf("%c", &ans);
            if (c == 1 && ans != '\n') {
                printf("\nYou answered: %c\n", ans);
                goto answer_received;
            }
            
            // Sleep 100ms trước khi check lại
            usleep(100000);
        }
        
        // Timeout - không có input
        printf("\n⏱️  TIME'S UP! Game Over!\n");
        printf("Total correct answers: %d/15\n", correct);
        break;
        
answer_received:
        if (ans == 'q' || ans == 'Q') {
            printf("Quit by user. Correct answers so far: %d\n", correct);
            break;
        }
        
        // Normalize
        if (ans >= 'a' && ans <= 'z') ans = ans - 'a' + 'A';

        char correct_op = q.correct_op[0];
        if (correct_op >= 'a' && correct_op <= 'z') correct_op = correct_op - 'a' + 'A';

        if (ans == correct_op) {
            ++correct;
            printf("✓ Correct! Total correct = %d/15\n", correct);
            // continue to next round
        } else {
            printf("✗ Wrong! Correct answer was %c. You answered %c. Game Over!\n", correct_op, ans);
            printf("Total correct answers: %d/15\n", correct);
            break;
        }
    }
    
    printf("\n========== GAME SUMMARY ==========\n");
    printf("Total correct answers: %d/15\n", correct);
    printf("Success rate: %.1f%%\n", (correct / 15.0) * 100);
    printf("==================================\n");

    db_disconnect();
    return 0;
}
