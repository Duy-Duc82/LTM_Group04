// client session state for a single TCP connection
#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#define READ_BUFFER_SIZE 8192

typedef struct ClientSession {
	int socket_fd;             // socket file descriptor, -1 if unused
	int64_t user_id;           // authenticated user id (0 if not logged in)
	char access_token[65];     // if authenticated: token (NULL-terminated)
	int64_t room_id;           // current room id (0 if not in room)
	
	// Buffer for partial packet reads
	char read_buffer[READ_BUFFER_SIZE];
	size_t read_buffer_len;    // Current bytes in buffer
	size_t expected_len;       // Expected total packet length (0 = reading header)
	uint16_t pending_cmd;      // Command from header (if header read)
} ClientSession;

// create/free
ClientSession *client_session_new(int socket_fd);
void client_session_free(ClientSession *sess);

// send raw bytes to client (returns number of bytes sent or -1)
ssize_t client_session_send(ClientSession *sess, const void *buf, size_t len);

// Read packet from socket (handles partial reads)
// Returns: 1 if complete packet read, 0 if more data needed, -1 on error
int client_session_read_packet(ClientSession *sess, uint16_t *cmd, char **payload, uint32_t *payload_len);

#endif
