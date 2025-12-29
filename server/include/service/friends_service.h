#ifndef FRIENDS_SERVICE_H
#define FRIENDS_SERVICE_H

#include <stdint.h>
#include "service/client_session.h"

// Dispatch friend-related commands
void friends_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);

// Notify friends when a user's status changes (online/offline/in-game)
void friends_notify_status_change(int64_t user_id, const char *status, int64_t room_id);

// Helper: Get online status string for a user
const char *friends_get_user_status(int64_t user_id);

#endif

