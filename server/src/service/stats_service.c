#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "service/client_session.h"
#include "service/commands.h"
#include "service/protocol.h"
#include "dao/dao_stats.h"
#include "dao/dao_users.h"
#include "dao/dao_onevn.h"
#include "service/stats_service.h"
#include "utils/json.h"

// Giả sử payload JSON kiểu: { "user_id": 123 }
// Còn nếu bạn store user_id trong session thì có thể bỏ đọc payload.
void stats_handle_get_profile(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)cmd;
    int64_t user_id = sess->user_id; // hoặc parse từ payload

    if (user_id <= 0) {
        fprintf(stderr, "[STATS] Invalid user_id: %ld\n", (long)user_id);
        protocol_send_error(sess, CMD_RES_GET_PROFILE, "INVALID_USER_ID");
        return;
    }

    fprintf(stderr, "[STATS] Getting profile for user_id: %ld\n", (long)user_id);
    void *json_profile = NULL;
    if (dao_stats_get_profile(user_id, &json_profile) != 0) {
        fprintf(stderr, "[STATS] dao_stats_get_profile failed for user_id: %ld\n", (long)user_id);
        protocol_send_error(sess, CMD_RES_GET_PROFILE, "GET_PROFILE_FAILED");
        return;
    }

    if (!json_profile) {
        fprintf(stderr, "[STATS] json_profile is NULL\n");
        protocol_send_error(sess, CMD_RES_GET_PROFILE, "GET_PROFILE_FAILED");
        return;
    }

    const char *json_str = (const char *)json_profile;
    fprintf(stderr, "[STATS] Sending profile response: %s\n", json_str);
    protocol_send_response(sess, CMD_RES_GET_PROFILE, json_str, strlen(json_str));
    free(json_profile);
}

void stats_handle_leaderboard(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)cmd; (void)payload; (void)payload_len;
    void *json_leaderboard = NULL;
    if (dao_stats_get_leaderboard(20, &json_leaderboard) != 0) {
        protocol_send_error(sess, CMD_RES_LEADERBOARD, "LEADERBOARD_FAILED");
        return;
    }
    const char *json_str = (const char *)json_leaderboard;
    protocol_send_response(sess, CMD_RES_LEADERBOARD, json_str, strlen(json_str));
    free(json_leaderboard);
}

void stats_handle_match_history(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)cmd; (void)payload; (void)payload_len;
    int64_t user_id = sess->user_id;
    void *json_history = NULL;
    if (dao_stats_get_match_history(user_id, &json_history) != 0) {
        protocol_send_error(sess, CMD_RES_MATCH_HISTORY, "MATCH_HISTORY_FAILED");
        return;
    }
    const char *json_str = (const char *)json_history;
    protocol_send_response(sess, CMD_RES_MATCH_HISTORY, json_str, strlen(json_str));
    free(json_history);
}

void stats_handle_update_avatar(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)cmd;
    int64_t user_id = sess->user_id;

    if (user_id <= 0) {
        fprintf(stderr, "[STATS] Invalid user_id: %ld\n", (long)user_id);
        protocol_send_error(sess, CMD_RES_UPDATE_AVATAR, "INVALID_USER_ID");
        return;
    }

    // Parse JSON payload: { "avatar_path": "C:/path/to/image.jpg" }
    char *avatar_path = NULL;
    if (payload && payload_len > 0) {
        avatar_path = util_json_get_string(payload, "avatar_path");
    }

    if (!avatar_path || strlen(avatar_path) == 0) {
        fprintf(stderr, "[STATS] Invalid avatar_path in payload\n");
        if (avatar_path) free(avatar_path);
        protocol_send_error(sess, CMD_RES_UPDATE_AVATAR, "INVALID_AVATAR_PATH");
        return;
    }

    fprintf(stderr, "[STATS] Updating avatar for user_id: %ld, path: %s\n", (long)user_id, avatar_path);

    if (dao_users_update_avatar(user_id, avatar_path) != 0) {
        fprintf(stderr, "[STATS] dao_users_update_avatar failed for user_id: %ld\n", (long)user_id);
        free(avatar_path);
        protocol_send_error(sess, CMD_RES_UPDATE_AVATAR, "UPDATE_AVATAR_FAILED");
        return;
    }

    // Send success response
    char response[256];
    snprintf(response, sizeof(response), "{\"success\": true, \"avatar_path\": \"%s\"}", avatar_path);
    protocol_send_response(sess, CMD_RES_UPDATE_AVATAR, response, strlen(response));
    free(avatar_path);
}

void stats_handle_get_onevn_history(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)cmd; (void)payload; (void)payload_len;
    int64_t user_id = sess->user_id;
    void *json_history = NULL;
    if (dao_onevn_get_user_history(user_id, &json_history) != 0) {
        protocol_send_error(sess, CMD_RES_GET_ONEVN_HISTORY, "ONEVN_HISTORY_FAILED");
        return;
    }
    const char *json_str = (const char *)json_history;
    protocol_send_response(sess, CMD_RES_GET_ONEVN_HISTORY, json_str, strlen(json_str));
    free(json_history);
}

void stats_handle_get_replay_details(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)cmd;
    long long session_id_ll = 0;
    util_json_get_int64(payload, "session_id", &session_id_ll);
    int64_t session_id = (int64_t)session_id_ll;
    
    if (session_id <= 0) {
        protocol_send_error(sess, CMD_RES_GET_REPLAY_DETAILS, "INVALID_SESSION_ID");
        return;
    }
    
    void *json_replay = NULL;
    if (dao_onevn_get_replay_details(session_id, &json_replay) != 0) {
        protocol_send_error(sess, CMD_RES_GET_REPLAY_DETAILS, "REPLAY_DETAILS_FAILED");
        return;
    }
    const char *json_str = (const char *)json_replay;
    protocol_send_response(sess, CMD_RES_GET_REPLAY_DETAILS, json_str, strlen(json_str));
    free(json_replay);
}
