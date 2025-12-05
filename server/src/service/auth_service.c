// server/src/service/auth_service.c
#include <stdio.h>
#include "dao/dao_users.h"
#include "dao/dao_sessions.h"
#include "service/auth_service.h"
#include "service/commands.h"
#include "service/protocol.h"
#include "utils/json.h"
#include <stdlib.h>
#include <string.h>

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

// Very small dispatcher implementation.
void auth_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)payload_len;
    switch (cmd) {
        case CMD_REQ_REGISTER: {
            char *username = util_json_get_string(payload, "username");
            char *password = util_json_get_string(payload, "password");
            if (!username || !password) {
                protocol_send_error(sess, CMD_RES_REGISTER, "INVALID_PAYLOAD");
            } else {
                AuthResult r = auth_signup(username, password);
                if (r == AUTH_OK) protocol_send_simple_ok(sess, CMD_RES_REGISTER);
                else if (r == AUTH_ERR_EXIST) protocol_send_error(sess, CMD_RES_REGISTER, "USERNAME_EXISTS");
                else protocol_send_error(sess, CMD_RES_REGISTER, "REGISTER_FAILED");
            }
            free(username); free(password);
        } break;

        case CMD_REQ_LOGIN: {
            char *username = util_json_get_string(payload, "username");
            char *password = util_json_get_string(payload, "password");
            if (!username || !password) {
                protocol_send_error(sess, CMD_RES_LOGIN, "INVALID_PAYLOAD");
            } else {
                UserSession us;
                AuthResult r = auth_login(username, password, &us);
                if (r == AUTH_OK) {
                    // attach to session
                    sess->user_id = us.user_id;
                    strncpy(sess->access_token, us.access_token, sizeof(sess->access_token)-1);
                    // reply with token
                    char buf[128];
                    int n = snprintf(buf, sizeof(buf), "{\"token\": \"%s\"}", us.access_token);
                    protocol_send_response(sess, CMD_RES_LOGIN, buf, (uint32_t)n);
                } else {
                    protocol_send_error(sess, CMD_RES_LOGIN, "LOGIN_FAILED");
                }
            }
            free(username); free(password);
        } break;

        default:
            protocol_send_error(sess, cmd, "UNKNOWN_AUTH_CMD");
    }
}
