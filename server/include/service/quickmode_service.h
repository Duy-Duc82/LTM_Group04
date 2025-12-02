// server/include/service/quickmode_service.h
#ifndef QUICKMODE_SERVICE_H
#define QUICKMODE_SERVICE_H

#include <stdint.h>
#include "service/client_session.h"

// Hàm test: tạo session + 1 round và in câu hỏi ra console
int qm_debug_start(int64_t user_id);

// Dispatch quickmode-related commands
void quickmode_dispatch(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);

#endif
