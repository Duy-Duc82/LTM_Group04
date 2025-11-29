// server/src/test/test_quickmode.c
#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "service/quickmode_service.h"

int main(void) {
    const char *conninfo = getenv("DB_CONN");
    if (!conninfo) {
        fprintf(stderr, "Set DB_CONN env first\n");
        return 1;
    }
    if (db_connect(conninfo) != 0) return 1;

    // giả sử đã có user 'alice'
    int64_t user_id = 1;   // tùy DB, có thể query trước, tạm hard-code để test
    qm_debug_start(user_id);

    db_disconnect();
    return 0;
}
