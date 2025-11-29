// server/include/db.h
#ifndef DB_H
#define DB_H

#include <libpq-fe.h>

extern PGconn *db_conn;

// conninfo ví dụ: "host=localhost port=5432 dbname=ltm_group04 user=postgres password=1"
int  db_connect(const char *conninfo);
void db_disconnect(void);
int  db_is_ok(void);

// helper
void db_log_error(PGresult *res, const char *msg);

#endif
