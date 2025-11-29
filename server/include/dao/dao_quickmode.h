// server/include/dao/dao_quickmode.h
#ifndef DAO_QUICKMODE_H
#define DAO_QUICKMODE_H

#include <stdint.h>

typedef struct {
    int64_t session_id;
    int64_t user_id;
    int     current_round;
    int     total_correct;
} QuickmodeSession;

typedef struct {
    int64_t round_id;
    int64_t session_id;
    int     round_number;
    int64_t question_id;
} QuickmodeRound;

int dao_qm_create_session(int64_t user_id, QuickmodeSession *out_sess);
int dao_qm_create_round(int64_t session_id, int round_no,
                        int64_t question_id, const char *difficulty,
                        QuickmodeRound *out_round);

#endif
