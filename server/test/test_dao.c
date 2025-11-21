// server/test/test_dao.c
#include <stdio.h>
#include <time.h>
#include "../include/db.h"
#include "../include/dao/dao_users.h"
#include "../include/dao/dao_sessions.h"

// Hàm tiện ích tạo expires_at dạng ISO 8601
static void make_expires_in(char *buf, size_t sz, int hours) {
    time_t now = time(NULL) + hours * 3600;
    struct tm *t = gmtime(&now);
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S+00", t);
}

int main() {
    const char *conninfo =
        "host=localhost port=5432 dbname=millionaire user=postgres password=123";

    printf("Connecting to DB...\n");
    if (!db_init(conninfo)) return 1;

    dao_users_init();
    dao_sessions_init();

    printf("\n=== TEST CREATE USER ===\n");
    int64_t user_id = dao_users_create("test_user", "hash_pass_123", "");
    if (user_id == 0)
        printf("User create failed (maybe exists)\n");
    else
        printf("Created user_id = %lld\n", (long long)user_id);

    printf("\n=== TEST GET USER ===\n");
    UserRow user;
    int r = dao_users_get_by_username("test_user", &user);
    printf("Get by username: %d → id=%lld, username=%s\n",
           r, (long long)user.user_id, user.username);

    printf("\n=== TEST USER_STATS ===\n");
    dao_user_stats_ensure(user.user_id);
    dao_user_stats_inc_quickmode(user.user_id, 1);

    UserStatsRow st;
    dao_user_stats_get(user.user_id, &st);
    printf("Stats: quickmode_games=%lld, quickmode_wins=%lld\n",
           (long long)st.quickmode_games,
           (long long)st.quickmode_wins);

    printf("\n=== TEST SESSION ===\n");
    char expires[64];
    make_expires_in(expires, sizeof(expires), 2);

    const char *token =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcd";

    int64_t sess_id = dao_session_create(user.user_id, token, expires);
    printf("Created session_id = %lld\n", (long long)sess_id);

    printf("Validate session = %d\n",
           dao_session_validate(user.user_id, token));

    dao_session_heartbeat(user.user_id, token);
    dao_session_delete(user.user_id, token);

    db_close();
    return 0;
}
