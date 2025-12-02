// server/src/test/test_quickmode.c
#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "service/quickmode_service.h"
#include "dao/dao_quickmode.h"
#include "dao/dao_question.h"

int main(void) {
    const char *conninfo = getenv("DB_CONN");
    if (!conninfo) {
        fprintf(stderr, "Set DB_CONN env first\n");
        return 1;
    }
    if (db_connect(conninfo) != 0) return 1;

    // giả sử đã có user 'alice'
    int64_t user_id = 1;   // tùy DB, có thể query trước, tạm hard-code để test

    // Interactive quickmode test: create session, loop rounds until wrong answer
    QuickmodeSession sess;
    if (dao_qm_create_session(user_id, &sess) != 0) {
        fprintf(stderr, "QM: create session failed\n");
        db_disconnect();
        return 1;
    }

    printf("Quickmode session created: session_id=%lld user_id=%lld\n", (long long)sess.session_id, (long long)sess.user_id);

    int correct = 0;
    for (int round = 1; round <= 15; ++round) {
        const char *difficulty = (round <= 5) ? "EASY" : (round <= 10) ? "MEDIUM" : "HARD";
        Question q;
        if (dao_question_get_random(difficulty, &q) != 0) {
            fprintf(stderr, "QM: failed to get question for difficulty=%s\n", difficulty);
            break;
        }

        QuickmodeRound rd;
        if (dao_qm_create_round(sess.session_id, round, q.question_id, q.difficulty, &rd) != 0) {
            fprintf(stderr, "QM: failed to create round %d\n", round);
            break;
        }

        printf("\n=== ROUND %d (%s) ===\n", round, difficulty);
        printf("Q: %s\n", q.content);
        printf("A: %s\n", q.op_a);
        printf("B: %s\n", q.op_b);
        printf("C: %s\n", q.op_c);
        printf("D: %s\n", q.op_d);

        printf("Your answer (A/B/C/D, q to quit): ");
        char buf[16];
        if (!fgets(buf, sizeof(buf), stdin)) break;
        char ans = buf[0];
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
            printf("Correct! total correct = %d\n", correct);
            // continue to next round
        } else {
            printf("Wrong. correct answer was %c. You answered %c. Total correct = %d\n", correct_op, ans, correct);
            break;
        }
    }

    db_disconnect();
    return 0;
}
