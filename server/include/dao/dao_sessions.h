// dao_sessions.h
#ifndef DAO_SESSIONS_H
#define DAO_SESSIONS_H

#include <stdint.h>

/**
 * user_sessions:
 *  id           BIGINT PK
 *  user_id      BIGINT
 *  access_token CHAR(64) UNIQUE
 *  last_heartbeat TIMESTAMPTZ
 *  expires_at   TIMESTAMPTZ
 */

int dao_sessions_init();

// tạo session mới, trả về id; 0 nếu lỗi
int64_t dao_session_create(int64_t user_id, const char *access_token, const char *expires_at_iso);

// validate (user_id, token) còn hạn
// 1 = ok, 0 = không có / hết hạn, -1 = lỗi
int dao_session_validate(int64_t user_id, const char *access_token);

// update last_heartbeat = NOW()
int dao_session_heartbeat(int64_t user_id, const char *access_token);

// xoá session (logout)
int dao_session_delete(int64_t user_id, const char *access_token);

#endif // DAO_SESSIONS_H
