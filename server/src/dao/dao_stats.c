#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "dao/dao_stats.h"
#include "utils/json.h"

int dao_stats_get_profile(int64_t user_id, void **json_profile) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT user_id, username, "
        "       COALESCE(avatar_img, '') as avatar_img, "
        "       quickmode_games, "
        "       onevn_games, "
        "       quickmode_wins, "
        "       onevn_wins "
        "FROM users "
        "WHERE user_id = $1;";

    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", user_id);
    const char *params[1] = { buf };

    PGresult *res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "[DAO_STATS] get_profile query error: %s\n", PQerrorMessage(conn));
        fprintf(stderr, "[DAO_STATS] SQL: %s\n", sql);
        fprintf(stderr, "[DAO_STATS] user_id param: %s\n", buf);
        PQclear(res);
        return -1;
    }

    int row_count = PQntuples(res);
    if (row_count != 1) {
        fprintf(stderr, "[DAO_STATS] get_profile: Expected 1 row, got %d rows for user_id: %ld\n", row_count, user_id);
        PQclear(res);
        return -1;
    }

    // build JSON object
    const char *uid = PQgetvalue(res, 0, 0);
    const char *uname = PQgetvalue(res, 0, 1);
    const char *avatar = PQgetvalue(res, 0, 2);
    const char *qm_games = PQgetvalue(res, 0, 3);
    const char *ov_games = PQgetvalue(res, 0, 4);
    const char *qm_wins = PQgetvalue(res, 0, 5);
    const char *ov_wins = PQgetvalue(res, 0, 6);

    char *esc_name = util_json_escape(uname);
    char *esc_avatar = util_json_escape(avatar);
    if (!esc_name) esc_name = strdup("");
    if (!esc_avatar) esc_avatar = strdup("");

    int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"avatar_img\": \"%s\", \"quickmode_games\": %s, \"onevn_games\": %s, \"quickmode_wins\": %s, \"onevn_wins\": %s}", uid, esc_name, esc_avatar, qm_games, ov_games, qm_wins, ov_wins) + 1;
    char *out = malloc((size_t)need);
    if (!out) { free(esc_name); free(esc_avatar); PQclear(res); return -1; }
    snprintf(out, need, "{\"user_id\": %s, \"username\": \"%s\", \"avatar_img\": \"%s\", \"quickmode_games\": %s, \"onevn_games\": %s, \"quickmode_wins\": %s, \"onevn_wins\": %s}", uid, esc_name, esc_avatar, qm_games, ov_games, qm_wins, ov_wins);

    *json_profile = out;
    free(esc_name);
    free(esc_avatar);
    PQclear(res);
    return 0;
}

int dao_stats_get_leaderboard(int limit, void **json_leaderboard) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;

    const char *sql =
        "SELECT user_id, username, "
        "       (quickmode_wins + onevn_wins) AS total_wins "
        "FROM users "
        "WHERE (quickmode_wins + onevn_wins) >= 0 "
        "ORDER BY total_wins DESC, username ASC "
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

int dao_stats_update_quickmode_game(int64_t user_id, int is_win) {
    PGconn *conn = db_get_conn();
    if (!conn) return -1;
    
    // Update quickmode stats directly in users table
    const char *sql_update =
        "UPDATE users SET "
        "quickmode_games = quickmode_games + 1, "
        "quickmode_wins = quickmode_wins + $2 "
        "WHERE user_id = $1;";
    
    char buf_user[32], buf_win[32];
    snprintf(buf_user, sizeof(buf_user), "%ld", user_id);
    snprintf(buf_win, sizeof(buf_win), "%d", is_win);
    const char *params[2] = { buf_user, buf_win };
    
    PGresult *res = PQexecParams(conn, sql_update, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DAO_STATS] update_quickmode_game error for user %ld: %s\n", 
                user_id, PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    PQclear(res);
    return 0;
}

int dao_stats_update_onevn_game(int64_t winner_id, int64_t *player_ids, int *player_scores, int *player_eliminated, int player_count) {
    PGconn *conn = db_get_conn();
    if (!conn || !player_ids || !player_scores || !player_eliminated || player_count <= 0) return -1;
    
    // Update stats for all players
    for (int i = 0; i < player_count; i++) {
        int64_t player_id = player_ids[i];
        int is_winner = (player_id == winner_id && winner_id > 0) ? 1 : 0;
        
        // Increment onevn_games for all players (update directly in users table)
        const char *sql_update =
            "UPDATE users SET "
            "onevn_games = onevn_games + 1, "
            "onevn_wins = onevn_wins + $2 "
            "WHERE user_id = $1;";
        
        char buf_user[32], buf_winner[32];
        snprintf(buf_user, sizeof(buf_user), "%ld", player_id);
        snprintf(buf_winner, sizeof(buf_winner), "%d", is_winner);
        const char *params[2] = { buf_user, buf_winner };
        
        PGresult *res = PQexecParams(conn, sql_update, 2, NULL, params, NULL, NULL, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "[DAO_STATS] update_onevn_game error for user %ld: %s\n", 
                    player_id, PQerrorMessage(conn));
            PQclear(res);
            // Continue with other players
            continue;
        }
        PQclear(res);
    }
    
    return 0;
}