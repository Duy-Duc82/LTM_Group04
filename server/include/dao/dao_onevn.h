#ifndef DAO_ONEVN_H
#define DAO_ONEVN_H

#include <stdint.h>

// Tạo 1vN session từ room
int dao_onevn_create_session(int64_t room_id, int64_t *out_session_id);

// Cập nhật players JSONB
int dao_onevn_update_players(int64_t session_id, const char *players_json);

// Kết thúc session và lưu kết quả
int dao_onevn_end_session(int64_t session_id, int64_t winner_id);

// Lấy thông tin session
int dao_onevn_get_session(int64_t session_id, int64_t *room_id, int64_t *winner_id, char *status, char *players_json, size_t json_len);

#endif

