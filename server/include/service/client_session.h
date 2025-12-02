// client session state for a single TCP connection
#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

typedef struct ClientSession {
	int socket_fd;             // socket file descriptor, -1 if unused
	int64_t user_id;           // authenticated user id (0 if not logged in)
	char access_token[65];     // if authenticated: token (NULL-terminated)
	// future: buffers, heartbeat, room_id, roles ...
} ClientSession;

// create/free
ClientSession *client_session_new(int socket_fd);
void client_session_free(ClientSession *sess);

// send raw bytes to client (returns number of bytes sent or -1)
ssize_t client_session_send(ClientSession *sess, const void *buf, size_t len);

#endif
