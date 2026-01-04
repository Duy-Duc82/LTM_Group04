#ifndef DAO_CHAT_H
#define DAO_CHAT_H

#include <stdint.h>

int dao_chat_send_dm(int64_t sender_id, int64_t receiver_id, const char *content);
int dao_chat_send_room(int64_t sender_id, int64_t room_id, const char *content);
int dao_chat_fetch_offline(int64_t user_id, void **result_json); // messages where receiver_id = user AND is_read=false
// messages from specific sender -> receiver where is_read=false
int dao_chat_fetch_offline_from_sender(int64_t receiver_id, int64_t sender_id, void **result_json);
// Fetch all messages (sent and received) between two users
int dao_chat_fetch_conversation(int64_t user_id, int64_t friend_id, void **result_json);
int dao_chat_mark_read(int64_t user_id);

#endif
