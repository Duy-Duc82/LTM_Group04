// dao_sessions.c
#include "..\include\dao\dao_sessions.h"
#include "..\include\db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dao_sessions_init() {
    // insert
    if (!db_prepare(
            "session_insert",
            "INSERT INTO user_sessions (user_id, access_token, expires_at) "
            "VALUES ($1, $2, $3) RETURNING id",
            3)) return 0;

    // validate
    if (!db_prepare(
            "session_validate",
            "SELECT id FROM user_sessions "
            "WHERE user_id = $1 AND access_token = $2 AND expires_at > NOW()",
            2)) return 0;

    // heartbeat
    if (!db_prepare(
            "session_heartbeat",
            "UPDATE user_sessions "
            "SET last_heartbeat = NOW() "
            "WHERE user_id = $1 AND access_token = $2",
            2)) return 0;

    // delete
    if (!db_prepare(
            "session_delete",
            "DELETE FROM user_sessions "
            "WHERE user_id = $1 AND access_token = $2",
            2)) return 0;

    return 1;
}

int64_t dao_session_create(int64_t user_id, const char *access_token, const char *expires_at_iso) {
    char buf_id[32];
    snprintf(buf_id, sizeof(buf_id), "%lld", (long long)user_id);

    const char *params[3] = { buf_id, access_token, expires_at_iso };

    PGresult *res = db_exec_params("session_insert", 3, params);
    if (!res) return 0;

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        db_print_error(res);
        PQclear(res);
        return 0;
    }

    int64_t id = db_get_int64(res, 0, 0);
    PQclear(res);
    return id;
}

int dao_session_validate(int64_t user_id, const char *access_token) {
    char buf_id[32];
    snprintf(buf_id, sizeof(buf_id), "%lld", (long long)user_id);

    const char *params[2] = { buf_id, access_token };

    PGresult *res = db_exec_params("session_validate", 2, params);
    if (!res) return -1;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_print_error(res);
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    PQclear(res);

    return rows > 0 ? 1 : 0;
}

int dao_session_heartbeat(int64_t user_id, const char *access_token) {
    char buf_id[32];
    snprintf(buf_id, sizeof(buf_id), "%lld", (long long)user_id);
    const char *params[2] = { buf_id, access_token };

    PGresult *res = db_exec_params("session_heartbeat", 2, params);
    if (!res) return 0;

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_print_error(res);
        PQclear(res);
        return 0;
    }

    PQclear(res);
    return 1;
}

int dao_session_delete(int64_t user_id, const char *access_token) {
    char buf_id[32];
    snprintf(buf_id, sizeof(buf_id), "%lld", (long long)user_id);
    const char *params[2] = { buf_id, access_token };

    PGresult *res = db_exec_params("session_delete", 2, params);
    if (!res) return 0;

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_print_error(res);
        PQclear(res);
        return 0;
    }

    PQclear(res);
    return 1;
}
