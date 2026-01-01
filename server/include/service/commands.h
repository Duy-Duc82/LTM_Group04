// server/include/service/commands.h
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

// ============================================================
// COMMAND TYPE DEFINITIONS
// ============================================================
// Format: 0xMAJOR*MINOR
// where MAJOR = category (0x01=Auth, 0x02=Friends, etc.)
//       MINOR = specific command
// ============================================================

// ========== 0x01xx – Authentication (REQ/RES pairs)
#define CMD_REQ_REGISTER        0x0101    // Client → Server: Register request
#define CMD_RES_REGISTER        0x0102    // Server → Client: Register response
#define CMD_REQ_LOGIN           0x0103    // Client → Server: Login request
#define CMD_RES_LOGIN           0x0104    // Server → Client: Login response
#define CMD_NOTIFY_USER_STATUS  0x0105    // Server → Client: User status change
#define CMD_REQ_LOGOUT          0x0106    // Client → Server: Logout request
#define CMD_RES_LOGOUT          0x0107    // Server → Client: Logout response

// ========== 0x02xx – Friends System
#define CMD_REQ_ADD_FRIEND      0x0201    // Client → Server: Send friend request
#define CMD_RES_ADD_FRIEND      0x0202    // Server → Client: Friend request result
#define CMD_NOTIFY_FRIEND_REQ   0x0203    // Server → Client: Incoming friend request
#define CMD_REQ_RESPOND_FRIEND  0x0204    // Client → Server: Accept/Reject friend
#define CMD_RES_RESPOND_FRIEND  0x0205    // Server → Client: Response to acceptance
#define CMD_REQ_LIST_FRIENDS    0x0206    // Client → Server: Get friend list
#define CMD_RES_LIST_FRIENDS    0x0207    // Server → Client: Return friend list
#define CMD_NOTIFY_FRIEND_STATUS 0x0208   // Server → Client: Friend online/offline
#define CMD_REQ_SEARCH_USER      0x0209    // Client → Server: Search user by username
#define CMD_RES_SEARCH_USER      0x020A    // Server → Client: Search user result
#define CMD_REQ_GET_FRIEND_INFO  0x020B    // Client → Server: Get friend info
#define CMD_RES_GET_FRIEND_INFO  0x020C    // Server → Client: Friend info result
#define CMD_REQ_GET_PENDING_REQ  0x020D    // Client → Server: Get pending requests
#define CMD_RES_GET_PENDING_REQ  0x020E    // Server → Client: Pending requests list
#define CMD_REQ_REMOVE_FRIEND    0x020F    // Client → Server: Remove friend
#define CMD_RES_REMOVE_FRIEND    0x0210    // Server → Client: Remove friend result

// ========== 0x03xx – Chat (Direct Messages & Rooms)
#define CMD_REQ_SEND_DM         0x0301    // Client → Server: Send private message
#define CMD_RES_SEND_DM         0x0302    // Server → Client: DM sent success
#define CMD_NOTIFY_DM           0x0303    // Server → Client: Receive private message
#define CMD_REQ_SEND_ROOM_CHAT  0x0304    // Client → Server: Send room message
#define CMD_RES_SEND_ROOM_CHAT  0x0305    // Server → Client: Room chat result
#define CMD_NOTIFY_ROOM_CHAT    0x0306    // Server → Client: Broadcast room message
#define CMD_REQ_FETCH_OFFLINE   0x0307    // Client → Server: Fetch offline messages
#define CMD_RES_FETCH_OFFLINE   0x0308    // Server → Client: Return offline messages

// ========== 0x04xx – Lobby & Room Management
#define CMD_REQ_CREATE_ROOM     0x0401    // Client → Server: Create room
#define CMD_RES_CREATE_ROOM     0x0402    // Server → Client: Room creation result
#define CMD_REQ_JOIN_ROOM       0x0403    // Client → Server: Join room
#define CMD_RES_JOIN_ROOM       0x0404    // Server → Client: Join room result
#define CMD_REQ_LIST_ROOMS      0x0405    // Client → Server: List waiting rooms
#define CMD_RES_LIST_ROOMS      0x0406    // Server → Client: List rooms result
#define CMD_NOTIFY_ROOM_UPDATE  0x0411    // Server → Client: Room member update
#define CMD_REQ_INVITE_FRIEND   0x0412    // Client → Server: Invite friend
#define CMD_RES_INVITE_FRIEND   0x0413    // Server → Client: Invite friend result
#define CMD_NOTIFY_ROOM_INVITE  0x0414    // Server → Client: Room invite received
#define CMD_REQ_RESPOND_INVITE  0x0415    // Client → Server: Accept/Reject invite
#define CMD_RES_RESPOND_INVITE  0x0416    // Server → Client: Invite result
#define CMD_REQ_LEAVE_ROOM      0x040B    // Client → Server: Leave room
#define CMD_RES_LEAVE_ROOM      0x040C    // Server → Client: Leave room result
#define CMD_REQ_START_GAME      0x040D    // Client → Server: Host starts game
#define CMD_RES_START_GAME      0x040E    // Server → Client: Start game result

