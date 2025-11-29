// server/src/test/test_auth.c
#include <stdio.h>
#include <stdlib.h>
#include "../include/db.h"
#include "service/auth_service.h"

int main(void) {
    const char *conninfo = getenv("DB_CONN");
    if (!conninfo) {
        fprintf(stderr, "Set DB_CONN env first\n");
        return 1;
    }
    if (db_connect(conninfo) != 0) return 1;

    UserSession sess;
    AuthResult r;

    printf("== TEST SIGNUP ==\n");
    r = auth_signup("alice", "123");
    printf("Signup result: %d\n", r);

    printf("== TEST LOGIN ==\n");
    r = auth_login("alice", "123", &sess);
    if (r == AUTH_OK) {
        printf("Login OK, token=%s\n", sess.access_token);
    } else {
        printf("Login FAIL, code=%d\n", r);
    }

    db_disconnect();
    return 0;
}
