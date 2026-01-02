#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "service/friends_service.h"
#include "service/commands.h"
#include "service/protocol.h"
#include "service/session_manager.h"
#include "dao/dao_friends.h"
#include "dao/dao_users.h"
#include "dao/dao_rooms.h"
#include "dao/dao_chat.h"
#include "utils/json.h"

#define ROOM_CHAT_MAX_LENGTH 200
#define ROOM_CHAT_RATE_LIMIT 5
#define ROOM_CHAT_RATE_WINDOW_SECONDS 3
#define ROOM_CHAT_RATE_TABLE_SIZE 256

typedef struct {
    int64_t user_id;
    int64_t room_id;
    time_t window_start;
    int count;
} RoomChatRateEntry;

static RoomChatRateEntry g_room_chat_rates[ROOM_CHAT_RATE_TABLE_SIZE];

// Simple per-user-per-room token bucket: 5 messages per 3 seconds
static bool room_chat_rate_allow(int64_t user_id, int64_t room_id) {
    time_t now = time(NULL);
    size_t empty_idx = (size_t)-1;
    size_t oldest_idx = 0;
    time_t oldest_ts = g_room_chat_rates[0].window_start;

    for (size_t i = 0; i < ROOM_CHAT_RATE_TABLE_SIZE; i++) {
        RoomChatRateEntry *entry = &g_room_chat_rates[i];

        if (entry->user_id == user_id && entry->room_id == room_id) {
            // Found bucket for this user/room
            if (now - entry->window_start >= ROOM_CHAT_RATE_WINDOW_SECONDS) {
                entry->window_start = now;
                entry->count = 0;
            }
            if (entry->count >= ROOM_CHAT_RATE_LIMIT) {
                return false;
            }
            entry->count++;
            return true;
        }

        if (entry->user_id == 0 && entry->room_id == 0 && empty_idx == (size_t)-1) {
            empty_idx = i;
        }

        if (i == 0 || entry->window_start < oldest_ts) {
            oldest_ts = entry->window_start;
            oldest_idx = i;
        }
    }

    // No existing bucket, create new one (prefer empty slot, else evict oldest)
    size_t idx = (empty_idx != (size_t)-1) ? empty_idx : oldest_idx;
    RoomChatRateEntry *entry = &g_room_chat_rates[idx];
    entry->user_id = user_id;
    entry->room_id = room_id;
    entry->window_start = now;
    entry->count = 1;
    return true;
}

// Helper: Get online status string for a user
const char *friends_get_user_status(int64_t user_id) {
    ClientSession *sess = session_manager_get_by_user_id(user_id);
    if (!sess) return "offline";
    
    if (sess->room_id > 0) {
        // Check if room is in game
        // For simplicity, we'll check if room status is IN_PROGRESS
        // This would require a dao_rooms_get_status call, but for now we'll use "in_game" if in room
        return "in_game";
    }
    return "online";
}

// Helper: Broadcast friend status change to all friends of a user
static void broadcast_status_to_friends(int64_t user_id, const char *status, int64_t room_id) {
    printf("[FRIENDS] === broadcast_status_to_friends ===\n");
    printf("[FRIENDS] user_id=%lld, status=%s, room_id=%lld\n", 
           (long long)user_id, status, (long long)room_id);
    fflush(stdout);
    
    void *friends_json = NULL;
    if (dao_friends_list(user_id, &friends_json) != 0) {
        printf("[FRIENDS] No friends found or error getting friends list\n");
        fflush(stdout);
        return; // No friends or error
    }
    
    printf("[FRIENDS] Friends JSON: %s\n", (char *)friends_json);
    fflush(stdout);
    
    // Parse friends list
    int64_t friend_ids[256];
    int count = util_json_parse_user_id_array((const char *)friends_json, friend_ids, 256);
    free(friends_json);
    
    printf("[FRIENDS] Parsed %d friends\n", count);
    fflush(stdout);
    
    // Build status notification JSON
    char status_json[512];
    snprintf(status_json, sizeof(status_json),
        "{\"user_id\": %ld, \"status\": \"%s\", \"room_id\": %ld}",
        user_id, status, room_id);
    
    printf("[FRIENDS] Status JSON: %s\n", status_json);
    fflush(stdout);
    
    // Send to each friend
    for (int i = 0; i < count; i++) {
        printf("[FRIENDS] Sending notification to friend_id=%lld\n", (long long)friend_ids[i]);
        fflush(stdout);
        int sent = session_manager_send_to_user(friend_ids[i], CMD_NOTIFY_FRIEND_STATUS,
            status_json, (uint32_t)strlen(status_json));
        printf("[FRIENDS] Notification sent to friend_id=%lld: %d\n", 
               (long long)friend_ids[i], sent);
        fflush(stdout);
    }
    
    printf("[FRIENDS] Finished broadcasting status to friends\n");
    fflush(stdout);
}

