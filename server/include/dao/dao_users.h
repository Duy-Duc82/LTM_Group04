// server/include/dao/dao_users.h
#ifndef DAO_USERS_H
#define DAO_USERS_H

#include <stdint.h>

typedef struct {
    int64_t user_id;
    char    username[33];
    char    password[256];  // tạm thời plaintext / hash
    char    avatar_img[513];
} User;

// trả về 0 nếu OK, -1 nếu lỗi, -2 nếu username đã tồn tại
int dao_users_create(const char *username, const char *password, int64_t *out_user_id);

// 0 nếu tìm thấy, -1 nếu không có
int dao_users_find_by_username(const char *username, User *out_user);

// find user by id, 0=OK, -1 if not found
int dao_users_find_by_id(int64_t user_id, User *out_user);

// auth: 1 = đúng, 0 = sai, <0 lỗi DB
int dao_users_check_password(const char *username, const char *password, int64_t *out_user_id);

// Search users by username (partial match), returns JSON array
int dao_users_search_by_username(const char *query, int limit, void **result_json);

// Update user avatar, 0=OK, -1 if error
int dao_users_update_avatar(int64_t user_id, const char *avatar_path);

#endif
