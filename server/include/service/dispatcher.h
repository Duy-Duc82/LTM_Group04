// route command -> handler function
#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <stdint.h>
#include "service/client_session.h"

void dispatcher_handle_packet(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);

#endif