// Notify friends when a user's status changes
void friends_notify_status_change(int64_t user_id, const char *status, int64_t room_id) {
    broadcast_status_to_friends(user_id, status, room_id);
}

// Handle search user
static void handle_search_user(ClientSession *sess, const char *payload) {
    char *query = util_json_get_string(payload, "query");
    if (!query) {
        protocol_send_error(sess, CMD_RES_SEARCH_USER, "MISSING_QUERY");
        return;
    }
    
    int limit = 20;
    long long limit_val_ll = 0;
    if (util_json_get_int64(payload, "limit", &limit_val_ll) && limit_val_ll > 0) {
        limit = (int)limit_val_ll;
        if (limit > 100) limit = 100; // Cap at 100
    }
    
    void *result_json = NULL;
    if (dao_users_search_by_username(query, limit, &result_json) != 0) {
        free(query);
        protocol_send_error(sess, CMD_RES_SEARCH_USER, "SEARCH_FAILED");
        return;
    }
    
    // Free query after use
    free(query);
    
    const char *json_str = (const char *)result_json;
    protocol_send_response(sess, CMD_RES_SEARCH_USER, json_str, strlen(json_str));
    free(result_json);
}

// Handle get friend info with online status
static void handle_get_friend_info(ClientSession *sess, const char *payload) {
    long long friend_id_ll = 0;
    if (!util_json_get_int64(payload, "friend_id", &friend_id_ll) || friend_id_ll <= 0) {
        protocol_send_error(sess, CMD_RES_GET_FRIEND_INFO, "INVALID_FRIEND_ID");
        return;
    }
    int64_t friend_id = (int64_t)friend_id_ll;
    
    // Get friend info from DB
    void *info_json = NULL;
    if (dao_friends_get_info(sess->user_id, friend_id, &info_json) != 0) {
        protocol_send_error(sess, CMD_RES_GET_FRIEND_INFO, "USER_NOT_FOUND");
        return;
    }
    
    // Parse the JSON to add online status
    char *info_str = (char *)info_json;
    size_t info_len = strlen(info_str);
    
    // Check online status
    const char *status = friends_get_user_status(friend_id);
    ClientSession *friend_sess = session_manager_get_by_user_id(friend_id);
    int64_t room_id = friend_sess ? friend_sess->room_id : 0;
    
    // Build enhanced JSON with online status
    char enhanced_json[2048];
    // Remove closing brace, add status fields
    if (info_len > 0 && info_str[info_len - 1] == '}') {
        info_str[info_len - 1] = '\0';
    }
    snprintf(enhanced_json, sizeof(enhanced_json),
        "%s, \"online_status\": \"%s\", \"room_id\": %ld}",
        info_str, status, room_id);
    
    protocol_send_response(sess, CMD_RES_GET_FRIEND_INFO, enhanced_json, strlen(enhanced_json));
    free(info_json);
}

