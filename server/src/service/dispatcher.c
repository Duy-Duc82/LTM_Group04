#include "service/dispatcher.h"
#include "service/auth_service.h"
#include "service/quickmode_service.h"
#include "service/stats_service.h"
// Nếu tách riêng friends/chat/room:
#include "dao/dao_friends.h"
#include "dao/dao_chat.h"
#include "dao/dao_rooms.h"
#include "service/protocol.h"

void dispatcher_handle_packet(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len) {
    uint8_t major = (cmd & 0xFF00) >> 8;
    // uint8_t minor = cmd & 0x00FF;

    switch (major) {
        case 0x01: // Auth
            auth_dispatch(sess, cmd, payload, payload_len);
            break;

        case 0x02: // Friends
            // Ví dụ handle nhanh trong dispatcher
            switch (cmd) {
                case 0x0201: { // REQ_ADD_FRIEND
                    // parse payload: { "friend_id": 123 }
                    int64_t friend_id = /* TODO parse */ 0;
                    if (dao_friends_send_request(sess->user_id, friend_id) == 0) {
                        protocol_send_simple_ok(sess, 0x0202); // RES_ADD_FRIEND
                    } else {
                        protocol_send_error(sess, 0x0202, "ADD_FRIEND_FAILED");
                    }
                } break;
                case 0x0204: { // REQ_RESPOND_FRIEND
                    int64_t from_user = /* parse */ 0;
                    int accept = /* parse */ 1;
                    if (dao_friends_respond_request(from_user, sess->user_id, accept) == 0) {
                        protocol_send_simple_ok(sess, 0x0205); // RES_RESPOND_FRIEND
                    } else {
                        protocol_send_error(sess, 0x0205, "RESPOND_FRIEND_FAILED");
                    }
                } break;
                case 0x0206: { // REQ_LIST_FRIENDS
                    void *json_list = NULL;
                    if (dao_friends_list(sess->user_id, &json_list) != 0) {
                        protocol_send_error(sess, 0x0207, "LIST_FRIENDS_FAILED");
                    } else {
                        const char *json_str = (const char *)json_list;
                        protocol_send_response(sess, 0x0207, json_str, strlen(json_str));
                        free(json_list);
                    }
                } break;
                default:
                    protocol_send_error(sess, cmd, "UNKNOWN_FRIENDS_CMD");
            }
            break;

        case 0x03: // Chat
            switch (cmd) {
                case 0x0301: { // REQ_SEND_DM
                    int64_t to_user = 0;
                    const char *msg = NULL;
                    // TODO parse JSON
                    if (dao_chat_send_dm(sess->user_id, to_user, msg) == 0) {
                        protocol_send_simple_ok(sess, 0x0302); // RES_SEND_DM
                        // Đồng thời push NOTIFY_DM cho người nhận (nếu online)
                        // server_broadcast_dm(to_user, sess->user_id, msg);
                    } else {
                        protocol_send_error(sess, 0x0302, "SEND_DM_FAILED");
                    }
                } break;
                case 0x0304: { // REQ_SEND_ROOM_CHAT
                    int64_t room_id = 0;
                    const char *msg = NULL;
                    // TODO parse JSON
                    if (dao_chat_send_room(sess->user_id, room_id, msg) == 0) {
                        protocol_send_simple_ok(sess, 0x0305);
                        // broadcast NOTIFY_ROOM_CHAT cho các member trong room
                    } else {
                        protocol_send_error(sess, 0x0305, "SEND_ROOM_CHAT_FAILED");
                    }
                } break;
                case 0x0307: { // REQ_FETCH_OFFLINE
                    void *json_msgs = NULL;
                    if (dao_chat_fetch_offline(sess->user_id, &json_msgs) != 0) {
                        protocol_send_error(sess, 0x0308, "FETCH_OFFLINE_FAILED");
                    } else {
                        const char *json_str = (const char *)json_msgs;
                        protocol_send_response(sess, 0x0308, json_str, strlen(json_str));
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
                case 0x0401: { // REQ_CREATE_ROOM
                    int64_t room_id;
                    room_mode_t mode = ROOM_MODE_ONEVN; // hoặc parse từ payload
                    if (dao_rooms_create(sess->user_id, mode, &room_id) == 0) {
                        // trả về info room
                        char buf[128];
                        snprintf(buf, sizeof(buf), "{\"room_id\": %ld}", room_id);
                        protocol_send_response(sess, 0x0402, buf, strlen(buf)); // RES_CREATE_ROOM
                    } else {
                        protocol_send_error(sess, 0x0402, "CREATE_ROOM_FAILED");
                    }
                } break;
                case 0x0403: { // REQ_JOIN_ROOM
                    int64_t room_id = 0; // parse
                    if (dao_rooms_join(room_id, sess->user_id, 0) == 0) {
                        protocol_send_simple_ok(sess, 0x0404);
                        // broadcast NOTIFY_ROOM_UPDATE cho các member
                    } else {
                        protocol_send_error(sess, 0x0404, "JOIN_ROOM_FAILED");
                    }
                } break;
                case 0x040A: { // REQ_LEAVE_ROOM
                    int64_t room_id = 0; // parse
                    if (dao_rooms_leave(room_id, sess->user_id) == 0) {
                        protocol_send_simple_ok(sess, 0x040B);
                    } else {
                        protocol_send_error(sess, 0x040B, "LEAVE_ROOM_FAILED");
                    }
                } break;
                case 0x040C: { // REQ_START_GAME
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
                case 0x0701: // REQ_GET_PROFILE
                    stats_handle_get_profile(sess, cmd, payload, payload_len);
                    break;
                case 0x0703: // REQ_LEADERBOARD
                    stats_handle_leaderboard(sess, cmd, payload, payload_len);
                    break;
                case 0x0705: // REQ_MATCH_HISTORY
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
