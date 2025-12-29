// Session manager for handling multiple client connections
#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "service/client_session.h"
#include <sys/epoll.h>

#define MAX_EPOLL_EVENTS 64
#define MAX_SESSIONS 1024

typedef struct SessionManager {
	ClientSession **sessions;  // Array of session pointers
	int max_sessions;
	int session_count;
	int epoll_fd;
} SessionManager;

// Create/destroy session manager
SessionManager *session_manager_new(int max_sessions);
void session_manager_free(SessionManager *mgr);

// Add/remove sessions
int session_manager_add(SessionManager *mgr, ClientSession *sess);
int session_manager_remove(SessionManager *mgr, int socket_fd);
ClientSession *session_manager_get_by_fd(SessionManager *mgr, int socket_fd);

// Epoll management
int session_manager_epoll_add(SessionManager *mgr, int fd, uint32_t events);
int session_manager_epoll_modify(SessionManager *mgr, int fd, uint32_t events);
int session_manager_epoll_remove(SessionManager *mgr, int fd);

// Get epoll fd for waiting
int session_manager_get_epoll_fd(SessionManager *mgr);

// Get count
int session_manager_count(SessionManager *mgr);

// Update session's room_id
void session_manager_set_room(ClientSession *sess, int64_t room_id);

// Get session by user_id
ClientSession *session_manager_get_by_user_id(int64_t user_id);

// Send message to a specific user (if online)
int session_manager_send_to_user(int64_t user_id, uint16_t cmd, const char *json, uint32_t json_len);

// Broadcast message to all sessions in a room
int session_manager_broadcast_to_room(int64_t room_id, uint16_t cmd, const char *json, uint32_t json_len);

// Set/get global session manager instance
void session_manager_set_global(SessionManager *mgr);
SessionManager *session_manager_get_global(void);

#endif
