#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "db.h"
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

    // TODO: build JSON array
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