// Handle remove friend request
static void handle_remove_friend(ClientSession *sess, const char *payload) {
    long long friend_id_ll = 0;
    if (!util_json_get_int64(payload, "friend_id", &friend_id_ll) || friend_id_ll <= 0) {
        protocol_send_error(sess, CMD_RES_REMOVE_FRIEND, "INVALID_FRIEND_ID");
        return;
    }
    int64_t friend_id = (int64_t)friend_id_ll;
    
    // Check if trying to remove self
    if (friend_id == sess->user_id) {
        protocol_send_error(sess, CMD_RES_REMOVE_FRIEND, "CANNOT_REMOVE_SELF");
        return;
    }
    
    // Remove friend relationship (both directions)
    if (dao_friends_remove(sess->user_id, friend_id) != 0) {
        protocol_send_error(sess, CMD_RES_REMOVE_FRIEND, "REMOVE_FRIEND_FAILED");
        return;
    }
    
    // Notify the removed friend if online
    ClientSession *friend_sess = session_manager_get_by_user_id(friend_id);
    if (friend_sess) {
        // Notify friend that they were removed
        char notify_json[256];
        snprintf(notify_json, sizeof(notify_json),
            "{\"user_id\": %lld, \"status\": \"removed\"}",
            (long long)sess->user_id);
        session_manager_send_to_user(friend_id, CMD_NOTIFY_FRIEND_STATUS,
            notify_json, (uint32_t)strlen(notify_json));
    }
    
    protocol_send_simple_ok(sess, CMD_RES_REMOVE_FRIEND);
}

// Handle add friend request
static void handle_add_friend(ClientSession *sess, const char *payload) {
    long long friend_id_ll = 0;
    if (!util_json_get_int64(payload, "friend_id", &friend_id_ll) || friend_id_ll <= 0) {
        protocol_send_error(sess, CMD_RES_ADD_FRIEND, "INVALID_FRIEND_ID");
        return;
    }
    int64_t friend_id = (int64_t)friend_id_ll;
    
    // Check if trying to add self
    if (friend_id == sess->user_id) {
        protocol_send_error(sess, CMD_RES_ADD_FRIEND, "CANNOT_ADD_SELF");
        return;
    }
    
    // Send friend request
    if (dao_friends_send_request(sess->user_id, friend_id) != 0) {
        protocol_send_error(sess, CMD_RES_ADD_FRIEND, "ADD_FRIEND_FAILED");
        return;
    }
    
    // Send notification to friend if online
    ClientSession *friend_sess = session_manager_get_by_user_id(friend_id);
    if (friend_sess) {
        User sender;
        if (dao_users_find_by_id(sess->user_id, &sender) == 0) {
            char notify_json[512];
            char *esc_username = util_json_escape(sender.username);
            if (!esc_username) esc_username = strdup("");
            
            snprintf(notify_json, sizeof(notify_json),
                "{\"from_user_id\": %lld, \"from_username\": \"%s\"}",
                (long long)sess->user_id, esc_username);
            
            session_manager_send_to_user(friend_id, CMD_NOTIFY_FRIEND_REQ,
                notify_json, (uint32_t)strlen(notify_json));
            
            free(esc_username);
        }
    }
    
    // Send success response
    protocol_send_simple_ok(sess, CMD_RES_ADD_FRIEND);
}

