// server/src/test/test_auth.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    printf("== TEST SIGNUP ==\n");
    r = auth_signup("bob", "1235");
    printf("Signup result: %d\n", r);

    // Interactive login test
    printf("== TEST LOGIN (interactive) ==\n");
    char userbuf[128];
    char passbuf[128];
    printf("Username: ");
    if (!fgets(userbuf, sizeof(userbuf), stdin)) {
        db_disconnect();
        return 1;
    }
    if (userbuf[strlen(userbuf)-1] == '\n') userbuf[strlen(userbuf)-1] = '\0';
    printf("Password: ");
    if (!fgets(passbuf, sizeof(passbuf), stdin)) {
        db_disconnect();
        return 1;
    }
    if (passbuf[strlen(passbuf)-1] == '\n') passbuf[strlen(passbuf)-1] = '\0';

    r = auth_login(userbuf, passbuf, &sess);
    if (r == AUTH_OK) {
        printf("Login OK, token=%s user_id=%lld\n", sess.access_token, (long long)sess.user_id);
    } else {
        printf("Login FAIL, code=%d\n", r);
    }

    db_disconnect();
    return 0;
}
