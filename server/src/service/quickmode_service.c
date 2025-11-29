// server/src/service/quickmode_service.c
#include <stdio.h>
#include "dao/dao_question.h"
#include "dao/dao_quickmode.h"
#include "service/quickmode_service.h"
#include <inttypes.h>

int qm_debug_start(int64_t user_id) {
    QuickmodeSession sess;
    if (dao_qm_create_session(user_id, &sess) != 0) {
        fprintf(stderr, "[QM] create_session failed\n");
        return -1;
    }

    Question q;
    if (dao_question_get_random("EASY", &q) != 0) {
        fprintf(stderr, "[QM] get_random question failed\n");
        return -1;
    }

    QuickmodeRound rd;
    if (dao_qm_create_round(sess.session_id, 1, q.question_id,
                            q.difficulty, &rd) != 0) {
        fprintf(stderr, "[QM] create_round failed\n");
        return -1;
    }

    printf("=== QUICKMODE DEBUG ===\n");
    printf("Session: %" PRId64 " User: %" PRId64 "\n",
           sess.session_id, sess.user_id);
    printf("Round %d, Question %" PRId64 "\n",
           rd.round_number, rd.question_id);
    printf("Q: %s\n", q.content);
    printf("A: %s\n", q.op_a);
    printf("B: %s\n", q.op_b);
    printf("C: %s\n", q.op_c);
    printf("D: %s\n", q.op_d);
    printf("Correct: %s\n", q.correct_op);

    return 0;
}
