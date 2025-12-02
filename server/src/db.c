// server/src/db.c
#include <stdio.h>
#include "../include/db.h"
#include <stdlib.h>
#include <libpq-fe.h>


PGconn *db_conn = NULL;

int db_connect(const char *conninfo) {
    db_conn = PQconnectdb(conninfo);
    if (PQstatus(db_conn) != CONNECTION_OK) {
        fprintf(stderr, "[DB] Connection failed: %s\n",
                PQerrorMessage(db_conn));
        return -1;
    }
    printf("[DB] Connected OK\n");
    return 0;
}

void db_disconnect(void) {
    if (db_conn) {
        PQfinish(db_conn);
        db_conn = NULL;
        printf("[DB] Disconnected\n");
    }
}

int db_is_ok(void) {
    if (!db_conn) return 0;
    return PQstatus(db_conn) == CONNECTION_OK;
}

void db_log_error(PGresult *res, const char *msg) {
    fprintf(stderr, "[DB] %s: %s\n", msg, PQerrorMessage(db_conn));
    if (res) PQclear(res);
}

PGconn *db_get_conn(void) {
    return db_conn;
}

int db_init(void) {
    const char *conn = getenv("DB_CONN");
    if (!conn) {
        fprintf(stderr, "Please set DB_CONN environment variable!\n");
        return -1;
    }
    return db_connect(conn);
}

void db_close(void) {
    db_disconnect();
}
