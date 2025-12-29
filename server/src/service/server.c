// Multi-client TCP server using epoll
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "service/server.h"
#include "service/client_session.h"
#include "service/session_manager.h"
#include "service/dispatcher.h"
#include "service/protocol.h"
#include "service/quickmode_service.h"
#include "service/friends_service.h"
#include "utils/timer.h"

static volatile int running = 1;

static void handle_sigint(int signum) {
	(void)signum;
	running = 0;
}

static int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int start_server(const char *bind_addr, const char *portstr) {
	struct addrinfo hints, *res, *rp;
	int sockfd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int s = getaddrinfo(bind_addr, portstr, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) continue;

		int opt = 1;
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
		close(sockfd);
		sockfd = -1;
	}
	freeaddrinfo(res);

	if (sockfd < 0) {
		fprintf(stderr, "Failed to bind socket\n");
		return -1;
	}

	// Set non-blocking
	if (set_nonblocking(sockfd) < 0) {
		fprintf(stderr, "Failed to set non-blocking\n");
		close(sockfd);
		return -1;
	}

	if (listen(sockfd, 16) != 0) {
		fprintf(stderr, "listen failed: %s\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	signal(SIGINT, handle_sigint);

	// Create session manager
	SessionManager *mgr = session_manager_new(MAX_SESSIONS);
	if (!mgr) {
		fprintf(stderr, "Failed to create session manager\n");
		close(sockfd);
		return -1;
	}

	// Set global session manager (important for session_manager_get_by_user_id)
	session_manager_set_global(mgr);

	// Add server socket to epoll
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sockfd;
	if (epoll_ctl(session_manager_get_epoll_fd(mgr), EPOLL_CTL_ADD, sockfd, &ev) < 0) {
		fprintf(stderr, "epoll_ctl failed: %s\n", strerror(errno));
		session_manager_free(mgr);
		close(sockfd);
		return -1;
	}

	printf("Server listening on %s:%s (epoll-based, multi-client)\n", 
	       bind_addr ? bind_addr : "0.0.0.0", portstr);

	struct epoll_event events[MAX_EPOLL_EVENTS];

	while (running) {
		// Check and run expired game timers (for 1vN mode timeout handling)
		game_timer_check_and_run();
		
		int nfds = epoll_wait(session_manager_get_epoll_fd(mgr), events, MAX_EPOLL_EVENTS, 100);
		
		if (nfds < 0) {
			if (errno == EINTR) continue;
			fprintf(stderr, "epoll_wait failed: %s\n", strerror(errno));
			break;
		}

		for (int i = 0; i < nfds; i++) {
			int fd = events[i].data.fd;

			// New connection
			if (fd == sockfd) {
				// Accept all pending connections (edge-triggered)
				while (1) {
					struct sockaddr_storage cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int client_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
					
					if (client_fd < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							// No more connections
							break;
						}
						fprintf(stderr, "accept failed: %s\n", strerror(errno));
						break;
					}

					// Set non-blocking
					if (set_nonblocking(client_fd) < 0) {
						close(client_fd);
						continue;
					}

					// Create session
					ClientSession *sess = client_session_new(client_fd);
					if (!sess) {
						close(client_fd);
						continue;
					}

					// Add to session manager
					if (session_manager_add(mgr, sess) < 0) {
						fprintf(stderr, "Failed to add session\n");
						close(client_fd);
						client_session_free(sess);
						continue;
					}

					printf("New client connected (fd=%d, total=%d)\n", 
					       client_fd, session_manager_count(mgr));
				}
			} else {
				// Data from client - get session from epoll_event.data.ptr
				ClientSession *sess = (ClientSession *)events[i].data.ptr;
				if (!sess) continue;

				// Read packet (handles partial reads)
				uint16_t cmd;
				char *payload = NULL;
				uint32_t payload_len = 0;

				int result = client_session_read_packet(sess, &cmd, &payload, &payload_len);

				if (result < 0) {
					// Error or disconnect
					printf("Client disconnected (fd=%d)\n", fd);
					// Notify friends that user is offline (before removing session)
					if (sess && sess->user_id > 0) {
						// Import friends_service to notify friends
						extern void friends_notify_status_change(int64_t user_id, const char *status, int64_t room_id);
						friends_notify_status_change(sess->user_id, "offline", 0);
						// Cleanup quickmode session if exists
						quickmode_cleanup_user(sess->user_id);
					}
					session_manager_remove(mgr, fd);
					continue;
				}

				if (result == 0) {
					// Partial packet, wait for more data
					continue;
				}

				// Complete packet received
				dispatcher_handle_packet(sess, cmd, payload, payload_len);

				if (payload) free(payload);
			}
		}
	}

	printf("Shutting down server...\n");
	session_manager_free(mgr);
	close(sockfd);
	return 0;
}