// ========== 0x05xx – Game: Basic Mode (Solo - Quickmode)
#define CMD_REQ_START_QUICKMODE    0x0500    // Client → Server: Start game
#define CMD_NOTIFY_GAME_START      0x0501    // Server → Client: Start game notification
#define CMD_NOTIFY_QUESTION        0x0502    // Server → Client: New question
#define CMD_REQ_SUBMIT_ANSWER      0x0503    // Client → Server: Submit answer
#define CMD_RES_SUBMIT_ANSWER      0x0504    // Server → Client: Answer received
#define CMD_NOTIFY_ANSWER_RESULT   0x0505    // Server → Client: Correct/Wrong
#define CMD_REQ_USE_LIFELINE       0x0506    // Client → Server: Use 50-50 lifeline (max 2 times per session)
#define CMD_RES_USE_LIFELINE       0x0507    // Server → Client: Lifeline result (2 options remaining, 2 removed)
#define CMD_NOTIFY_ROUND_END       0x0508    // Server → Client: End of round [OPTIONAL]
#define CMD_NOTIFY_GAME_OVER       0x0509    // Server → Client: Game end
#define CMD_REQ_GET_QUESTION        0x050A    // Client → Server: Get question for round

// ========== 0x06xx – Game: 1vN Mode (Competitive)
#define CMD_NOTIFY_GAME_START_1VN 0x0601  // Server → Client: Start 1vN
#define CMD_NOTIFY_QUESTION_1VN   0x0602  // Server → Client: Question to all
#define CMD_REQ_SUBMIT_ANSWER_1VN 0x0603  // Client → Server: Submit answer
#define CMD_RES_SUBMIT_ANSWER_1VN 0x0604  // Server → Client: Answer result
#define CMD_NOTIFY_ELIMINATION    0x0605  // Server → Client: Eliminated players
#define CMD_NOTIFY_GAME_OVER_1VN  0x0606  // Server → Client: Game end

// ========== 0x07xx – Stats / Leaderboard
#define CMD_REQ_GET_PROFILE     0x0701    // Client → Server: Request profile
#define CMD_RES_GET_PROFILE     0x0702    // Server → Client: Profile result
#define CMD_REQ_LEADERBOARD     0x0703    // Client → Server: Request leaderboard
#define CMD_RES_LEADERBOARD     0x0704    // Server → Client: Leaderboard result
#define CMD_REQ_MATCH_HISTORY   0x0705    // Client → Server: Request match history
#define CMD_RES_MATCH_HISTORY   0x0706    // Server → Client: Match history result
#define CMD_REQ_UPDATE_AVATAR   0x0707    // Client → Server: Update avatar
#define CMD_RES_UPDATE_AVATAR   0x0708    // Server → Client: Update avatar result
#define CMD_REQ_GET_ONEVN_HISTORY 0x0709  // Client → Server: Request 1vN game history
#define CMD_RES_GET_ONEVN_HISTORY 0x070A  // Server → Client: 1vN game history result
#define CMD_REQ_GET_REPLAY_DETAILS 0x070B  // Client → Server: Request replay details
#define CMD_RES_GET_REPLAY_DETAILS 0x070C  // Server → Client: Replay details result

// ========== 0x08xx – System / Error / Connection
#define CMD_REQ_PING            0x0801    // Client → Server: Ping
#define CMD_RES_PING            0x0802    // Server → Client: Ping response
#define CMD_NOTIFY_ERROR        0x0803    // Server → Client: Generic error
#define CMD_REQ_RECONNECT       0x0804    // Client → Server: Reconnect after disconnect
#define CMD_RES_RECONNECT       0x0805    // Server → Client: Reconnect success

// ============================================================
// HELPER FUNCTIONS
// ============================================================

/**
 * Get category from command (high byte)
 * Example: get_cmd_category(0x0101) returns 0x01
 */
static inline uint8_t get_cmd_category(uint16_t cmd) {
    return (uint8_t)((cmd >> 8) & 0xFF);
}

/**
 * Get specific command (low byte)
 * Example: get_cmd_specific(0x0101) returns 0x01
 */
static inline uint8_t get_cmd_specific(uint16_t cmd) {
    return (uint8_t)(cmd & 0xFF);
}

/**
 * Check if command is a request (typically odd cmd_specific)
 */
static inline int is_request_cmd(uint16_t cmd) {
    uint8_t specific = get_cmd_specific(cmd);
    return (specific & 0x01) == 0x01;  // Requests end in odd numbers
}

/**
 * Check if command is a response (typically even cmd_specific)
 */
static inline int is_response_cmd(uint16_t cmd) {
    uint8_t specific = get_cmd_specific(cmd);
    return (specific & 0x01) == 0x00;  // Responses end in even numbers
}

#endif