// Handle list friends
static void handle_list_friends(ClientSession *sess) {
    void *result_json = NULL;
    if (dao_friends_list(sess->user_id, &result_json) != 0) {
        protocol_send_error(sess, CMD_RES_LIST_FRIENDS, "LIST_FRIENDS_FAILED");
        return;
    }
    
    // Parse the JSON array and add online status for each friend
    char *json_str = (char *)result_json;
    size_t json_len = strlen(json_str);
    
    // Check if it's an array
    if (json_str[0] != '[') {
        // Not an array, send as-is
        protocol_send_response(sess, CMD_RES_LIST_FRIENDS, json_str, json_len);
        free(result_json);
        return;
    }
    
    // Simple approach: parse JSON string manually to extract user_id and username,
    // then rebuild with online_status
    // Format: [{"user_id": X, "username": "Y", "status": "Z"}, ...]
    
    // Extract friend IDs first
    int64_t friend_ids[256];
    int count = util_json_parse_user_id_array(json_str, friend_ids, 256);
    
    if (count > 0) {
        // Build enhanced JSON with online status
        size_t cap = json_len + count * 150; // Extra space for online_status and room_id fields
        size_t used = 0;
        char *enhanced = malloc(cap);
        if (!enhanced) {
            free(result_json);
            protocol_send_error(sess, CMD_RES_LIST_FRIENDS, "MEMORY_ERROR");
            return;
        }
        
        used += snprintf(enhanced + used, cap - used, "[");
        
        // Parse original JSON to get usernames
        // Simple parsing: find each object and extract username
        int processed = 0;
        
        for (int i = 0; i < count && processed < count; i++) {
            int64_t friend_id = friend_ids[i];
            const char *status = friends_get_user_status(friend_id);
            ClientSession *friend_sess = session_manager_get_by_user_id(friend_id);
            int64_t room_id = friend_sess ? friend_sess->room_id : 0;
            
            // Get friend username from database
            User friend_user;
            if (dao_users_find_by_id(friend_id, &friend_user) != 0) {
                continue; // Skip if user not found
            }
            
            char *esc_username = util_json_escape(friend_user.username);
            if (!esc_username) esc_username = strdup("");
            
            if (processed > 0) used += snprintf(enhanced + used, cap - used, ",");
            used += snprintf(enhanced + used, cap - used,
                "{\"user_id\": %ld, \"username\": \"%s\", \"status\": \"ACCEPTED\", \"online_status\": \"%s\", \"room_id\": %ld}",
                friend_id, esc_username, status, room_id);
            
            free(esc_username);
            processed++;
        }
        
        used += snprintf(enhanced + used, cap - used, "]");
        
        protocol_send_response(sess, CMD_RES_LIST_FRIENDS, enhanced, used);
        free(enhanced);
    } else {
        // No friends or parse failed, send original
        protocol_send_response(sess, CMD_RES_LIST_FRIENDS, json_str, json_len);
    }
    
    free(result_json);
}

// Handle get pending friend requests
static void handle_get_pending_requests(ClientSession *sess) {
    void *result_json = NULL;
    if (dao_friends_get_pending_requests(sess->user_id, &result_json) != 0) {
        protocol_send_error(sess, CMD_RES_GET_PENDING_REQ, "GET_PENDING_FAILED");
        return;
    }
    
    const char *json_str = (const char *)result_json;
    protocol_send_response(sess, CMD_RES_GET_PENDING_REQ, json_str, strlen(json_str));
    free(result_json);
}

