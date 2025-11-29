// server/src/dao/dao_users.c
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "db.h"
#include "dao/dao_users.h"

int dao_users_create(const char *username, const char *password, int64_t *out_user_id) {
    if (!db_is_ok()) return -1;

    const char *sql =
        "INSERT INTO users (username, password) "
        "VALUES ($1, $2) RETURNING user_id;";

    const char *params[2] = { username, password };
    int paramLengths[2]   = { (int)strlen(username), (int)strlen(password) };
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
        "SELECT user_id, username, password, COALESCE(avatar_img,'') "
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

int dao_users_check_password(const char *username, const char *password, int64_t *out_user_id) {
    User u;
    if (dao_users_find_by_username(username, &u) != 0) return 0; // not found

    // TODO: nếu dùng hash, thay bằng hàm verify
    if (strcmp(u.password, password) == 0) {
        if (out_user_id) *out_user_id = u.user_id;
        return 1;
    }
    return 0;
}
