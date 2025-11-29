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

// auth: 1 = đúng, 0 = sai, <0 lỗi DB
int dao_users_check_password(const char *username, const char *password, int64_t *out_user_id);

#endif