// Handle respond to friend request (accept/reject)
static void handle_respond_friend(ClientSession *sess, const char *payload) {
    long long from_user_id_ll = 0;
    bool accept = false;
    
    if (!util_json_get_int64(payload, "from_user_id", &from_user_id_ll) || from_user_id_ll <= 0) {
        protocol_send_error(sess, CMD_RES_RESPOND_FRIEND, "INVALID_FROM_USER_ID");
        return;
    }
    int64_t from_user_id = (int64_t)from_user_id_ll;
    
    // Get accept flag (true = accept, false = reject)
    char *accept_str = util_json_get_string(payload, "accept");
    if (!accept_str) {
        protocol_send_error(sess, CMD_RES_RESPOND_FRIEND, "MISSING_ACCEPT_FLAG");
        return;
    }
    accept = (strcmp(accept_str, "true") == 0 || strcmp(accept_str, "1") == 0);
    free(accept_str);
    
    // Respond to friend request
    if (dao_friends_respond_request(from_user_id, sess->user_id, accept) != 0) {
        protocol_send_error(sess, CMD_RES_RESPOND_FRIEND, "RESPOND_FAILED");
        return;
    }
    
    // If accepted, create reverse relationship and notify both users
    if (accept) {
        if (dao_friends_send_request(sess->user_id, from_user_id) != 0) {
            // If reverse relationship creation fails, still return success
            // (the main relationship is already accepted)
            fprintf(stderr, "[FRIENDS] Warning: Failed to create reverse relationship\n");
        }
        // Update the reverse relationship to ACCEPTED
        dao_friends_update_status(sess->user_id, from_user_id, FRIEND_STATUS_ACCEPTED);
        
        // Notify the sender (A) that their request was accepted
        // This will trigger A's client to refresh friends list
        ClientSession *sender_sess = session_manager_get_by_user_id(from_user_id);
        
        // Get B's status (the one who accepted)
        const char *status_b = friends_get_user_status(sess->user_id);
        int64_t room_id_b = sess->room_id;
        
        char notify_json_a[512];
        snprintf(notify_json_a, sizeof(notify_json_a),
            "{\"user_id\": %lld, \"status\": \"%s\", \"room_id\": %ld}",
            (long long)sess->user_id, status_b, room_id_b);
        
        session_manager_send_to_user(from_user_id, CMD_NOTIFY_FRIEND_STATUS,
            notify_json_a, (uint32_t)strlen(notify_json_a));
        
        printf("[FRIENDS] Sent CMD_NOTIFY_FRIEND_STATUS to user %lld (sender) about user %lld (accepter)\n",
               (long long)from_user_id, (long long)sess->user_id);
        fflush(stdout);
        
        // Also notify B about A's status (optional, but good for consistency)
        const char *status_a = friends_get_user_status(from_user_id);
        int64_t room_id_a = 0;
        if (sender_sess) {
            room_id_a = sender_sess->room_id;
        }
        
        char notify_json_b[512];
        snprintf(notify_json_b, sizeof(notify_json_b),
            "{\"user_id\": %lld, \"status\": \"%s\", \"room_id\": %ld}",
            (long long)from_user_id, status_a, room_id_a);
        
        session_manager_send_to_user(sess->user_id, CMD_NOTIFY_FRIEND_STATUS,
            notify_json_b, (uint32_t)strlen(notify_json_b));
        
        printf("[FRIENDS] Sent CMD_NOTIFY_FRIEND_STATUS to user %lld (accepter) about user %lld (sender)\n",
               (long long)sess->user_id, (long long)from_user_id);
        fflush(stdout);
    }
    
    protocol_send_simple_ok(sess, CMD_RES_RESPOND_FRIEND);
}

// Handle invite friend to room
static void handle_invite_friend(ClientSession *sess, const char *payload) {
    long long friend_id_ll = 0;
    long long room_id_ll = 0;
    
    if (!util_json_get_int64(payload, "friend_id", &friend_id_ll) || friend_id_ll <= 0) {
        protocol_send_error(sess, CMD_RES_INVITE_FRIEND, "INVALID_FRIEND_ID");
        return;
    }
    int64_t friend_id = (int64_t)friend_id_ll;
    
    // Get room_id from session or payload
    int64_t room_id = 0;
    if (util_json_get_int64(payload, "room_id", &room_id_ll) && room_id_ll > 0) {
        room_id = (int64_t)room_id_ll;
    } else if (sess->room_id > 0) {
        room_id = sess->room_id;
    } else {
        protocol_send_error(sess, CMD_RES_INVITE_FRIEND, "NOT_IN_ROOM");
        return;
    }
    
    // Check if they are friends
    bool are_friends = false;
    if (dao_friends_are_friends(sess->user_id, friend_id, &are_friends) != 0 || !are_friends) {
        protocol_send_error(sess, CMD_RES_INVITE_FRIEND, "NOT_FRIENDS");
        return;
    }
    
    // Check if friend is online
    ClientSession *friend_sess = session_manager_get_by_user_id(friend_id);
    if (!friend_sess) {
        protocol_send_error(sess, CMD_RES_INVITE_FRIEND, "FRIEND_OFFLINE");
        return;
    }
    
    // Get room info
    int64_t owner_id = 0;
    if (dao_rooms_get_owner(room_id, &owner_id) != 0) {
        protocol_send_error(sess, CMD_RES_INVITE_FRIEND, "ROOM_NOT_FOUND");
        return;
    }
    
    // Get sender username
    User sender;
    if (dao_users_find_by_id(sess->user_id, &sender) != 0) {
        protocol_send_error(sess, CMD_RES_INVITE_FRIEND, "USER_NOT_FOUND");
        return;
    }
    
    // Send invite notification to friend
    char invite_json[512];
    char *esc_username = util_json_escape(sender.username);
    if (!esc_username) esc_username = strdup("");
    
    snprintf(invite_json, sizeof(invite_json),
        "{\"from_user_id\": %ld, \"from_username\": \"%s\", \"room_id\": %ld}",
        sess->user_id, esc_username, room_id);
    
    session_manager_send_to_user(friend_id, CMD_NOTIFY_ROOM_INVITE,
        invite_json, (uint32_t)strlen(invite_json));
    
    free(esc_username);
    
    // Send success response to sender
    protocol_send_simple_ok(sess, CMD_RES_INVITE_FRIEND);
}

