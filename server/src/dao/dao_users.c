// server/src/dao/dao_users.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "../include/db.h"
#include "dao/dao_users.h"
#include "utils/crypto.h"
#include "utils/json.h"

int dao_users_create(const char *username, const char *password, int64_t *out_user_id) {
    if (!db_is_ok()) return -1;

    const char *sql =
        "INSERT INTO users (username, password) "
        "VALUES ($1, $2) RETURNING user_id;";

    // Hash password before storing
    char hashed_password[128];
    if (util_password_hash(password, hashed_password, sizeof(hashed_password)) != 0) {
        fprintf(stderr, "dao_users_create: failed to hash password\n");
        return -1;
    }

    const char *params[2] = { username, hashed_password };
    int paramLengths[2]   = { (int)strlen(username), (int)strlen(hashed_password) };
    int paramFormats[2]   = { 0, 0 };

    PGresult *res = PQexecParams(db_conn,
                                 sql,
                                 2,
                                 NULL,
                                 params,
                                 paramLengths,
                                 paramFormats,
                                 0); // text

    if (PQresultStatus(res) == PGRES_FATAL_ERROR) {
        // check trùng username (unique constraint)
        const char *msg = PQerrorMessage(db_conn);
        if (strstr(msg, "unique") && strstr(msg, "users_username_key")) {
            PQclear(res);
            return -2;
        }
        db_log_error(res, "dao_users_create failed");
        return -1;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_log_error(res, "dao_users_create unexpected status");
        return -1;
    }

    if (PQntuples(res) != 1) {
        db_log_error(res, "dao_users_create no row returned");
        return -1;
    }

    char *idstr = PQgetvalue(res, 0, 0);
    if (out_user_id) {
        *out_user_id = atoll(idstr);
    }

    PQclear(res);
    return 0;
}

int dao_users_find_by_username(const char *username, User *out_user) {
    if (!db_is_ok()) return -1;

    const char *sql =
        "SELECT user_id, username, password, COALESCE(avatar_img,''), "
        "       quickmode_games, quickmode_wins, onevn_games, onevn_wins "
        "FROM users WHERE username = $1;";

    const char *params[1] = { username };
    int paramLengths[1]   = { (int)strlen(username) };
    int paramFormats[1]   = { 0 };

    PGresult *res = PQexecParams(db_conn,
                                 sql,
                                 1,
                                 NULL,
                                 params,
                                 paramLengths,
                                 paramFormats,
                                 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_log_error(res, "dao_users_find_by_username failed");
        return -1;
    }

    int n = PQntuples(res);
    if (n == 0) {
        PQclear(res);
        return -1; // not found
    }

    if (out_user) {
        memset(out_user, 0, sizeof(*out_user));
        out_user->user_id = atoll(PQgetvalue(res, 0, 0));
        strncpy(out_user->username, PQgetvalue(res, 0, 1), 32);
        strncpy(out_user->password, PQgetvalue(res, 0, 2), 255);
        strncpy(out_user->avatar_img, PQgetvalue(res, 0, 3), 512);
    }

    PQclear(res);
    return 0;
}

int dao_users_find_by_id(int64_t user_id, User *out_user) {
    if (!db_is_ok()) return -1;

    const char *sql =
        "SELECT user_id, username, password, COALESCE(avatar_img,''), "
        "       quickmode_games, quickmode_wins, onevn_games, onevn_wins "
        "FROM users WHERE user_id = $1;";

    char idbuf[32];
    snprintf(idbuf, sizeof(idbuf), "%lld", (long long)user_id);
    const char *params[1] = { idbuf };
    int paramLengths[1] = { (int)strlen(idbuf) };
    int paramFormats[1] = { 0 };

    PGresult *res = PQexecParams(db_conn,
                                 sql,
                                 1,
                                 NULL,
                                 params,
                                 paramLengths,
                                 paramFormats,
                                 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_log_error(res, "dao_users_find_by_id failed");
        return -1;
    }

    int n = PQntuples(res);
    if (n == 0) {
        PQclear(res);
        return -1; // not found
    }

    if (out_user) {
        memset(out_user, 0, sizeof(*out_user));
        out_user->user_id = atoll(PQgetvalue(res, 0, 0));
        strncpy(out_user->username, PQgetvalue(res, 0, 1), 32);
        strncpy(out_user->password, PQgetvalue(res, 0, 2), 255);
        strncpy(out_user->avatar_img, PQgetvalue(res, 0, 3), 512);
    }

    PQclear(res);
    return 0;
}

