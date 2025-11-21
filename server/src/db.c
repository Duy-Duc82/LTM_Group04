#include "..\include\db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DBConnection g_db = {0};

DBConnection *db() {
    return &g_db;
}

/**
 * Kết nối PostgreSQL
 * conninfo ví dụ:
 *   "host=localhost port=5432 dbname=ailatrieuphu user=postgres password=123"
 */
int db_init(const char *conninfo) {
    g_db.conn = PQconnectdb(conninfo);

    if (PQstatus(g_db.conn) != CONNECTION_OK) {
        fprintf(stderr, "[DB] Connection failed: %s\n", PQerrorMessage(g_db.conn));
        return 0;
    }

    printf("[DB] Connected successfully.\n");
    return 1;
}

void db_close() {
    if (g_db.conn) {
        PQfinish(g_db.conn);
        g_db.conn = NULL;
        printf("[DB] Connection closed.\n");
    }
}

int db_connection_ok() {
    return (PQstatus(g_db.conn) == CONNECTION_OK);
}

/**
 * Chạy query không tham số
 */
PGresult *db_exec(const char *query) {
    PGresult *res = PQexec(g_db.conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK &&
        PQresultStatus(res) != PGRES_COMMAND_OK) {

        fprintf(stderr, "[DB] Query failed: %s\nSQL: %s\n",
                PQerrorMessage(g_db.conn), query);
    }

    return res;
}

/**
 * Chạy prepared statement
 */
PGresult *db_exec_params(const char *stmtName, int nParams, const char *const *paramValues) {
    PGresult *res = PQexecPrepared(g_db.conn, stmtName, nParams, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK &&
        PQresultStatus(res) != PGRES_COMMAND_OK) {

        fprintf(stderr, "[DB] ExecPrepared failed: %s\nStatement: %s\n",
                PQerrorMessage(g_db.conn), stmtName);
    }

    return res;
}

/**
 * Tạo prepared statement
 */
int db_prepare(const char *stmtName, const char *sql, int nParams) {
    PGresult *res = PQprepare(g_db.conn, stmtName, sql, nParams, NULL);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "[DB] Prepare failed (%s): %s\nSQL: %s\n",
                stmtName, PQerrorMessage(g_db.conn), sql);
        PQclear(res);
        return 0;
    }

    PQclear(res);
    return 1;
}

/* ============================================================
 *              Helper đọc dữ liệu từ PGresult
 * ============================================================ */

int64_t db_get_int64(PGresult *res, int row, int col) {
    if (PQgetisnull(res, row, col)) return 0;
    return atoll(PQgetvalue(res, row, col));
}

int db_get_int(PGresult *res, int row, int col) {
    if (PQgetisnull(res, row, col)) return 0;
    return atoi(PQgetvalue(res, row, col));
}

const char *db_get_text(PGresult *res, int row, int col) {
    if (PQgetisnull(res, row, col)) return NULL;
    return PQgetvalue(res, row, col);
}

void db_print_error(PGresult *res) {
    fprintf(stderr, "[DB] ERROR: %s\n", PQerrorMessage(g_db.conn));
}