// Handle send DM
static void handle_send_dm(ClientSession *sess, const char *payload) {
    printf("[FRIENDS] === handle_send_dm ===\n");
    printf("[FRIENDS] sender user_id=%lld\n", (long long)sess->user_id);
    printf("[FRIENDS] payload=%s\n", payload ? payload : "(null)");
    fflush(stdout);
    
    long long to_user_id_ll = 0;
    
    if (!util_json_get_int64(payload, "to_user_id", &to_user_id_ll) || to_user_id_ll <= 0) {
        printf("[FRIENDS] ERROR: INVALID_TO_USER_ID\n");
        fflush(stdout);
        protocol_send_error(sess, CMD_RES_SEND_DM, "INVALID_TO_USER_ID");
        return;
    }
    int64_t to_user_id = (int64_t)to_user_id_ll;
    printf("[FRIENDS] to_user_id=%lld\n", (long long)to_user_id);
    fflush(stdout);
    
    char *message = util_json_get_string(payload, "message");
    if (!message) {
        printf("[FRIENDS] ERROR: MISSING_MESSAGE\n");
        fflush(stdout);
        protocol_send_error(sess, CMD_RES_SEND_DM, "MISSING_MESSAGE");
        return;
    }
    printf("[FRIENDS] message=%s\n", message);
    fflush(stdout);
    
    // Check if they are friends (optional, can be removed if DMs are open)
    bool are_friends = false;
    dao_friends_are_friends(sess->user_id, to_user_id, &are_friends);
    printf("[FRIENDS] are_friends=%d\n", are_friends);
    // For now, we allow DMs even if not friends
    
    // Check if user is logged in
    if (sess->user_id <= 0) {
        printf("[FRIENDS] ERROR: NOT_LOGGED_IN\n");
        fflush(stdout);
        protocol_send_error(sess, CMD_RES_SEND_DM, "NOT_LOGGED_IN");
        free(message);
        return;
    }
    
    // Get sender info
    User sender;
    if (dao_users_find_by_id(sess->user_id, &sender) != 0) {
        fprintf(stderr, "[FRIENDS] SENDER_NOT_FOUND: user_id=%lld\n", (long long)sess->user_id);
        protocol_send_error(sess, CMD_RES_SEND_DM, "SENDER_NOT_FOUND");
        free(message);
        return;
    }
    printf("[FRIENDS] sender username=%s\n", sender.username);
    fflush(stdout);
    
    // Build DM notification JSON
    char *esc_username = util_json_escape(sender.username);
    char *esc_message = util_json_escape(message);
    if (!esc_username) esc_username = strdup("");
    if (!esc_message) esc_message = strdup("");
    
    // Save message to database
    printf("[FRIENDS] Saving message to database...\n");
    fflush(stdout);
    if (dao_chat_send_dm(sess->user_id, to_user_id, message) != 0) {
        printf("[FRIENDS] ERROR: SAVE_MESSAGE_FAILED\n");
        fflush(stdout);
        free(esc_username);
        free(esc_message);
        free(message);
        protocol_send_error(sess, CMD_RES_SEND_DM, "SAVE_MESSAGE_FAILED");
        return;
    }
    printf("[FRIENDS] Message saved to database successfully\n");
    fflush(stdout);
    
    long timestamp = (long)time(NULL);
    char dm_json[2048];
    snprintf(dm_json, sizeof(dm_json),
        "{\"from_user_id\": %lld, \"from_username\": \"%s\", \"message\": \"%s\", \"timestamp\": %ld}",
        (long long)sess->user_id, esc_username, esc_message, timestamp);
    
    printf("[FRIENDS] DM JSON: %s\n", dm_json);
    fflush(stdout);
    
    // Try to send to receiver if online
    printf("[FRIENDS] Sending notification to receiver (user_id=%lld)...\n", (long long)to_user_id);
    fflush(stdout);
    int sent = session_manager_send_to_user(to_user_id, CMD_NOTIFY_DM,
        dm_json, (uint32_t)strlen(dm_json));
    printf("[FRIENDS] Notification sent to receiver: %d\n", sent);
    fflush(stdout);
    
    // Also send notification back to sender (echo) so they see their own message confirmed
    // This is especially useful if the receiver is offline
    printf("[FRIENDS] Sending echo to sender (user_id=%lld)...\n", (long long)sess->user_id);
    fflush(stdout);
    session_manager_send_to_user(sess->user_id, CMD_NOTIFY_DM,
        dm_json, (uint32_t)strlen(dm_json));
    printf("[FRIENDS] Echo sent to sender\n");
    fflush(stdout);
    
    // Always return success if saved to DB, even if user is offline
    protocol_send_simple_ok(sess, CMD_RES_SEND_DM);
    printf("[FRIENDS] Success response sent\n");
    fflush(stdout);
    
    free(esc_username);
    free(esc_message);
    free(message);
}

