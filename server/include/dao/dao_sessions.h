// server/include/dao/dao_sessions.h
#ifndef DAO_SESSIONS_H
#define DAO_SESSIONS_H

#include <stdint.h>
#include <time.h>

typedef struct {
    int64_t id;
    int64_t user_id;
    char    access_token[65]; // CHAR(64) + '\0'
    time_t  expires_at;
} UserSession;

// tạo session mới cho user, gen token 64 hex char
int dao_sessions_create(int64_t user_id, int ttl_seconds, UserSession *out_sess);

// lấy theo token, 0=OK, -1=không thấy
int dao_sessions_find_by_token(const char *token, UserSession *out_sess);

// cập nhật last_heartbeat + expires_at (refresh)
int dao_sessions_touch(const char *token, int ttl_seconds);

#endif
