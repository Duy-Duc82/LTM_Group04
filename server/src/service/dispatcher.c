#include "service/dispatcher.h"
#include "service/commands.h"
#include "service/auth_service.h"
#include "service/quickmode_service.h"
#include "service/stats_service.h"
#include "service/friends_service.h"
#include "service/onevn_service.h"
// Nếu tách riêng friends/chat/room:
#include "dao/dao_friends.h"
#include "dao/dao_chat.h"
#include "dao/dao_rooms.h"
#include "service/protocol.h"
#include "service/session_manager.h"
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
            friends_dispatch(sess, cmd, payload, payload_len);
            break;

        case 0x03: // Chat
            friends_dispatch(sess, cmd, payload, payload_len);
            break;

        case 0x04: // Room
            switch (cmd) {
                case CMD_REQ_CREATE_ROOM: {
                    int64_t room_id;
                    if (dao_rooms_create(sess->user_id, &room_id) == 0) {
                        // Set room_id in session
                        session_manager_set_room(sess, room_id);
                        
                        // Get room members and send back with members list
                        void *members_json = NULL;
                        if (dao_rooms_get_members(room_id, &members_json) == 0) {
                            char buf[2048];
                            snprintf(buf, sizeof(buf), "{\"room_id\": %ld, \"members\": %s}", 
                                    room_id, (char*)members_json);
                            protocol_send_response(sess, CMD_RES_CREATE_ROOM, buf, strlen(buf));
                            free(members_json);
                        } else {
                            // Fallback
                            char buf[128];
                            snprintf(buf, sizeof(buf), "{\"room_id\": %ld}", room_id);
                            protocol_send_response(sess, CMD_RES_CREATE_ROOM, buf, strlen(buf));
                        }
                    } else {
                        protocol_send_error(sess, CMD_RES_CREATE_ROOM, "CREATE_ROOM_FAILED");
                    }
                } break;
                case CMD_REQ_JOIN_ROOM: {
                    long long room_id = 0; // parse
                    util_json_get_int64(payload, "room_id", &room_id);
                    if (dao_rooms_join(room_id, sess->user_id, 0) == 0) {
                        // Set room_id in session
                        session_manager_set_room(sess, (int64_t)room_id);
                        
                        // Get room members and send back
                        void *members_json = NULL;
                        if (dao_rooms_get_members(room_id, &members_json) == 0) {
                            char response_buf[2048];
                            snprintf(response_buf, sizeof(response_buf), 
                                "{\"room_id\": %lld, \"members\": %s}", 
                                room_id, (char*)members_json);
                            protocol_send_response(sess, CMD_RES_JOIN_ROOM, response_buf, strlen(response_buf));
                            
                            // Broadcast room update to all members in the room
                            char notify_buf[2048];
                            snprintf(notify_buf, sizeof(notify_buf), 
                                "{\"members\": %s}", (char*)members_json);
                            session_manager_broadcast_to_room(room_id, CMD_NOTIFY_ROOM_UPDATE, 
                                                              notify_buf, strlen(notify_buf));
                            
                            free(members_json);
                        } else {
                            // Fallback if can't get members
                            char response_buf[128];
                            snprintf(response_buf, sizeof(response_buf), 
                                "{\"room_id\": %lld, \"status\": \"success\"}", room_id);
                            protocol_send_response(sess, CMD_RES_JOIN_ROOM, response_buf, strlen(response_buf));
                        }
                    } else {
                        protocol_send_error(sess, CMD_RES_JOIN_ROOM, "JOIN_ROOM_FAILED");
                    }
                } break;
                case CMD_REQ_LEAVE_ROOM: {
                    long long room_id = 0; // parse
                    util_json_get_int64(payload, "room_id", &room_id);
                    if (dao_rooms_leave(room_id, sess->user_id) == 0) {
                        char response_buf[128];
                        snprintf(response_buf, sizeof(response_buf), 
                            "{\"room_id\": %lld, \"status\": \"left\"}", room_id);
                        protocol_send_response(sess, CMD_RES_LEAVE_ROOM, response_buf, strlen(response_buf));
                    } else {
                        protocol_send_error(sess, CMD_RES_LEAVE_ROOM, "LEAVE_ROOM_FAILED");
                    }
                } break;
                case CMD_REQ_LIST_ROOMS: {
                    void *rooms_json = NULL;
                    if (dao_rooms_list_waiting(&rooms_json) == 0) {
                        const char *json_str = (const char *)rooms_json;
                        protocol_send_response(sess, CMD_RES_LIST_ROOMS, json_str, strlen(json_str));
                        free(rooms_json);
                    } else {
                        protocol_send_error(sess, CMD_RES_LIST_ROOMS, "LIST_ROOMS_FAILED");
                    }
                } break;
                case CMD_REQ_START_GAME: {
                    // Start 1vN game - handled by onevn_service
                    onevn_dispatch(sess, cmd, payload, payload_len);
                } break;
                case CMD_REQ_INVITE_FRIEND:
                    friends_dispatch(sess, cmd, payload, payload_len);
                    break;
                default:
                    protocol_send_error(sess, cmd, "UNKNOWN_ROOM_CMD");
            }
            break;

        case 0x05: // Basic Mode – Quickmode
            quickmode_dispatch(sess, cmd, payload, payload_len);
            break;
        case 0x06: // 1vN MODE
            onevn_dispatch(sess, cmd, payload, payload_len);
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
                case CMD_REQ_UPDATE_AVATAR:
                    stats_handle_update_avatar(sess, cmd, payload, payload_len);
                    break;
                case CMD_REQ_GET_ONEVN_HISTORY:
                    stats_handle_get_onevn_history(sess, cmd, payload, payload_len);
                    break;
                case CMD_REQ_GET_REPLAY_DETAILS:
                    stats_handle_get_replay_details(sess, cmd, payload, payload_len);
                    break;
                default:
                    protocol_send_error(sess, cmd, "UNKNOWN_STATS_CMD");
            }
            break;

        default:
            protocol_send_error(sess, cmd, "UNKNOWN_CMD");
    }
}
