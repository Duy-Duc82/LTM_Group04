// server/src/test/test_db.c
#include <stdio.h>
#include "db.h"

int main(void) {
    const char *conninfo = getenv("DB_CONN");
    if (!conninfo) {
        fprintf(stderr, "Please set DB_CONN env, ví dụ:\n");
        fprintf(stderr,
          "export DB_CONN=\"host=localhost port=5432 dbname=ltm_group04 user=postgres password=1\"\n");
        return 1;
    }

    if (db_connect(conninfo) != 0) return 1;

    if (db_is_ok())
        printf("DB OK\n");
    else
        printf("DB NOT OK\n");

    db_disconnect();
    return 0;
}
