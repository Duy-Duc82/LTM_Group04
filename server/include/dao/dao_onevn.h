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

// Lấy lịch sử game 1vN của user
int dao_onevn_get_user_history(int64_t user_id, void **json_history);

// Tạo round mới (khi bắt đầu round)
int dao_onevn_create_round(int64_t session_id, int round_number, int64_t question_id, const char *difficulty, int64_t *out_round_id);

// Kết thúc round
int dao_onevn_end_round(int64_t round_id);

// Lưu câu trả lời của player
int dao_onevn_save_player_answer(int64_t round_id, int64_t user_id, char answer, int is_correct, int score_gained, double time_left);

// Lấy chi tiết replay của session
int dao_onevn_get_replay_details(int64_t session_id, void **json_replay);

#endif

