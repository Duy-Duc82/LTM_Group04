#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "db.h"
#include "utils/json.h"
#include "dao/dao_chat.h"

int dao_chat_send_dm(int64_t sender_id, int64_t receiver_id, const char *content) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO messages (sender_id, receiver_id, content) "
        "VALUES ($1, $2, $3);";

    char buf_sender[32], buf_receiver[32];
    snprintf(buf_sender, sizeof(buf_sender), "%ld", sender_id);
    snprintf(buf_receiver, sizeof(buf_receiver), "%ld", receiver_id);

    const char *params[3] = { buf_sender, buf_receiver, content };

    PGresult *res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_CHAT] send_dm error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_chat_send_room(int64_t sender_id, int64_t room_id, const char *content) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO messages (sender_id, room_id, content) "
        "VALUES ($1, $2, $3);";

    char buf_sender[32], buf_room[32];
    snprintf(buf_sender, sizeof(buf_sender), "%ld", sender_id);
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);

    const char *params[3] = { buf_sender, buf_room, content };

    PGresult *res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_CHAT] send_room error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_chat_fetch_offline(int64_t user_id, void **result_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT msg_id, sender_id, content, created_at "
        "FROM messages "
        "WHERE receiver_id = $1 AND is_read = FALSE "
        "ORDER BY created_at ASC;";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_CHAT] fetch_offline error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    size_t cap = 256; size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';

    for (int i = 0; i < rows; ++i) {
        const char *msg_id = PQgetvalue(res, i, 0);
        const char *sender = PQgetvalue(res, i, 1);
        const char *content = PQgetvalue(res, i, 2);
        const char *created = PQgetvalue(res, i, 3);

        char *esc = util_json_escape(content);
        if (!esc) esc = strdup("");

        int need = snprintf(NULL, 0, "{\"msg_id\": %s, \"sender_id\": %s, \"content\": \"%s\", \"created_at\": \"%s\"}", msg_id, sender, esc, created);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"msg_id\": %s, \"sender_id\": %s, \"content\": \"%s\", \"created_at\": \"%s\"}", msg_id, sender, esc, created);

        free(esc);
    }

    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp; cap = used + 2;
    }
    out[used++] = ']'; out[used] = '\0';

    *result_json = out;

    PQclear(res);
    return 0;
}

int dao_chat_fetch_offline_from_sender(int64_t receiver_id, int64_t sender_id, void **result_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT msg_id, sender_id, content, created_at "
        "FROM messages "
        "WHERE receiver_id = $1 AND sender_id = $2 AND is_read = FALSE "
        "ORDER BY created_at ASC;";

    char buf_recv[32], buf_sender[32];
    snprintf(buf_recv, sizeof(buf_recv), "%ld", receiver_id);
    snprintf(buf_sender, sizeof(buf_sender), "%ld", sender_id);
    const char *params[2] = { buf_recv, buf_sender };

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_CHAT] fetch_offline_from_sender error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    size_t cap = 256; size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';

    for (int i = 0; i < rows; ++i) {
        const char *msg_id = PQgetvalue(res, i, 0);
        const char *sender = PQgetvalue(res, i, 1);
        const char *content = PQgetvalue(res, i, 2);
        const char *created = PQgetvalue(res, i, 3);

        char *esc = util_json_escape(content);
        if (!esc) esc = strdup("");

        int need = snprintf(NULL, 0, "{\"msg_id\": %s, \"sender_id\": %s, \"content\": \"%s\", \"created_at\": \"%s\"}", msg_id, sender, esc, created);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"msg_id\": %s, \"sender_id\": %s, \"content\": \"%s\", \"created_at\": \"%s\"}", msg_id, sender, esc, created);

        free(esc);
    }

    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp; cap = used + 2;
    }
    out[used++] = ']'; out[used] = '\0';

    *result_json = out;

    PQclear(res);
    return 0;
}

int dao_chat_mark_read(int64_t user_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE messages SET is_read = TRUE "
        "WHERE receiver_id = $1 AND is_read = FALSE;";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_CHAT] mark_read error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}
