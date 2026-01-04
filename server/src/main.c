// server/src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "db.h"
#include "service/auth_service.h"
#include "service/quickmode_service.h"
#include "service/server.h"
#include <string.h>

int main() {
    // Seed random number generator
    srand((unsigned int)time(NULL));

    const char *conn = getenv("DB_CONN");
    if (!conn) {
        printf("Please set DB_CONN environment variable!\n");
        return 1;
    }

    if (db_connect(conn) != 0) {
        printf("DB connection failed.\n");
        return 1;
    }

    printf("=== SERVER STARTED ===\n");

    // If running as server: start runtime loop
    const char *server_mode = getenv("SERVER_MODE");
    if (server_mode && strcmp(server_mode, "1") == 0) {
        const char *port = getenv("SERVER_PORT");
        if (!port) port = "9000";
        start_server(NULL, port);
        db_disconnect();
        return 0;
    }

    // Sample test: signup + login
    UserSession session;

    printf("SIGNUP TEST...\n");
    AuthResult r = auth_signup("bob", "pass");
    printf("Signup result = %d\n", r);

    printf("LOGIN TEST...\n");
    r = auth_login("bob", "pass", &session);

    if (r == AUTH_OK) {
        printf("Login OK, token = %s\n", session.access_token);
    } else {
        printf("Login failed (%d)\n", r);
    }

    // Test Quickmode Start
    printf("QUICKMODE TEST...\n");
    qm_debug_start(1); // ví dụ user_id = 1

    db_disconnect();
    return 0;
}
