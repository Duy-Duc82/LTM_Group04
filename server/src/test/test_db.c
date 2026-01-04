#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

int main(void) {
    const char *conninfo =
        "host=127.0.0.1 port=5432 dbname=ltm_group04 user=postgres password=1";

    printf("Connecting with conninfo: %s\n", conninfo);

    PGconn *conn = PQconnectdb(conninfo);

    if (conn == NULL) {
        fprintf(stderr, "[FATAL] PQconnectdb returned NULL (libpq problem)\n");
        return EXIT_FAILURE;
    }

    ConnStatusType status = PQstatus(conn);
    if (status != CONNECTION_OK) {
        fprintf(stderr, "[ERROR] Connection failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return EXIT_FAILURE;
    }

    printf("[OK] Connected to database.\n");

    // Test query
    const char *sql = "SELECT 1;";
    printf("Running test query: %s\n", sql);

    PGresult *res = PQexec(conn, sql);

    if (res == NULL) {
        fprintf(stderr, "[ERROR] PQexec returned NULL: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return EXIT_FAILURE;
    }

    ExecStatusType rstatus = PQresultStatus(res);
    if (rstatus != PGRES_TUPLES_OK) {
        fprintf(stderr, "[ERROR] Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return EXIT_FAILURE;
    }

    int nrows = PQntuples(res);
    int ncols = PQnfields(res);

    printf("[OK] Query succeeded. Rows=%d, Cols=%d\n", nrows, ncols);
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            printf("row %d col %d = %s\n", i, j, PQgetvalue(res, i, j));
        }
    }

    PQclear(res);
    PQfinish(conn);

    printf("[DONE] Test finished successfully.\n");
    return EXIT_SUCCESS;
}
