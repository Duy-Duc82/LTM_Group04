#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "../include/db.h"
#include "dao/dao_friends.h"
#include "utils/json.h"

// Convert enum -> text
static const char *friend_status_to_str(friend_status_t st) {
    switch (st) {
        case FRIEND_STATUS_PENDING:  return "PENDING";
        case FRIEND_STATUS_ACCEPTED: return "ACCEPTED";
        case FRIEND_STATUS_BLOCKED:  return "BLOCKED";
        default: return "PENDING";
    }
}

int dao_friends_send_request(int64_t from_user, int64_t to_user) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO friends (user_id, friend_id, status) "
        "VALUES ($1, $2, 'PENDING') "
        "ON CONFLICT (user_id, friend_id) DO UPDATE SET status = 'PENDING', updated_at = NOW();";

    const char *params[2];
    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", from_user);
    snprintf(buf2, sizeof(buf2), "%ld", to_user);
    params[0] = buf1;
    params[1] = buf2;

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_FRIENDS] send_request error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_friends_respond_request(int64_t from_user, int64_t to_user, bool accept) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE friends "
        "SET status = $3, updated_at = NOW() "
        "WHERE user_id = $1 AND friend_id = $2 AND status = 'PENDING';";

    const char *params[3];
    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", from_user);
    snprintf(buf2, sizeof(buf2), "%ld", to_user);
    params[0] = buf1;
    params[1] = buf2;
    params[2] = accept ? "ACCEPTED" : "BLOCKED";

    PGresult *res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_FRIENDS] respond_request error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    int affected = atoi(PQcmdTuples(res));
    PQclear(res);
    return (affected > 0) ? 0 : -1;
}

// Ở đây mình giả sử bạn dùng JSON/Jansson phía service.
// DAO có thể trả về raw PGresult, nhưng để đơn giản ta trả về void* chứa JSON string.
int dao_friends_list(int64_t user_id, void **result_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT u.user_id, u.username, f.status "
        "FROM friends f "
        "JOIN users u ON u.user_id = f.friend_id "
        "WHERE f.user_id = $1 AND f.status = 'ACCEPTED';";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_FRIENDS] list error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    // Build JSON array
    size_t cap = 256;
    size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';

    for (int i = 0; i < rows; ++i) {
        const char *uid = PQgetvalue(res, i, 0);
        const char *uname = PQgetvalue(res, i, 1);
        const char *status = PQgetvalue(res, i, 2);

        char *esc_name = util_json_escape(uname);
        if (!esc_name) esc_name = strdup("");

        // estimate needed
        int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"status\": \"%s\"}", uid, esc_name, status);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc_name); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"user_id\": %s, \"username\": \"%s\", \"status\": \"%s\"}", uid, esc_name, status);

        free(esc_name);
    }

    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp;
        cap = used + 2;
    }
    out[used++] = ']';
    out[used] = '\0';

    *result_json = out;
    PQclear(res);
    return 0;
}

int dao_friends_update_status(int64_t user_id, int64_t friend_id, friend_status_t status) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE friends SET status = $3, updated_at = NOW() "
        "WHERE user_id = $1 AND friend_id = $2;";

    char buf1[32], buf2[32];
    snprintf(buf1, sizeof(buf1), "%ld", user_id);
    snprintf(buf2, sizeof(buf2), "%ld", friend_id);

    const char *params[3] = { buf1, buf2, friend_status_to_str(status) };

    PGresult *res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_FRIENDS] update_status error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}
