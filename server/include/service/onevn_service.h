// server/include/service/onevn_service.h
#ifndef ONEVN_SERVICE_H
#define ONEVN_SERVICE_H

#include <stdint.h>
#include "service/client_session.h"

// Dispatch 1vN-related commands (0x06xx)
void onevn_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);

// Mark a player as eliminated in the active game by room_id
// This updates both in-memory game state and database
// Returns 0 if success, -1 if game not found or player not in game
int onevn_eliminate_player_by_room(int64_t room_id, int64_t user_id);

#endif

