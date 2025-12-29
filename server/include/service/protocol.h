// header + mã lệnh + JSON structure
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "service/client_session.h"


typedef struct {
    uint16_t cmd;        // CommandType: 0x0101, 0x0103, ...
    uint16_t user_id;    // 0 nếu chưa login
    uint32_t length;     // độ dài payload (bytes)
} PacketHeader;

typedef struct {
    PacketHeader header;
    char payload[];      // dữ liệu theo định dạng JSON
} Packet;

// Protocol helpers used by services/dispatcher
// Sends a success response with a JSON payload
void protocol_send_response(ClientSession *sess, uint16_t cmd, const char *json, uint32_t len);

// Sends an error response for a command (payload will be small JSON {"error":"msg"})
void protocol_send_error(ClientSession *sess, uint16_t cmd, const char *error_msg);

// Sends a simple OK response without JSON body (payload length 0)
void protocol_send_simple_ok(ClientSession *sess, uint16_t cmd);

#endif


