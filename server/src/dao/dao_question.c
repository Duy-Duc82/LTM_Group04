// server/src/dao/dao_question.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "db.h"
#include "dao/dao_question.h"

int dao_question_get_random(const char *difficulty, Question *out_q) {
    if (!db_is_ok()) return -1;

    const char *sql =
        "SELECT question_id, difficulty_level, content, "
        "       \"opA\", \"opB\", \"opC\", \"opD\", correct_op "
        "FROM question "
        "WHERE difficulty_level = $1 "
        "ORDER BY random() LIMIT 1;";

    const char *params[1] = { difficulty };
    int paramLengths[1]   = { (int)strlen(difficulty) };
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
        db_log_error(res, "dao_question_get_random failed");
        return -1;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return -1;
    }

    if (out_q) {
        memset(out_q, 0, sizeof(*out_q));
        out_q->question_id = atoll(PQgetvalue(res, 0, 0));
        strncpy(out_q->difficulty, PQgetvalue(res, 0, 1), 15);
        strncpy(out_q->content, PQgetvalue(res, 0, 2), 1023);
        strncpy(out_q->op_a, PQgetvalue(res, 0, 3), 511);
        strncpy(out_q->op_b, PQgetvalue(res, 0, 4), 511);
        strncpy(out_q->op_c, PQgetvalue(res, 0, 5), 511);
        strncpy(out_q->op_d, PQgetvalue(res, 0, 6), 511);
        strncpy(out_q->correct_op, PQgetvalue(res, 0, 7), 1);
    }

    PQclear(res);
    return 0;
}
