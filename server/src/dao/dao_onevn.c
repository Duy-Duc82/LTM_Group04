#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "dao/dao_onevn.h"

int dao_onevn_create_session(int64_t room_id, int64_t *out_session_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "INSERT INTO onevn_sessions (room_id, status, players) "
        "VALUES ($1, 'IN_PROGRESS', '[]'::jsonb) "
        "RETURNING session_id;";

    char buf_room[32];
    snprintf(buf_room, sizeof(buf_room), "%ld", room_id);
    const char *params[1] = { buf_room };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ONEVN] create_session error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 1) {
        *out_session_id = atoll(PQgetvalue(res, 0, 0));
    } else {
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_onevn_update_players(int64_t session_id, const char *players_json) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE onevn_sessions SET players = $2::jsonb "
        "WHERE session_id = $1;";

    char buf_session[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    const char *params[2] = { buf_session, players_json };

    PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_ONEVN] update_players error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_onevn_end_session(int64_t session_id, int64_t winner_id) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "UPDATE onevn_sessions SET status = 'FINISHED', winner_id = $2, ended_at = NOW() "
        "WHERE session_id = $1;";

    char buf_session[32], buf_winner[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    if (winner_id > 0) {
        snprintf(buf_winner, sizeof(buf_winner), "%ld", winner_id);
        const char *params[2] = { buf_session, buf_winner };
        PGresult *res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "[DAO_ONEVN] end_session error: %s\n", PQerrorMessage(conn));
            PQclear(res);
            return -1;
        }
        PQclear(res);
    } else {
        // No winner (aborted)
        const char *sql2 =
            "UPDATE onevn_sessions SET status = 'ABORTED', ended_at = NOW() "
            "WHERE session_id = $1;";
        const char *params[1] = { buf_session };
        PGresult *res = PQexecParams(conn, sql2, 1, NULL, params, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "[DAO_ONEVN] end_session error: %s\n", PQerrorMessage(conn));
            PQclear(res);
            return -1;
        }
        PQclear(res);
    }
    return 0;
}

int dao_onevn_get_session(int64_t session_id, int64_t *room_id, int64_t *winner_id, char *status, char *players_json, size_t json_len) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT room_id, winner_id, status, players::text "
        "FROM onevn_sessions WHERE session_id = $1;";

    char buf_session[32];
    snprintf(buf_session, sizeof(buf_session), "%ld", session_id);
    const char *params[1] = { buf_session };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_ONEVN] get_session error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return -1;
    }

    if (room_id) *room_id = atoll(PQgetvalue(res, 0, 0));
    const char *winner_str = PQgetvalue(res, 0, 1);
    if (winner_id) *winner_id = (winner_str && strlen(winner_str) > 0) ? atoll(winner_str) : 0;
    if (status) strncpy(status, PQgetvalue(res, 0, 2), 31);
    if (players_json) {
        const char *players_str = PQgetvalue(res, 0, 3);
        strncpy(players_json, players_str ? players_str : "[]", json_len - 1);
        players_json[json_len - 1] = '\0';
    }

    PQclear(res);
    return 0;
}

