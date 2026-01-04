// Helper test: Login and join a room (for 2-client testing)
// Compile: make build/test_onevn_join
// Usage: ./build/test_onevn_join [host] [port] [username] [password] [room_id]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdint.h>
#include <sys/select.h>
#include "service/commands.h"

typedef struct {
	uint16_t cmd;
	uint16_t user_id;
	uint32_t length;
} PacketHeader;

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "9000"

static int connect_to_server(const char *host, const char *port) {
	struct addrinfo hints, *res, *rp;
	int sockfd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int s = getaddrinfo(host, port, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1) continue;
		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
		close(sockfd);
		sockfd = -1;
	}

	freeaddrinfo(res);
	return sockfd;
}

static int send_packet(int sockfd, uint16_t cmd, uint16_t user_id, const char *json) {
	PacketHeader hdr;
	uint32_t json_len = json ? (uint32_t)strlen(json) : 0;

	hdr.cmd = htons(cmd);
	hdr.user_id = htons(user_id);
	hdr.length = htonl(json_len);

	if (send(sockfd, &hdr, sizeof(hdr), 0) != sizeof(hdr)) return -1;
	if (json_len > 0 && send(sockfd, json, json_len, 0) != (ssize_t)json_len) return -1;
	return 0;
}

static int recv_packet(int sockfd, uint16_t *cmd, char *payload, size_t payload_size) {
	PacketHeader hdr;
	ssize_t n = recv(sockfd, &hdr, sizeof(hdr), MSG_WAITALL);
	if (n != sizeof(hdr)) return -1;

	*cmd = ntohs(hdr.cmd);
	uint32_t len = ntohl(hdr.length);

	if (len > 0) {
		if (len >= payload_size) len = payload_size - 1;
		n = recv(sockfd, payload, len, MSG_WAITALL);
		if (n != (ssize_t)len) return -1;
		payload[len] = '\0';
	} else {
		payload[0] = '\0';
	}
	return 0;
}

static int64_t extract_user_id(const char *json) {
	const char *p = strstr(json, "\"user_id\"");
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	while (*p == ' ' || *p == ':') p++;
	return atoll(p);
}

static int64_t extract_room_id(const char *json) {
	const char *p = strstr(json, "\"room_id\"");
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	while (*p == ' ' || *p == ':') p++;
	return atoll(p);
}

static int test_login(int sockfd, const char *username, const char *password, uint16_t *user_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

	printf("[CLIENT 2] Sending LOGIN...\n");
	if (send_packet(sockfd, CMD_REQ_LOGIN, 0, json) != 0) {
		fprintf(stderr, "[CLIENT 2] Failed to send LOGIN\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[CLIENT 2] Failed to receive LOGIN response\n");
		return -1;
	}

	if (cmd == CMD_RES_LOGIN) {
		if (strstr(response, "\"error\"") != NULL) {
			fprintf(stderr, "[CLIENT 2] ✗ LOGIN failed: %s\n", response);
			return -1;
		}
		printf("[CLIENT 2] ✓ LOGIN successful\n");
		int64_t user_id_64 = extract_user_id(response);
		if (user_id_64 > 0 && user_id_64 <= UINT16_MAX) {
			*user_id = (uint16_t)user_id_64;
		} else {
			fprintf(stderr, "[CLIENT 2] Could not extract user_id\n");
			return -1;
		}
		return 0;
	} else {
		fprintf(stderr, "[CLIENT 2] ✗ LOGIN failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_join_room(int sockfd, uint16_t user_id, int64_t room_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"room_id\":%lld}", (long long)room_id);

	printf("[CLIENT 2] Joining room %lld...\n", (long long)room_id);
	if (send_packet(sockfd, CMD_REQ_JOIN_ROOM, user_id, json) != 0) {
		fprintf(stderr, "[CLIENT 2] Failed to send JOIN_ROOM\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[CLIENT 2] Failed to receive JOIN_ROOM response\n");
		return -1;
	}

	if (cmd == CMD_RES_JOIN_ROOM) {
		if (strstr(response, "\"error\"") == NULL) {
			printf("[CLIENT 2] ✓ Joined room successfully\n");
			return 0;
		} else {
			fprintf(stderr, "[CLIENT 2] ✗ Join room failed: %s\n", response);
			return -1;
		}
	} else {
		fprintf(stderr, "[CLIENT 2] ✗ Join room failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

int main(int argc, char *argv[]) {
	const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
	const char *port = (argc > 2) ? argv[2] : DEFAULT_PORT;
	const char *username = (argc > 3) ? argv[3] : "testuser2";
	const char *password = (argc > 4) ? argv[4] : "testpass2";
	int64_t room_id = (argc > 5) ? atoll(argv[5]) : 0;

	printf("=== Client 2: Join Room Helper ===\n");
	printf("Host: %s:%s\n", host, port);
	printf("Username: %s\n", username);
	if (room_id > 0) {
		printf("Room ID: %lld\n", (long long)room_id);
	} else {
		printf("Room ID: (will wait for room_id from stdin)\n");
	}

	int sockfd = connect_to_server(host, port);
	if (sockfd < 0) {
		fprintf(stderr, "[CLIENT 2] Failed to connect to server\n");
		return 1;
	}

	printf("[CLIENT 2] ✓ Connected to server\n");

	uint16_t user_id;
	if (test_login(sockfd, username, password, &user_id) != 0) {
		close(sockfd);
		return 1;
	}

	// If room_id not provided, read from stdin (for coordination with client 1)
	if (room_id == 0) {
		char line[256];
		if (fgets(line, sizeof(line), stdin) != NULL) {
			room_id = atoll(line);
		}
	}

	if (room_id <= 0) {
		fprintf(stderr, "[CLIENT 2] Invalid room_id\n");
		close(sockfd);
		return 1;
	}

	if (test_join_room(sockfd, user_id, room_id) != 0) {
		close(sockfd);
		return 1;
	}

	// Wait a bit to ensure join is fully processed
	printf("[CLIENT 2] Waiting 1 second for join to be fully processed...\n");
	usleep(1000000);  // 1 second

	printf("[CLIENT 2] Waiting for game notifications...\n");
	
	// Wait for game notifications (game start, questions, etc.)
	fd_set readfds;
	struct timeval timeout;
	for (int i = 0; i < 30; i++) {  // Wait up to 30 seconds
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
		if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
			uint16_t cmd;
			char response[2048];
			if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
				printf("[CLIENT 2] Received cmd=0x%04x: %s\n", cmd, response);
				if (cmd == CMD_NOTIFY_GAME_OVER_1VN) {
					printf("[CLIENT 2] ✓ Game Over received\n");
					break;
				}
			}
		}
	}

	printf("[CLIENT 2] Test completed\n");
	close(sockfd);
	return 0;
}

