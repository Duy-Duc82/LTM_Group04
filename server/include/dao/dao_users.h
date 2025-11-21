// dao_users.h
#ifndef DAO_USERS_H
#define DAO_USERS_H

#include <stdint.h>

/**
 * users:
 *  user_id    BIGINT PK
 *  username   VARCHAR(32) UNIQUE
 *  password   VARCHAR(255)
 *  avatar_img VARCHAR(512)
 *
 * user_stats:
 *  user_id BIGINT PK
 *  quickmode_games BIGINT
 *  onevn_games BIGINT
 *  quickmode_wins BIGINT
 *  onevn_wins BIGINT
 */

typedef struct {
    int64_t user_id;
    char    username[64];
    char    password[256];
    char    avatar_img[512];
} UserRow;

typedef struct {
    int64_t user_id;
    int64_t quickmode_games;
    int64_t onevn_games;
    int64_t quickmode_wins;
    int64_t onevn_wins;
} UserStatsRow;

// chuẩn bị prepared statements
int dao_users_init();

// tạo user mới, trả về user_id; 0 nếu lỗi
int64_t dao_users_create(const char *username, const char *password_hash, const char *avatar_img);

// lấy user theo username: 1 = found, 0 = not found, -1 = error
int dao_users_get_by_username(const char *username, UserRow *out);

// lấy user theo id
int dao_users_get_by_id(int64_t user_id, UserRow *out);

// đảm bảo có bản ghi user_stats (insert nếu chưa tồn tại)
int dao_user_stats_ensure(int64_t user_id);

// lấy stats
int dao_user_stats_get(int64_t user_id, UserStatsRow *out);

// tăng thống kê sau 1 game quickmode
int dao_user_stats_inc_quickmode(int64_t user_id, int win);

// tăng thống kê sau 1 game 1vN
int dao_user_stats_inc_onevn(int64_t user_id, int win);

#endif // DAO_USERS_H
