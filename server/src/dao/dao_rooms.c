#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "utils/json.h"
#include "dao/dao_rooms.h"

static const char *room_status_to_str(room_status_t s) {
    switch (s) {
        case ROOM_STATUS_WAITING:      return "WAITING";
        case ROOM_STATUS_STARTING:     return "STARTING";
        case ROOM_STATUS_IN_PROGRESS:  return "IN_PROGRESS";
        case ROOM_STATUS_FINISHED:     return "FINISHED";
        default: return "WAITING";
    }
}

int dao_rooms_create(int64_t owner_id, int64_t *out_room_id) {
    // Use default config: 5 easy, 5 medium, 5 hard = 15 questions total
    return dao_rooms_create_with_config(owner_id, 5, 5, 5, out_room_id);
}

int dao_rooms_join(int64_t room_id, int64_t user_id, int is_owner) {
    (void)is_owner;  // Unused parameter (kept for compatibility)
    PGconn *conn = db_get_conn();
    if (!conn) return -1;
    const char *sql =
        "INSERT INTO room_members (room_id, user_id) "
        "VALUES ($1, $2) "
        "ON CONFLICT (room_id, user_id) DO NOTHING;";

    char buf_room[32], buf_user[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    snprintf(buf_user, sizeof(buf_user), "%ld", user_id);

    const char *params[2] = { buf_room, buf_user };

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
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
        "SELECT rm.user_id, u.username "
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

        char *esc = util_json_escape(uname);
        if (!esc) esc = strdup("");

        int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\"}", uid, esc);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"user_id\": %s, \"username\": \"%s\"}", uid, esc);

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

    const char *sql = "UPDATE room SET status = $2 WHERE room_id = $1;";

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

int dao_rooms_create_with_config(int64_t owner_id, int easy_count, int medium_count, int hard_count, int64_t *out_room_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO room (owner_id, status, easy_count, medium_count, hard_count) "
        "VALUES ($1, 'WAITING', $2, $3, $4) RETURNING room_id;";

    char buf_owner[32], buf_easy[32], buf_medium[32], buf_hard[32];
    snprintf(buf_owner, sizeof(buf_owner), "%ld", owner_id);
    snprintf(buf_easy, sizeof(buf_easy), "%d", easy_count);
    snprintf(buf_medium, sizeof(buf_medium), "%d", medium_count);
    snprintf(buf_hard, sizeof(buf_hard), "%d", hard_count);

    const char *params[4] = { buf_owner, buf_easy, buf_medium, buf_hard };

    PGresult *res = PQexecParams(conn, sql, 4, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ROOMS] create_with_config error: %s\n", PQerrorMessage(conn));
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

    // Add owner to room_members
    int rc = dao_rooms_join(*out_room_id, owner_id, 1);
    return rc;
}

int dao_rooms_get_config(int64_t room_id, int *easy_count, int *medium_count, int *hard_count) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT easy_count, medium_count, hard_count "
        "FROM room WHERE room_id = $1;";

    char buf_room[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    const char *params[1] = { buf_room };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ROOMS] get_config error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return -1;
    }

    if (easy_count) *easy_count = atoi(PQgetvalue(res, 0, 0));
    if (medium_count) *medium_count = atoi(PQgetvalue(res, 0, 1));
    if (hard_count) *hard_count = atoi(PQgetvalue(res, 0, 2));

    PQclear(res);
    return 0;
}

int dao_rooms_update_config(int64_t room_id, int easy_count, int medium_count, int hard_count) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE room SET easy_count = $2, medium_count = $3, hard_count = $4 "
        "WHERE room_id = $1;";

    char buf_room[32], buf_easy[32], buf_medium[32], buf_hard[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    snprintf(buf_easy, sizeof(buf_easy), "%d", easy_count);
    snprintf(buf_medium, sizeof(buf_medium), "%d", medium_count);
    snprintf(buf_hard, sizeof(buf_hard), "%d", hard_count);

    const char *params[4] = { buf_room, buf_easy, buf_medium, buf_hard };

    PGresult *res = PQexecParams(conn, sql, 4, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ROOMS] update_config error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_rooms_get_owner(int64_t room_id, int64_t *owner_id) {
    PGconn *conn = db_get_conn();
    if (!conn || !owner_id) return -1;

    const char *sql = "SELECT owner_id FROM room WHERE room_id = $1;";

    char buf_room[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    const char *params[1] = { buf_room };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ROOMS] get_owner error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return -1;
    }

    *owner_id = atoll(PQgetvalue(res, 0, 0));
    PQclear(res);
    return 0;
}
