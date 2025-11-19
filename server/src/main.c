// main.c
#include <stdio.h>
#include <time.h>
#include "..\include\dao\dao_users.h"
#include "..\include\db.h"
#include "..\include\dao\dao_sessions.h"

static void iso_time_plus_hours(char *buf, size_t sz, int hours) {
    time_t now = time(NULL);
    now += hours * 3600;
    struct tm t;
    gmtime_r(&now, &t); // hoặc gmtime_s trên Windows

    // format ISO 8601 cho Postgres: 'YYYY-MM-DD HH:MM:SS+00'
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S+00", &t);
}

int main() {
    // chỉnh conninfo theo DB của bạn
    const char *conninfo =
        "host=localhost port=5432 dbname=millionaire user=postgres password=123";

    if (!db_init(conninfo)) {
        return 1;
    }

    if (!dao_users_init() || !dao_sessions_init()) {
        fprintf(stderr, "Failed to init DAO.\n");
        db_close();
        return 1;
    }

    printf("=== Creating user ===\n");
    int64_t user_id = dao_users_create("test_user", "hashed_password_here", NULL);
    if (user_id == 0) {
        printf("Create user failed (có thể username đã tồn tại).\n");
    } else {
        printf("Created user_id = %lld\n", (long long)user_id);
    }

    printf("=== Getting user by username ===\n");
    UserRow u;
    int r = dao_users_get_by_username("test_user", &u);
    if (r == 1) {
        printf("Found user: id=%lld, username=%s\n",
               (long long)u.user_id, u.username);
    } else if (r == 0) {
        printf("User not found.\n");
    } else {
        printf("Error querying user.\n");
    }

    dao_user_stats_ensure(u.user_id);
    dao_user_stats_inc_quickmode(u.user_id, 1);

    UserStatsRow st;
    if (dao_user_stats_get(u.user_id, &st) == 1) {
        printf("Stats: quick_games=%lld, quick_wins=%lld\n",
               (long long)st.quickmode_games,
               (long long)st.quickmode_wins);
    }

    printf("=== Creating session ===\n");
    char expires_at[64];
    iso_time_plus_hours(expires_at, sizeof(expires_at), 2); // hết hạn sau 2h

    const char *token = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcd"; // 64 ký tự
    int64_t sess_id = dao_session_create(u.user_id, token, expires_at);
    printf("Session id = %lld\n", (long long)sess_id);

    int valid = dao_session_validate(u.user_id, token);
    printf("Validate token: %d\n", valid);

    dao_session_heartbeat(u.user_id, token);
    dao_session_delete(u.user_id, token);

    db_close();
    return 0;
}
