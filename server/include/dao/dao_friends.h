#ifndef DAO_FRIENDS_H
#define DAO_FRIENDS_H

#include <stdbool.h>
#include <stdint.h>

// Trạng thái bạn bè
typedef enum {
    FRIEND_STATUS_PENDING,
    FRIEND_STATUS_ACCEPTED,
    FRIEND_STATUS_BLOCKED
} friend_status_t;

int dao_friends_send_request(int64_t from_user, int64_t to_user);
int dao_friends_respond_request(int64_t from_user, int64_t to_user, bool accept);
int dao_friends_list(int64_t user_id, /* OUT */ void **result_json);
int dao_friends_update_status(int64_t user_id, int64_t friend_id, friend_status_t status);

#endif
