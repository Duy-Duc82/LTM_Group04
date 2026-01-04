// Session manager implementation
#include "service/session_manager.h"
#include "service/protocol.h"
#include "service/commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <stdint.h>
#include <stdbool.h>

SessionManager *session_manager_new(int max_sessions) {
	SessionManager *mgr = calloc(1, sizeof(SessionManager));
	if (!mgr) return NULL;

	mgr->max_sessions = max_sessions;
	mgr->sessions = calloc(max_sessions, sizeof(ClientSession*));
	
	if (!mgr->sessions) {
		free(mgr->sessions);
		free(mgr);
		return NULL;
	}

	// Create epoll instance
	mgr->epoll_fd = epoll_create1(0);
	if (mgr->epoll_fd < 0) {
		free(mgr->sessions);
		free(mgr);
		return NULL;
	}

	mgr->session_count = 0;
	return mgr;
}

void session_manager_free(SessionManager *mgr) {
	if (!mgr) return;

	// Close all sessions
	for (int i = 0; i < mgr->max_sessions; i++) {
		if (mgr->sessions[i]) {
			if (mgr->sessions[i]->socket_fd >= 0) {
				close(mgr->sessions[i]->socket_fd);
			}
			client_session_free(mgr->sessions[i]);
		}
	}

	if (mgr->epoll_fd >= 0) {
		close(mgr->epoll_fd);
	}

	free(mgr->sessions);
	free(mgr);
}

int session_manager_add(SessionManager *mgr, ClientSession *sess) {
	if (!mgr || !sess || sess->socket_fd < 0) return -1;
	if (mgr->session_count >= mgr->max_sessions) return -1;

	// Find free slot
	int index = -1;
	for (int i = 0; i < mgr->max_sessions; i++) {
		if (mgr->sessions[i] == NULL) {
			index = i;
			break;
		}
	}

	if (index < 0) return -1;

	// Add to epoll - store session pointer in epoll_event.data.ptr
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET; // Edge-triggered, read events
	ev.data.ptr = sess; // Store session pointer directly

	if (epoll_ctl(mgr->epoll_fd, EPOLL_CTL_ADD, sess->socket_fd, &ev) < 0) {
		return -1;
	}

	// Store session
	mgr->sessions[index] = sess;
	mgr->session_count++;

	return 0;
}

int session_manager_remove(SessionManager *mgr, int socket_fd) {
	if (!mgr || socket_fd < 0) return -1;

	// Find session by socket_fd
	ClientSession *sess = NULL;
	int index = -1;
	for (int i = 0; i < mgr->max_sessions; i++) {
		if (mgr->sessions[i] && mgr->sessions[i]->socket_fd == socket_fd) {
			sess = mgr->sessions[i];
			index = i;
			break;
		}
	}

	if (!sess || index < 0) return -1;

	// Remove from epoll
	epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);

	// Free session
	client_session_free(sess);
	mgr->sessions[index] = NULL;
	mgr->session_count--;

	return 0;
}

int session_manager_remove_by_user_id(SessionManager *mgr, int64_t user_id, ClientSession *exclude_sess) {
	if (!mgr || user_id <= 0) return -1;

	int removed_count = 0;
	
	// Find and remove all sessions with this user_id (except exclude_sess)
	for (int i = 0; i < mgr->max_sessions; i++) {
		if (mgr->sessions[i] && 
		    mgr->sessions[i]->user_id == user_id &&
		    mgr->sessions[i] != exclude_sess) {
			
			ClientSession *sess = mgr->sessions[i];
			int socket_fd = sess->socket_fd;
			
			// Remove from epoll
			epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
			
			// Free session
			client_session_free(sess);
			mgr->sessions[i] = NULL;
			mgr->session_count--;
			removed_count++;
		}
	}

	return removed_count;
}

ClientSession *session_manager_get_by_fd(SessionManager *mgr, int socket_fd) {
	if (!mgr || socket_fd < 0) return NULL;

	// Find session by socket_fd
	for (int i = 0; i < mgr->max_sessions; i++) {
		if (mgr->sessions[i] && mgr->sessions[i]->socket_fd == socket_fd) {
			return mgr->sessions[i];
		}
	}

	return NULL;
}

