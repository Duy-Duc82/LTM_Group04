#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "db.h"
#include "utils/json.h"
#include "dao/dao_rooms.h"

static const char *room_mode_to_str(room_mode_t m) {
    switch (m) {
        case ROOM_MODE_BASIC: return "BASIC";
        case ROOM_MODE_ONEVN: return "ONEVN";
        default: return "BASIC";
    }
}

static const char *room_status_to_str(room_status_t s) {
    switch (s) {
        case ROOM_STATUS_WAITING:  return "WAITING";
        case ROOM_STATUS_PLAYING:  return "PLAYING";
        case ROOM_STATUS_FINISHED: return "FINISHED";
        default: return "WAITING";
    }
}

int dao_rooms_create(int64_t owner_id, room_mode_t mode, int64_t *out_room_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO room (owner_id, mode, status) "
        "VALUES ($1, $2, 'WAITING') RETURNING room_id;";

    char buf_owner[32];
    snprintf(buf_owner, sizeof(buf_owner), "%ld", owner_id);

    const char *params[2] = { buf_owner, room_mode_to_str(mode) };

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ROOMS] create error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 1) {
        *out_room_id = atoll(PQgetvalue(res, 0, 0));
    } else {
        PQclear(res);
        return -1;
    }
    PQclear(res);

    // Thêm owner vào room_members
    int rc = dao_rooms_join(*out_room_id, owner_id, 1);
    return rc;
}

int dao_rooms_join(int64_t room_id, int64_t user_id, int is_owner) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;
    const char *sql =
        "INSERT INTO room_members (room_id, user_id, role) "
        "VALUES ($1, $2, $3) "
        "ON CONFLICT (room_id, user_id) DO NOTHING;";

    char buf_room[32], buf_user[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    snprintf(buf_user, sizeof(buf_user), "%ld", user_id);

    const char *role = is_owner ? "OWNER" : "PLAYER";
    const char *params[3] = { buf_room, buf_user, role };

    PGresult *res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ROOMS] join error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_rooms_leave(int64_t room_id, int64_t user_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql = "DELETE FROM room_members WHERE room_id=$1 AND user_id=$2;";

    char buf_room[32], buf_user[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    snprintf(buf_user, sizeof(buf_user), "%ld", user_id);

    const char *params[2] = { buf_room, buf_user };

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ROOMS] leave error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_rooms_get_members(int64_t room_id, void **result_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT rm.user_id, u.username, rm.role "
        "FROM room_members rm JOIN users u ON u.user_id = rm.user_id "
        "WHERE rm.room_id = $1;";

    char buf_room[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    const char *params[1] = { buf_room };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ROOMS] get_members error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    size_t cap = 256; size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';

    for (int i = 0; i < rows; ++i) {
        const char *uid = PQgetvalue(res, i, 0);
        const char *uname = PQgetvalue(res, i, 1);
        const char *role = PQgetvalue(res, i, 2);

        char *esc = util_json_escape(uname);
        if (!esc) esc = strdup("");

        int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"role\": \"%s\"}", uid, esc, role);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"user_id\": %s, \"username\": \"%s\", \"role\": \"%s\"}", uid, esc, role);

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

int dao_rooms_update_status(int64_t room_id, room_status_t status) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql = "UPDATE rooms SET status = $2 WHERE room_id = $1;";

    char buf_room[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    const char *params[2] = { buf_room, room_status_to_str(status) };

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ROOMS] update_status error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}
