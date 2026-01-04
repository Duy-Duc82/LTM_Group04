// server/include/db.h
#ifndef DB_H
#define DB_H

#include <libpq-fe.h>

extern PGconn *db_conn;

// conninfo ví dụ: "host=localhost port=5432 dbname=ltm_group04 user=postgres password=1"
int  db_connect(const char *conninfo);
void db_disconnect(void);
int  db_is_ok(void);

// returns the global libpq connection (NULL if not connected)
PGconn *db_get_conn(void);

// compatibility helpers used by tests
// db_init: look for DB_CONN env var, call db_connect
int db_init(void);
void db_close(void);

// helper
void db_log_error(PGresult *res, const char *msg);

#endif
