// server/src/dao/dao_quickmode.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "db.h"
#include "dao/dao_quickmode.h"

int dao_qm_create_session(int64_t user_id, QuickmodeSession *out_sess) {
    if (!db_is_ok()) return -1;

    const char *sql =
        "INSERT INTO quickmode_sessions (user_id, status) "
        "VALUES ($1, 'IN_PROGRESS') "
        "RETURNING session_id, user_id, current_round, total_correct;";

    char uid[32];
    snprintf(uid, sizeof(uid), "%lld", (long long)user_id);

    const char *params[1] = { uid };
    int paramLengths[1]   = { (int)strlen(uid) };
    int paramFormats[1]   = { 0 };

    PGresult *res = PQexecParams(db_conn,
                                 sql,
                                 1,
                                 NULL,
                                 params,
                                 paramLengths,
                                 paramFormats,
                                 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_log_error(res, "dao_qm_create_session failed");
        return -1;
    }

    if (PQntuples(res) != 1) {
        db_log_error(res, "dao_qm_create_session no row");
        return -1;
    }

    if (out_sess) {
        memset(out_sess, 0, sizeof(*out_sess));
        out_sess->session_id    = atoll(PQgetvalue(res, 0, 0));
        out_sess->user_id       = atoll(PQgetvalue(res, 0, 1));
        out_sess->current_round = atoi(PQgetvalue(res, 0, 2));
        out_sess->total_correct = atoi(PQgetvalue(res, 0, 3));
    }

    PQclear(res);
    return 0;
}

int dao_qm_create_round(int64_t session_id, int round_no,
                        int64_t question_id, const char *difficulty,
                        QuickmodeRound *out_round) {
    if (!db_is_ok()) return -1;

    const char *sql =
        "INSERT INTO quickmode_rounds "
        "(session_id, round_number, question_id, difficulty_level) "
        "VALUES ($1, $2, $3, $4) "
        "RETURNING round_id, session_id, round_number, question_id;";

    char sid[32], rno[16], qid[32];
    snprintf(sid, sizeof(sid), "%lld", (long long)session_id);
    snprintf(rno, sizeof(rno), "%d",  round_no);
    snprintf(qid, sizeof(qid), "%lld", (long long)question_id);

    const char *params[4] = { sid, rno, qid, difficulty };
    int paramLengths[4]   = { (int)strlen(sid), (int)strlen(rno),
                              (int)strlen(qid), (int)strlen(difficulty) };
    int paramFormats[4]   = { 0,0,0,0 };

    PGresult *res = PQexecParams(db_conn,
                                 sql,
                                 4,
                                 NULL,
                                 params,
                                 paramLengths,
                                 paramFormats,
                                 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_log_error(res, "dao_qm_create_round failed");
        return -1;
    }

    if (PQntuples(res) != 1) {
        db_log_error(res, "dao_qm_create_round no row");
        return -1;
    }

    if (out_round) {
        memset(out_round, 0, sizeof(*out_round));
        out_round->round_id    = atoll(PQgetvalue(res, 0, 0));
        out_round->session_id  = atoll(PQgetvalue(res, 0, 1));
        out_round->round_number= atoi(PQgetvalue(res, 0, 2));
        out_round->question_id = atoll(PQgetvalue(res, 0, 3));
    }

    PQclear(res);
    return 0;
}
