// dao_users.c
#include "..\include\dao\dao_users.h"
#include "..\include\db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dao_users_init() {
    // insert user
    if (!db_prepare(
            "users_insert",
            "INSERT INTO users (username, password, avatar_img) "
            "VALUES ($1, $2, $3) RETURNING user_id",
            3)) return 0;

    // get by username
    if (!db_prepare(
            "users_get_by_username",
            "SELECT user_id, username, password, COALESCE(avatar_img, '') "
            "FROM users WHERE username = $1",
            1)) return 0;

    // get by id
    if (!db_prepare(
            "users_get_by_id",
            "SELECT user_id, username, password, COALESCE(avatar_img, '') "
            "FROM users WHERE user_id = $1",
            1)) return 0;

    // ensure user_stats
    if (!db_prepare(
            "user_stats_ensure",
            "INSERT INTO user_stats (user_id) "
            "VALUES ($1) "
            "ON CONFLICT (user_id) DO NOTHING",
            1)) return 0;

    // get user_stats
    if (!db_prepare(
            "user_stats_get",
            "SELECT user_id, quickmode_games, onevn_games, "
            "       quickmode_wins, onevn_wins "
            "FROM user_stats WHERE user_id = $1",
            1)) return 0;

    // inc quickmode stats
    if (!db_prepare(
            "user_stats_inc_quickmode",
            "UPDATE user_stats "
            "SET quickmode_games = quickmode_games + 1, "
            "    quickmode_wins  = quickmode_wins  + $2 "
            "WHERE user_id = $1",
            2)) return 0;

    // inc onevn stats
    if (!db_prepare(
            "user_stats_inc_onevn",
            "UPDATE user_stats "
            "SET onevn_games = onevn_games + 1, "
            "    onevn_wins  = onevn_wins  + $2 "
            "WHERE user_id = $1",
            2)) return 0;

    return 1;
}

int64_t dao_users_create(const char *username, const char *password_hash, const char *avatar_img) {
    const char *params[3];
    params[0] = username;
    params[1] = password_hash;
    params[2] = avatar_img ? avatar_img : "";

    PGresult *res = db_exec_params("users_insert", 3, params);
    if (!res) return 0;

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
        db_print_error(res);
        PQclear(res);
        return 0;
    }

    int64_t user_id = db_get_int64(res, 0, 0);
    PQclear(res);

    // đảm bảo có user_stats
    dao_user_stats_ensure(user_id);

    return user_id;
}

static int fill_user_row(PGresult *res, int row, UserRow *out) {
    if (!out) return 0;
    out->user_id = db_get_int64(res, row, 0);
    snprintf(out->username,   sizeof(out->username),   "%s", db_get_text(res, row, 1));
    snprintf(out->password,   sizeof(out->password),   "%s", db_get_text(res, row, 2));
    snprintf(out->avatar_img, sizeof(out->avatar_img), "%s", db_get_text(res, row, 3));
    return 1;
}

int dao_users_get_by_username(const char *username, UserRow *out) {
    const char *params[1] = { username };

    PGresult *res = db_exec_params("users_get_by_username", 1, params);
    if (!res) return -1;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_print_error(res);
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    if (rows == 0) {
        PQclear(res);
        return 0; // not found
    }

    fill_user_row(res, 0, out);
    PQclear(res);
    return 1;
}

int dao_users_get_by_id(int64_t user_id, UserRow *out) {
    char buf_id[32];
    snprintf(buf_id, sizeof(buf_id), "%lld", (long long)user_id);

    const char *params[1] = { buf_id };

    PGresult *res = db_exec_params("users_get_by_id", 1, params);
    if (!res) return -1;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_print_error(res);
        PQclear(res);
        return -1;
    }

    int rows = PQntuples(res);
    if (rows == 0) {
        PQclear(res);
        return 0;
    }

    fill_user_row(res, 0, out);
    PQclear(res);
    return 1;
}

int dao_user_stats_ensure(int64_t user_id) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)user_id);
    const char *params[1] = { buf };

    PGresult *res = db_exec_params("user_stats_ensure", 1, params);
    if (!res) return 0;

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_print_error(res);
        PQclear(res);
        return 0;
    }

    PQclear(res);
    return 1;
}

int dao_user_stats_get(int64_t user_id, UserStatsRow *out) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)user_id);
    const char *params[1] = { buf };

    PGresult *res = db_exec_params("user_stats_get", 1, params);
    if (!res) return -1;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_print_error(res);
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        return 0;
    }

    if (out) {
        out->user_id         = db_get_int64(res, 0, 0);
        out->quickmode_games = db_get_int64(res, 0, 1);
        out->onevn_games     = db_get_int64(res, 0, 2);
        out->quickmode_wins  = db_get_int64(res, 0, 3);
        out->onevn_wins      = db_get_int64(res, 0, 4);
    }

    PQclear(res);
    return 1;
}

int dao_user_stats_inc_quickmode(int64_t user_id, int win) {
    char buf_id[32], buf_win[8];
    snprintf(buf_id, sizeof(buf_id), "%lld", (long long)user_id);
    snprintf(buf_win, sizeof(buf_win), "%d", win ? 1 : 0);

    const char *params[2] = { buf_id, buf_win };

    PGresult *res = db_exec_params("user_stats_inc_quickmode", 2, params);
    if (!res) return 0;

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_print_error(res);
        PQclear(res);
        return 0;
    }
    PQclear(res);
    return 1;
}

int dao_user_stats_inc_onevn(int64_t user_id, int win) {
    char buf_id[32], buf_win[8];
    snprintf(buf_id, sizeof(buf_id), "%lld", (long long)user_id);
    snprintf(buf_win, sizeof(buf_win), "%d", win ? 1 : 0);

    const char *params[2] = { buf_id, buf_win };

    PGresult *res = db_exec_params("user_stats_inc_onevn", 2, params);
    if (!res) return 0;

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_print_error(res);
        PQclear(res);
        return 0;
    }
    PQclear(res);
    return 1;
}
