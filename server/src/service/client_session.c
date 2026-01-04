// Simple client session helpers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "service/client_session.h"
#include "service/protocol.h"

ClientSession *client_session_new(int socket_fd) {
    ClientSession *s = calloc(1, sizeof(ClientSession));
    if (!s) return NULL;
    s->socket_fd = socket_fd;
    s->user_id = 0;
    s->room_id = 0;
    s->status = USER_STATUS_ONLINE;  // Initialize status
    s->access_token[0] = '\0';
    s->read_buffer_len = 0;
    s->expected_len = 0;
    s->pending_cmd = 0;
    return s;
}

void client_session_free(ClientSession *sess) {
    if (!sess) return;
    // If we had ownership of the socket we could close it here;
    // keep it simple: do not close socket here (caller may manage it).
    free(sess);
}

ssize_t client_session_send(ClientSession *sess, const void *buf, size_t len) {
    if (!sess || !buf) return -1;
    if (sess->socket_fd >= 0) {
        ssize_t n = send(sess->socket_fd, buf, len, 0);
        return n;
    }
    // If no socket is present (e.g. unit tests), fall back to stdout
    size_t written = fwrite(buf, 1, len, stdout);
    fflush(stdout);
    return (ssize_t)written;
}

int client_session_read_packet(ClientSession *sess, uint16_t *cmd, char **payload, uint32_t *payload_len) {
	if (!sess || sess->socket_fd < 0) return -1;

	// Read available data into buffer
	ssize_t n = recv(sess->socket_fd, 
	                 sess->read_buffer + sess->read_buffer_len,
	                 READ_BUFFER_SIZE - sess->read_buffer_len, 0);

	if (n <= 0) {
		// Connection closed or error
		return -1;
	}

	sess->read_buffer_len += n;

	// If we haven't read the header yet, try to read it
	if (sess->expected_len == 0) {
		if (sess->read_buffer_len < sizeof(PacketHeader)) {
			// Need more data for header
			return 0;
		}

		// Parse header
		PacketHeader *hdr = (PacketHeader *)sess->read_buffer;
		sess->pending_cmd = ntohs(hdr->cmd);
		sess->expected_len = sizeof(PacketHeader) + ntohl(hdr->length);

		if (sess->expected_len > READ_BUFFER_SIZE) {
			// Packet too large
			return -1;
		}
	}

	// Check if we have complete packet
	if (sess->read_buffer_len < sess->expected_len) {
		// Need more data
		return 0;
	}

	// Complete packet received
	PacketHeader *hdr = (PacketHeader *)sess->read_buffer;
	*cmd = sess->pending_cmd;
	uint32_t plen = ntohl(hdr->length);
	*payload_len = plen;

	if (plen > 0) {
		*payload = malloc(plen + 1);
		if (!*payload) return -1;
		memcpy(*payload, sess->read_buffer + sizeof(PacketHeader), plen);
		(*payload)[plen] = '\0';
	} else {
		*payload = NULL;
	}

	// Reset for next packet
	sess->read_buffer_len = 0;
	sess->expected_len = 0;
	sess->pending_cmd = 0;

	return 1;
}
