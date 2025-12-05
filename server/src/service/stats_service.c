#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "service/client_session.h"
#include "service/commands.h"
#include "service/protocol.h"
#include "dao/dao_stats.h"
#include "service/stats_service.h"

// Giả sử payload JSON kiểu: { "user_id": 123 }
// Còn nếu bạn store user_id trong session thì có thể bỏ đọc payload.
void stats_handle_get_profile(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    (void)cmd;
    int64_t user_id = sess->user_id; // hoặc parse từ payload

    void *json_profile = NULL;
    if (dao_stats_get_profile(user_id, &json_profile) != 0) {
        protocol_send_error(sess, CMD_RES_GET_PROFILE, "GET_PROFILE_FAILED");
        return;
    }

    const char *json_str = (const char *)json_profile; // tuỳ bạn build kiểu gì
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
