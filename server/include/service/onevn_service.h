// server/include/service/onevn_service.h
#ifndef ONEVN_SERVICE_H
#define ONEVN_SERVICE_H

#include <stdint.h>
#include "service/client_session.h"

// Dispatch 1vN-related commands (0x06xx)
void onevn_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);

#endif