// Handle send room chat
static void handle_send_room_chat(ClientSession *sess, const char *payload) {
    char *message = util_json_get_string(payload, "message");
    if (!message) {
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "MISSING_MESSAGE");
        return;
    }

    size_t msg_len = strlen(message);
    if (msg_len == 0 || msg_len > ROOM_CHAT_MAX_LENGTH) {
        free(message);
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "MESSAGE_TOO_LONG");
        return;
    }
    
    // Get room_id from session or payload
    int64_t room_id = 0;
    long long room_id_ll = 0;
    if (util_json_get_int64(payload, "room_id", &room_id_ll) && room_id_ll > 0) {
        room_id = (int64_t)room_id_ll;
    } else if (sess->room_id > 0) {
        room_id = sess->room_id;
    } else {
        free(message);
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "NOT_IN_ROOM");
        return;
    }

    // Enforce membership: session must match the target room
    if (sess->room_id <= 0 || sess->room_id != room_id) {
        free(message);
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "NOT_IN_ROOM");
        return;
    }
    
    // Check if user is logged in
    if (sess->user_id <= 0) {
        free(message);
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "NOT_LOGGED_IN");
        return;
    }

    // Only allow chat when room is waiting
    room_status_t status = ROOM_STATUS_WAITING;
    if (dao_rooms_get_status(room_id, &status) != 0 || status != ROOM_STATUS_WAITING) {
        free(message);
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "ROOM_NOT_WAITING");
        return;
    }

    // Rate limit per user per room
    if (!room_chat_rate_allow(sess->user_id, room_id)) {
        free(message);
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "RATE_LIMITED");
        return;
    }
    
    // Get sender info
    User sender;
    if (dao_users_find_by_id(sess->user_id, &sender) != 0) {
        free(message);
        fprintf(stderr, "[FRIENDS] SENDER_NOT_FOUND: user_id=%lld\n", (long long)sess->user_id);
        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "SENDER_NOT_FOUND");
        return;
    }
    
    // Build room chat message JSON
    char *esc_username = util_json_escape(sender.username);
    char *esc_message = util_json_escape(message);
    if (!esc_username) esc_username = strdup("");
    if (!esc_message) esc_message = strdup("");
    
    char chat_json[2048];
    snprintf(chat_json, sizeof(chat_json),
        "{\"user_id\": %lld, \"username\": \"%s\", \"message\": \"%s\", \"timestamp\": %ld}",
        (long long)sess->user_id, esc_username, esc_message, (long)time(NULL));
    
    // Broadcast to all room members
    (void)session_manager_broadcast_to_room(room_id, CMD_NOTIFY_ROOM_CHAT,
        chat_json, (uint32_t)strlen(chat_json));
    
    // Return success after broadcasting
    protocol_send_simple_ok(sess, CMD_RES_SEND_ROOM_CHAT);
    
    free(esc_username);
    free(esc_message);
    free(message);
}

