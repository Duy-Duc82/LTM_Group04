#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "db.h"
#include "dao/dao_stats.h"

int dao_stats_get_profile(int64_t user_id, void **json_profile) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT u.user_id, u.username, "
        "       s.quickmode_games, s.onevn_games, "
        "       s.quickmode_wins, s.onevn_wins "
        "FROM users u "
        "LEFT JOIN user_stats s ON s.user_id = u.user_id "
        "WHERE u.user_id = $1;";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_STATS] get_profile error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) != 1) {
        PQclear(res);
        return -1;
    }

    // TODO: build JSON object
    PQclear(res);
    return 0;
}

int dao_stats_get_leaderboard(int limit, void **json_leaderboard) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT u.user_id, u.username, s.quickmode_wins + s.onevn_wins AS total_wins "
        "FROM user_stats s "
        "JOIN users u ON u.user_id = s.user_id "
        "ORDER BY total_wins DESC "
        "LIMIT $1;";

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", limit);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_STATS] get_leaderboard error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    // TODO: build JSON array
    PQclear(res);
    return 0;
}

int dao_stats_get_match_history(int64_t user_id, void **json_history) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT match_id, mode, score, is_win, total_correct, avg_answer_ms, played_at "
        "FROM match_history "
        "WHERE user_id = $1 "
        "ORDER BY played_at DESC "
        "LIMIT 50;";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_STATS] get_match_history error: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    // TODO: build JSON array
    PQclear(res);
    return 0;
}