int session_manager_epoll_add(SessionManager *mgr, int fd, uint32_t events) {
	if (!mgr || fd < 0) return -1;

	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd; // For server socket, we use fd

	return epoll_ctl(mgr->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int session_manager_epoll_modify(SessionManager *mgr, int fd, uint32_t events) {
	if (!mgr || fd < 0) return -1;

	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	return epoll_ctl(mgr->epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

int session_manager_epoll_remove(SessionManager *mgr, int fd) {
	if (!mgr || fd < 0) return -1;
	return epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

int session_manager_get_epoll_fd(SessionManager *mgr) {
	return mgr ? mgr->epoll_fd : -1;
}

int session_manager_count(SessionManager *mgr) {
	return mgr ? mgr->session_count : 0;
}

// Global session manager instance
static SessionManager *g_session_manager = NULL;

void session_manager_set_global(SessionManager *mgr) {
	g_session_manager = mgr;
}

SessionManager *session_manager_get_global(void) {
	return g_session_manager;
}

void session_manager_set_room(ClientSession *sess, int64_t room_id) {
	if (sess) {
		sess->room_id = room_id;
	}
}

ClientSession *session_manager_get_by_user_id(int64_t user_id) {
	if (!g_session_manager || user_id <= 0) {
		printf("[SESSION_MGR] get_by_user_id: g_session_manager=%p, user_id=%lld\n",
		       (void*)g_session_manager, (long long)user_id);
		fflush(stdout);
		return NULL;
	}
	
	printf("[SESSION_MGR] Searching for user_id=%lld in %d sessions\n",
	       (long long)user_id, g_session_manager->max_sessions);
	fflush(stdout);
	
	for (int i = 0; i < g_session_manager->max_sessions; i++) {
		if (g_session_manager->sessions[i]) {
			printf("[SESSION_MGR] session[%d]: user_id=%lld, room_id=%lld\n",
			       i, (long long)g_session_manager->sessions[i]->user_id,
			       (long long)g_session_manager->sessions[i]->room_id);
			fflush(stdout);
			if (g_session_manager->sessions[i]->user_id == user_id) {
				printf("[SESSION_MGR] Found session for user_id=%lld\n", (long long)user_id);
				fflush(stdout);
				return g_session_manager->sessions[i];
			}
		}
	}
	
	printf("[SESSION_MGR] Not found session for user_id=%lld\n", (long long)user_id);
	fflush(stdout);
	return NULL;
}

int session_manager_send_to_user(int64_t user_id, uint16_t cmd, const char *json, uint32_t json_len) {
	ClientSession *sess = session_manager_get_by_user_id(user_id);
	if (!sess) return 0;
	
	// Use protocol_send_response to send message
	protocol_send_response(sess, cmd, json, json_len);
	return 1;
}

int session_manager_broadcast_to_room(int64_t room_id, uint16_t cmd, const char *json, uint32_t json_len) {
	if (!g_session_manager || room_id <= 0) {
		printf("[SESSION_MGR] broadcast_to_room: invalid params (room_id=%lld)\n", (long long)room_id);
		fflush(stdout);
		return 0;
	}
	
	int count = 0;
	int total_sessions = 0;
	for (int i = 0; i < g_session_manager->max_sessions; i++) {
		if (g_session_manager->sessions[i]) {
			total_sessions++;
			if (g_session_manager->sessions[i]->room_id == room_id) {
				printf("[SESSION_MGR] Broadcasting to session[%d]: user_id=%d, room_id=%lld\n",
				       i, g_session_manager->sessions[i]->user_id, 
				       (long long)g_session_manager->sessions[i]->room_id);
				fflush(stdout);
				protocol_send_response(g_session_manager->sessions[i], cmd, json, json_len);
				count++;
			}
		}
	}
	printf("[SESSION_MGR] broadcast_to_room(room_id=%lld, cmd=0x%04x): sent to %d/%d sessions\n",
	       (long long)room_id, cmd, count, total_sessions);
	fflush(stdout);
	return count;
}

// Get status string for broadcasting
const char *session_get_status_string(int64_t user_id) {
	ClientSession *sess = session_manager_get_by_user_id(user_id);
	if (!sess) return "offline";
	
	switch (sess->status) {
		case USER_STATUS_ONLINE:
			return "online";
		case USER_STATUS_IN_WAITING_ROOM:
			return "in_waiting_room";
		case USER_STATUS_IN_GAME:
			return "in_game";
		default:
			return "unknown";
	}
}

// Check if user can be invited
bool session_can_invite_user(int64_t target_user_id, int64_t from_room_id) {
	ClientSession *sess = session_manager_get_by_user_id(target_user_id);
	
	// User not online -> cannot invite
	if (!sess) {
		return false;
	}
	
	// User is playing game -> cannot invite
	if (sess->status == USER_STATUS_IN_GAME) {
		return false;
	}
	
	// User is in the same room -> already there, no need to invite
	if (sess->status == USER_STATUS_IN_WAITING_ROOM && sess->room_id == from_room_id) {
		return false;
	}
	
	// ONLINE or in different room -> OK
	return true;
}

// Update user status
void session_manager_update_status(int64_t user_id, int status, int64_t room_id) {
	ClientSession *sess = session_manager_get_by_user_id(user_id);
	if (!sess) return;
	
	sess->status = (UserStatus)status;
	if (room_id > 0) {
		sess->room_id = room_id;
	} else if (status == USER_STATUS_ONLINE) {
		sess->room_id = 0;  // Clear room when back to online
	}
	
	printf("[SESSION_MGR] Updated user %lld status to %d, room_id=%lld\n",
	       (long long)user_id, status, (long long)sess->room_id);
	fflush(stdout);
}

// Broadcast friend status update
void session_broadcast_friend_status(int64_t user_id) {
	if (!g_session_manager) return;
	
	// Get user's current status
	const char *status_str = session_get_status_string(user_id);
	ClientSession *user_sess = session_manager_get_by_user_id(user_id);
	int64_t room_id = user_sess ? user_sess->room_id : 0;
	
	// Build JSON notification
	char json[256];
	snprintf(json, sizeof(json),
		"{\"user_id\": %lld, \"status\": \"%s\", \"room_id\": %lld}",
		(long long)user_id, status_str, (long long)room_id);
	
	printf("[SESSION_MGR] Broadcasting friend status for user %lld: %s\n",
	       (long long)user_id, status_str);
	fflush(stdout);
	
	// Send to all online users (simplified - in production, filter by friends)
	// TODO: Query dao_friends to get actual friends list
	for (int i = 0; i < g_session_manager->max_sessions; i++) {
		ClientSession *sess = g_session_manager->sessions[i];
		if (sess && sess->user_id > 0 && sess->user_id != user_id) {
			session_manager_send_to_user(sess->user_id, 0x0208, json, (uint32_t)strlen(json));
		}
	}
}