// Handle fetch offline messages
static void handle_fetch_offline(ClientSession *sess, const char *payload) {
    void *json_result = NULL;
    
    // Try to get friend_id from payload (optional - for conversation fetch)
    long long friend_id_ll = 0;
    bool has_friend_id = util_json_get_int64(payload, "friend_id", &friend_id_ll);
    
    if (has_friend_id && friend_id_ll > 0) {
        // Fetch conversation between user and friend (both sent and received messages)
        int64_t friend_id = (int64_t)friend_id_ll;
        if (dao_chat_fetch_conversation(sess->user_id, friend_id, &json_result) != 0) {
            protocol_send_error(sess, CMD_RES_FETCH_OFFLINE, "FETCH_CONVERSATION_FAILED");
            return;
        }
    } else {
        // Fallback: fetch all offline messages (old behavior)
        if (dao_chat_fetch_offline(sess->user_id, &json_result) != 0) {
            protocol_send_error(sess, CMD_RES_FETCH_OFFLINE, "FETCH_OFFLINE_FAILED");
            return;
        }
    }
    
    if (json_result) {
        char *json_str = (char *)json_result;
        protocol_send_response(sess, CMD_RES_FETCH_OFFLINE, json_str, (uint32_t)strlen(json_str));
        free(json_result);
    } else {
        // No messages
        protocol_send_response(sess, CMD_RES_FETCH_OFFLINE, "[]", 2);
    }
}

// Main dispatch function
void friends_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)payload_len; // Unused for now
    
    switch (cmd) {
        case CMD_REQ_SEARCH_USER:
            handle_search_user(sess, payload);
            break;
            
        case CMD_REQ_ADD_FRIEND:
            handle_add_friend(sess, payload);
            break;
            
        case CMD_REQ_REMOVE_FRIEND:
            handle_remove_friend(sess, payload);
            break;
            
        case CMD_REQ_LIST_FRIENDS:
            handle_list_friends(sess);
            break;
            
        case CMD_REQ_GET_FRIEND_INFO:
            handle_get_friend_info(sess, payload);
            break;
            
        case CMD_REQ_GET_PENDING_REQ:
            handle_get_pending_requests(sess);
            break;
            
        case CMD_REQ_RESPOND_FRIEND:
            handle_respond_friend(sess, payload);
            break;
            
        case CMD_REQ_INVITE_FRIEND:
            handle_invite_friend(sess, payload);
            break;
            
        case CMD_REQ_SEND_DM:
            handle_send_dm(sess, payload);
            break;
            
        case CMD_REQ_SEND_ROOM_CHAT:
            handle_send_room_chat(sess, payload);
            break;
            
        case CMD_REQ_FETCH_OFFLINE:
            handle_fetch_offline(sess, payload);
            break;
            
        default:
            protocol_send_error(sess, cmd, "UNKNOWN_FRIENDS_CMD");
            break;
    }
}