int dao_users_check_password(const char *username, const char *password, int64_t *out_user_id) {
    User u;
    if (dao_users_find_by_username(username, &u) != 0) return 0; // not found

    // If stored password contains salt (format salt$hash) we verify with util_password_verify
    if (strchr(u.password, '$')) {
        int ok = util_password_verify(password, u.password);
        if (ok == 1) {
            if (out_user_id) *out_user_id = u.user_id;
            return 1;
        }
        return 0;
    }

    // legacy plaintext
    if (strcmp(u.password, password) == 0) {
        if (out_user_id) *out_user_id = u.user_id;
        return 1;
    }
    return 0;
}

int dao_users_search_by_username(const char *query, int limit, void **result_json) {
    if (!db_is_ok() || !query || !result_json) return -1;
    if (limit <= 0 || limit > 100) limit = 20;
    
    const char *sql =
        "SELECT user_id, username, COALESCE(avatar_img, '') as avatar_img "
        "FROM users "
        "WHERE username ILIKE $1 "
        "ORDER BY username "
        "LIMIT $2;";
    
    // Build search pattern: %query%
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "%%%s%%", query);
    
    char limit_buf[32];
    snprintf(limit_buf, sizeof(limit_buf), "%d", limit);
    
    const char *params[2] = { pattern, limit_buf };
    int paramLengths[2] = { (int)strlen(pattern), (int)strlen(limit_buf) };
    int paramFormats[2] = { 0, 0 };
    
    PGresult *res = PQexecParams(db_conn, sql, 2, NULL, params, paramLengths, paramFormats, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        db_log_error(res, "dao_users_search_by_username failed");
        return -1;
    }
    
    int rows = PQntuples(res);
    size_t cap = 512;
    size_t used = 0;
    char *out = malloc(cap);
    if (!out) { PQclear(res); return -1; }
    out[used++] = '[';
    
    for (int i = 0; i < rows; i++) {
        const char *uid = PQgetvalue(res, i, 0);
        const char *uname = PQgetvalue(res, i, 1);
        const char *avatar = PQgetvalue(res, i, 2);
        
        char *esc_name = util_json_escape(uname);
        char *esc_avatar = util_json_escape(avatar);
        if (!esc_name) esc_name = strdup("");
        if (!esc_avatar) esc_avatar = strdup("");
        
        int need = snprintf(NULL, 0, "{\"user_id\": %s, \"username\": \"%s\", \"avatar_img\": \"%s\"}", 
                          uid, esc_name, esc_avatar);
        if (used + (size_t)need + 3 >= cap) {
            cap = (used + (size_t)need + 3) * 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); free(esc_name); free(esc_avatar); PQclear(res); return -1; }
            out = tmp;
        }
        
        if (i > 0) out[used++] = ',';
        used += snprintf(out + used, cap - used, 
                        "{\"user_id\": %s, \"username\": \"%s\", \"avatar_img\": \"%s\"}", 
                        uid, esc_name, esc_avatar);
        
        free(esc_name);
        free(esc_avatar);
    }
    
    if (used + 2 >= cap) {
        char *tmp = realloc(out, used + 2);
        if (!tmp) { free(out); PQclear(res); return -1; }
        out = tmp;
    }
    out[used++] = ']';
    out[used] = '\0';
    
    *result_json = out;
    PQclear(res);
    return 0;
}

int dao_users_update_avatar(int64_t user_id, const char *avatar_path) {
    if (!db_is_ok() || !avatar_path) return -1;

    const char *sql = "UPDATE users SET avatar_img = $1 WHERE user_id = $2;";

    char idbuf[32];
    snprintf(idbuf, sizeof(idbuf), "%lld", (long long)user_id);
    const char *params[2] = { avatar_path, idbuf };
    int paramLengths[2] = { (int)strlen(avatar_path), (int)strlen(idbuf) };
    int paramFormats[2] = { 0, 0 };

    PGresult *res = PQexecParams(db_conn, sql, 2, NULL, params, paramLengths, paramFormats, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        db_log_error(res, "dao_users_update_avatar failed");
        PQclear(res);
        return -1;
    }

    PQclear(res);
    return 0;
}