// server/src/service/auth_service.c
#include <stdio.h>
#include "dao/dao_users.h"
#include "dao/dao_sessions.h"
#include "service/auth_service.h"

#define SESSION_TTL_SECONDS 3600

AuthResult auth_signup(const char *username, const char *password) {
    int64_t user_id;
    int rc = dao_users_create(username, password, &user_id);
    if (rc == -2) {
        return AUTH_ERR_EXIST;
    }
    if (rc != 0) {
        return AUTH_ERR_DB;
    }
    printf("[AUTH] Signup OK: user_id=%lld\n", (long long)user_id);
    return AUTH_OK;
}

AuthResult auth_login(const char *username, const char *password,
                      UserSession *out_session) {
    int64_t user_id = 0;
    int rc = dao_users_check_password(username, password, &user_id);
    if (rc < 0) {
        return AUTH_ERR_DB;
    }
    if (rc == 0) {
        return AUTH_ERR_CRED;
    }

    if (dao_sessions_create(user_id, SESSION_TTL_SECONDS, out_session) != 0) {
        return AUTH_ERR_DB;
    }

    printf("[AUTH] Login OK: user_id=%lld, token=%s\n",
           (long long)user_id, out_session->access_token);
    return AUTH_OK;
}
