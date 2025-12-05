#include "service/dispatcher.h"
#include "service/commands.h"
#include "service/auth_service.h"
#include "service/quickmode_service.h"
#include "service/stats_service.h"
// Nếu tách riêng friends/chat/room:
#include "dao/dao_friends.h"
#include "dao/dao_chat.h"
#include "dao/dao_rooms.h"
#include "service/protocol.h"
#include "utils/json.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>




void dispatcher_handle_packet(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    uint8_t major = (cmd & 0xFF00) >> 8;
    // uint8_t minor = cmd & 0x00FF;

    switch (major) {
        case 0x01: // Auth
            auth_dispatch(sess, cmd, payload, payload_len);
            break;

        case 0x02: // Friends
            switch (cmd) {
                case CMD_REQ_ADD_FRIEND: {
                    // parse payload: { "friend_id": 123 }
                    long long friend_id = 0;
                    util_json_get_int64(payload, "friend_id", &friend_id);
                    if (dao_friends_send_request(sess->user_id, friend_id) == 0) {
                        protocol_send_simple_ok(sess, CMD_RES_ADD_FRIEND);
                    } else {
                        protocol_send_error(sess, CMD_RES_ADD_FRIEND, "ADD_FRIEND_FAILED");
                    }
                } break;
                case CMD_REQ_RESPOND_FRIEND: {
                    long long from_user = 0;
                    long long accept_ll = 0;
                    util_json_get_int64(payload, "from_user", &from_user);
                    util_json_get_int64(payload, "accept", &accept_ll);
                    int accept = accept_ll ? 1 : 0;
                    if (dao_friends_respond_request(from_user, sess->user_id, accept) == 0) {
                        protocol_send_simple_ok(sess, CMD_RES_RESPOND_FRIEND);
                    } else {
                        protocol_send_error(sess, CMD_RES_RESPOND_FRIEND, "RESPOND_FRIEND_FAILED");
                    }
                } break;
                case CMD_REQ_LIST_FRIENDS: {
                    void *json_list = NULL;
                    if (dao_friends_list(sess->user_id, &json_list) != 0) {
                        protocol_send_error(sess, CMD_RES_LIST_FRIENDS, "LIST_FRIENDS_FAILED");
                    } else {
                        const char *json_str = (const char *)json_list;
                        protocol_send_response(sess, CMD_RES_LIST_FRIENDS, json_str, strlen(json_str));
                        free(json_list);
                    }
                } break;
                default:
                    protocol_send_error(sess, cmd, "UNKNOWN_FRIENDS_CMD");
            }
            break;

        case 0x03: // Chat
            switch (cmd) {
                case CMD_REQ_SEND_DM: {
                    long long to_user = 0;
                    util_json_get_int64(payload, "to_user", &to_user);
                    char *msg = util_json_get_string(payload, "content");
                    if (dao_chat_send_dm(sess->user_id, to_user, msg) == 0) {
                        protocol_send_simple_ok(sess, CMD_RES_SEND_DM);
                        // Đồng thời push NOTIFY_DM cho người nhận (nếu online)
                        // server_broadcast_dm(to_user, sess->user_id, msg);
                    } else {
                        protocol_send_error(sess, CMD_RES_SEND_DM, "SEND_DM_FAILED");
                    }
                    if (msg) free(msg);
                } break;
                case CMD_REQ_SEND_ROOM_CHAT: {
                    long long room_id = 0;
                    util_json_get_int64(payload, "room_id", &room_id);
                    char *msg = util_json_get_string(payload, "content");
                    if (dao_chat_send_room(sess->user_id, room_id, msg) == 0) {
                        protocol_send_simple_ok(sess, CMD_RES_SEND_ROOM_CHAT);
                        // broadcast NOTIFY_ROOM_CHAT cho các member trong room
                    } else {
                        protocol_send_error(sess, CMD_RES_SEND_ROOM_CHAT, "SEND_ROOM_CHAT_FAILED");
                    }
                    if (msg) free(msg);
                } break;
                case CMD_REQ_FETCH_OFFLINE: {
                    void *json_msgs = NULL;
                    if (dao_chat_fetch_offline(sess->user_id, &json_msgs) != 0) {
                        protocol_send_error(sess, CMD_RES_FETCH_OFFLINE, "FETCH_OFFLINE_FAILED");
                    } else {
                        const char *json_str = (const char *)json_msgs;
                        protocol_send_response(sess, CMD_RES_FETCH_OFFLINE, json_str, strlen(json_str));
                        dao_chat_mark_read(sess->user_id);
                        free(json_msgs);
                    }
                } break;
                default:
                    protocol_send_error(sess, cmd, "UNKNOWN_CHAT_CMD");
            }
            break;

        case 0x04: // Room
            switch (cmd) {
                case CMD_REQ_CREATE_ROOM: {
                    int64_t room_id;
                    room_mode_t mode = ROOM_MODE_ONEVN; // hoặc parse từ payload
                    if (dao_rooms_create(sess->user_id, mode, &room_id) == 0) {
                        // trả về info room
                        char buf[128];
                        snprintf(buf, sizeof(buf), "{\"room_id\": %ld}", room_id);
                        protocol_send_response(sess, CMD_RES_CREATE_ROOM, buf, strlen(buf));
                    } else {
                        protocol_send_error(sess, CMD_RES_CREATE_ROOM, "CREATE_ROOM_FAILED");
                    }
                } break;
                case CMD_REQ_JOIN_ROOM: {
                    long long room_id = 0; // parse
                    util_json_get_int64(payload, "room_id", &room_id);
                    if (dao_rooms_join(room_id, sess->user_id, 0) == 0) {
                        protocol_send_simple_ok(sess, CMD_RES_JOIN_ROOM);
                        // broadcast NOTIFY_ROOM_UPDATE cho các member
                    } else {
                        protocol_send_error(sess, CMD_RES_JOIN_ROOM, "JOIN_ROOM_FAILED");
                    }
                } break;
                case CMD_REQ_LEAVE_ROOM: {
                    long long room_id = 0; // parse
                    util_json_get_int64(payload, "room_id", &room_id);
                    if (dao_rooms_leave(room_id, sess->user_id) == 0) {
                        protocol_send_simple_ok(sess, CMD_RES_LEAVE_ROOM);
                    } else {
                        protocol_send_error(sess, CMD_RES_LEAVE_ROOM, "LEAVE_ROOM_FAILED");
                    }
                } break;
                case CMD_REQ_START_GAME: {
                    // update status room, gọi quickmode/1vN
                    // dao_rooms_update_status(room_id, ROOM_STATUS_PLAYING);
                    // rồi quickmode_start_1vN(...)
                } break;
                default:
                    protocol_send_error(sess, cmd, "UNKNOWN_ROOM_CMD");
            }
            break;

        case 0x05: // Basic Mode – bạn đã làm phần quickmode_service
        case 0x06: // 1vN MODE – sau này bạn mở rộng thêm
            quickmode_dispatch(sess, cmd, payload, payload_len);
            break;

        case 0x07: // Stats
            switch (cmd) {
                case CMD_REQ_GET_PROFILE:
                    stats_handle_get_profile(sess, cmd, payload, payload_len);
                    break;
                case CMD_REQ_LEADERBOARD:
                    stats_handle_leaderboard(sess, cmd, payload, payload_len);
                    break;
                case CMD_REQ_MATCH_HISTORY:
                    stats_handle_match_history(sess, cmd, payload, payload_len);
                    break;
                default:
                    protocol_send_error(sess, cmd, "UNKNOWN_STATS_CMD");
            }
            break;

        default:
            protocol_send_error(sess, cmd, "UNKNOWN_CMD");
    }
}
