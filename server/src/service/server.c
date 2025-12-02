// Minimal TCP server runtime: accept connections and dispatch packets.
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

#include "service/server.h"
#include "service/client_session.h"
#include "service/dispatcher.h"
#include "service/protocol.h"

static volatile int running = 1;

static void handle_sigint(int signum) {
	(void)signum;
	running = 0;
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

	if (listen(sockfd, 16) != 0) {
		fprintf(stderr, "listen failed: %s\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	signal(SIGINT, handle_sigint);

	printf("Server listening on %s:%s\n", bind_addr ? bind_addr : "0.0.0.0", portstr);

	while (running) {
		struct sockaddr_storage cli_addr;
		socklen_t cli_len = sizeof(cli_addr);
		int fd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
		if (fd < 0) {
			if (errno == EINTR) break;
			fprintf(stderr, "accept: %s\n", strerror(errno));
			continue;
		}

		ClientSession *sess = client_session_new(fd);
		if (!sess) {
			close(fd);
			continue;
		}

		// read header
		PacketHeader hdr;
		ssize_t n = recv(fd, &hdr, sizeof(hdr), MSG_WAITALL);
		if (n != (ssize_t)sizeof(hdr)) {
			close(fd);
			client_session_free(sess);
			continue;
		}

		// convert from network byte order
		uint16_t cmd = ntohs(hdr.cmd);
		uint32_t len = ntohl(hdr.length);

		char *payload = NULL;
		if (len > 0) {
			payload = malloc(len + 1);
			if (!payload) { close(fd); client_session_free(sess); continue; }
			ssize_t r = recv(fd, payload, len, MSG_WAITALL);
			if (r != (ssize_t)len) { free(payload); close(fd); client_session_free(sess); continue; }
			payload[len] = '\0';
		}

		dispatcher_handle_packet(sess, cmd, payload, len);

		if (payload) free(payload);

		close(fd);
		client_session_free(sess);
	}

	close(sockfd);
	return 0;
}

