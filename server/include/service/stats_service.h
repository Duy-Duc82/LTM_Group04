#ifndef STATS_SERVICE_H
#define STATS_SERVICE_H

#include "service/client_session.h"
#include <stdint.h>

void stats_handle_get_profile(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);
void stats_handle_leaderboard(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);
void stats_handle_match_history(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);
void stats_handle_get_onevn_history(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);
void stats_handle_get_replay_details(ClientSession *sess, uint16_t cmd, const char *payload, uint32_t payload_len);

#endif
