// server/include/service/auth_service.h
#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <stdint.h>
#include "dao/dao_sessions.h"

typedef enum {
    AUTH_OK = 0,
    AUTH_ERR_DB   = -1,
    AUTH_ERR_EXIST= -2,
    AUTH_ERR_CRED = -3
} AuthResult;

AuthResult auth_signup(const char *username, const char *password);
AuthResult auth_login(const char *username, const char *password,
                      UserSession *out_session);

#endif
