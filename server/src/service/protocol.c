// Minimal protocol helpers used by services/dispatcher to send responses
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "service/protocol.h"
#include "service/client_session.h"

void protocol_send_response(ClientSession *sess, uint16_t cmd, const char *json, uint32_t len) {
	if (!sess) {
		// no session: just log
		fprintf(stderr, "[PROTOCOL] no session, cmd=0x%04x\n", cmd);
		return;
	}

	size_t hdr_sz = sizeof(PacketHeader);
	size_t total = hdr_sz + (size_t)len;
	char *buf = malloc(total);
	if (!buf) return;

	PacketHeader hdr;
	hdr.cmd = htons(cmd);
	hdr.user_id = htons((uint16_t)(sess->user_id & 0xFFFF));
	hdr.length = htonl(len);

	memcpy(buf, &hdr, hdr_sz);
	if (len && json) memcpy(buf + hdr_sz, json, len);

	// send via session
	client_session_send(sess, buf, total);
	free(buf);
}

void protocol_send_error(ClientSession *sess, uint16_t cmd, const char *error_msg) {
	// Build small JSON {"error":"..."}
	char tmp[512];
	if (!error_msg) error_msg = "unknown";
	int n = snprintf(tmp, sizeof(tmp), "{\"error\": \"%s\"}", error_msg);
	if (n < 0) return;
	protocol_send_response(sess, cmd, tmp, (uint32_t)n);
}

void protocol_send_simple_ok(ClientSession *sess, uint16_t cmd) {
	protocol_send_response(sess, cmd, NULL, 0);
}
