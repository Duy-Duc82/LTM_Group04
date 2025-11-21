// Kết nối, helper chung cho PostgreSQL (libpq)
#ifndef DB_H
#define DB_H

#include <libpq-fe.h>
#include <stdint.h>

/**
 * db.h
 * Module quản lý kết nối PostgreSQL và các hàm helper
 * Dùng mô hình Singleton: toàn bộ server dùng 1 kết nối duy nhất.
 */

typedef struct {
    PGconn *conn;
} DBConnection;

// Lấy con trỏ DB global
DBConnection *db();

// Khởi tạo / đóng DB
int db_init(const char *conninfo);
void db_close();

// Kiểm tra kết nối còn sống
int db_connection_ok();

// Chạy query thường (không param)
PGresult *db_exec(const char *query);

// Chạy prepared query (param)
PGresult *db_exec_params(const char *stmtName, int nParams, const char *const *paramValues);

// Chuẩn bị prepared statement
int db_prepare(const char *stmtName, const char *sql, int nParams);

// ===== Helper extract values từ PGresult =====

// Lấy BIGINT (int64) từ result
int64_t db_get_int64(PGresult *res, int row, int col);

// Lấy INT (int32)
int db_get_int(PGresult *res, int row, int col);

// Lấy TEXT (nullable), trả con trỏ string bên trong PGresult
const char *db_get_text(PGresult *res, int row, int col);

// In lỗi SQL
void db_print_error(PGresult *res);

#endif // DB_H
