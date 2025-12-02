#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include "db.h"
#include "dao/dao_stats.h"
#include "utils/json.h"

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

    // build JSON object
    const char *uid = PQgetvalue(res, 0, 0);
    const char *uname = PQgetvalue(res, 0, 1);
    const char *qm_games = PQgetvalue(res, 0, 2);
    const char *ov_games = PQgetvalue(res, 0, 3);
    const char *qm_wins = PQgetvalue(res, 0, 4);
    const char *ov_wins = PQgetvalue(res, 0, 5);

    char *esc_name = util_json_escape(uname);
    if (!esc_name) esc_name = strdup("");

    int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"quickmode_games\": %s, \"onevn_games\": %s, \"quickmode_wins\": %s, \"onevn_wins\": %s}", uid, esc_name, qm_games, ov_games, qm_wins, ov_wins) + 1;
    char *out = malloc((size_t)need);
    if (!out) { free(esc_name); PQclear(res); return -1; }
    snprintf(out, need, "{\"user_id\": %s, \"username\": \"%s\", \"quickmode_games\": %s, \"onevn_games\": %s, \"quickmode_wins\": %s, \"onevn_wins\": %s}", uid, esc_name, qm_games, ov_games, qm_wins, ov_wins);

    *json_profile = out;
    free(esc_name);
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

    int rows = PQntuples(res);
    size_t cap = 256; size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';

    for (int i = 0; i < rows; ++i) {
        const char *uid = PQgetvalue(res, i, 0);
        const char *uname = PQgetvalue(res, i, 1);
        const char *wins = PQgetvalue(res, i, 2);
        char *esc = util_json_escape(uname);
        if (!esc) esc = strdup("");

        int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"total_wins\": %s}", uid, esc, wins);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc); PQclear(res); return -1; }
            out = tmp;
        }
        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"user_id\": %s, \"username\": \"%s\", \"total_wins\": %s}", uid, esc, wins);
        free(esc);
    }
    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp; cap = used + 2;
    }
    out[used++] = ']'; out[used] = '\0';
    *json_leaderboard = out;
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

    int rows = PQntuples(res);
    size_t cap = 512; size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';
    for (int i = 0; i < rows; ++i) {
        const char *match_id = PQgetvalue(res, i, 0);
        const char *mode = PQgetvalue(res, i, 1);
        const char *score = PQgetvalue(res, i, 2);
        const char *is_win = PQgetvalue(res, i, 3);
        const char *total_correct = PQgetvalue(res, i, 4);
        const char *avg_ms = PQgetvalue(res, i, 5);
        const char *played_at = PQgetvalue(res, i, 6);

        char *esc_mode = util_json_escape(mode);
        if (!esc_mode) esc_mode = strdup("");

        int need = snprintf(NULL, 0, "{\"match_id\": %s, \"mode\": \"%s\", \"score\": %s, \"is_win\": %s, \"total_correct\": %s, \"avg_answer_ms\": %s, \"played_at\": \"%s\"}", match_id, esc_mode, score, is_win, total_correct, avg_ms, played_at);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc_mode); PQclear(res); return -1; }
            out = tmp;
        }

        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, "{\"match_id\": %s, \"mode\": \"%s\", \"score\": %s, \"is_win\": %s, \"total_correct\": %s, \"avg_answer_ms\": %s, \"played_at\": \"%s\"}", match_id, esc_mode, score, is_win, total_correct, avg_ms, played_at);
        free(esc_mode);
    }
    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp; cap = used + 2;
    }
    out[used++] = ']'; out[used] = '\0';
    *json_history = out;
    PQclear(res);
    return 0;
}
