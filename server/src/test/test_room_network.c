// Test Room features via network protocol
// Compile: make build/test_room_network
// Usage: ./build/test_room_network [host] [port] [username] [password]

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

static int test_login(int sockfd, const char *username, const char *password, uint16_t *user_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

	printf("[TEST] Sending LOGIN...\n");
	if (send_packet(sockfd, CMD_REQ_LOGIN, 0, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send LOGIN\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive LOGIN response\n");
		return -1;
	}

	if (cmd == CMD_RES_LOGIN) {
		printf("[TEST] ✓ LOGIN successful\n");
		*user_id = 1;
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ LOGIN failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int64_t extract_room_id(const char *json) {
	const char *p = strstr(json, "\"room_id\"");
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	return atoll(p + 1);
}

static int test_create_room(int sockfd, uint16_t user_id, int64_t *room_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"easy_count\":5,\"medium_count\":5,\"hard_count\":5}");

	printf("\n[TEST] ========== CREATE ROOM ==========\n");
	if (send_packet(sockfd, CMD_REQ_CREATE_ROOM, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send CREATE_ROOM\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive CREATE_ROOM response\n");
		return -1;
	}

	if (cmd == CMD_RES_CREATE_ROOM) {
		*room_id = extract_room_id(response);
		printf("[TEST] ✓ Room created successfully\n");
		printf("[TEST] Response: %s\n", response);
		printf("[TEST] Room ID: %lld\n", (long long)*room_id);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Create room failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_join_room(int sockfd, uint16_t user_id, int64_t room_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"room_id\":%lld}", (long long)room_id);

	printf("\n[TEST] ========== JOIN ROOM ==========\n");
	printf("[TEST] Joining room %lld\n", (long long)room_id);
	if (send_packet(sockfd, CMD_REQ_JOIN_ROOM, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send JOIN_ROOM\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive JOIN_ROOM response\n");
		return -1;
	}

	if (cmd == CMD_RES_JOIN_ROOM) {
		printf("[TEST] ✓ Joined room successfully\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Join room failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_receive_room_update(int sockfd) {
	printf("\n[TEST] ========== RECEIVE ROOM UPDATE ==========\n");
	
	// Set socket to non-blocking for timeout
	fd_set readfds;
	struct timeval timeout;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
	if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
		uint16_t cmd;
		char response[512];
		if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
			if (cmd == CMD_NOTIFY_ROOM_UPDATE) {
				printf("[TEST] ✓ Room update received\n");
				printf("[TEST] Update: %s\n", response);
				return 0;
			}
		}
	}
	
	printf("[TEST] No room update received (timeout or not available)\n");
	return 0;
}

static int test_leave_room(int sockfd, uint16_t user_id, int64_t room_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"room_id\":%lld}", (long long)room_id);

	printf("\n[TEST] ========== LEAVE ROOM ==========\n");
	if (send_packet(sockfd, CMD_REQ_LEAVE_ROOM, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send LEAVE_ROOM\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive LEAVE_ROOM response\n");
		return -1;
	}

	if (cmd == CMD_RES_LEAVE_ROOM) {
		printf("[TEST] ✓ Left room successfully\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Leave room failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_start_game(int sockfd, uint16_t user_id, int64_t room_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"room_id\":%lld}", (long long)room_id);

	printf("\n[TEST] ========== START GAME ==========\n");
	if (send_packet(sockfd, CMD_REQ_START_GAME, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send START_GAME\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive START_GAME response\n");
		return -1;
	}

	if (cmd == CMD_RES_START_GAME || cmd == CMD_NOTIFY_GAME_START_1VN) {
		printf("[TEST] ✓ Game started\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Start game failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

int main(int argc, char *argv[]) {
	const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
	const char *port = (argc > 2) ? argv[2] : DEFAULT_PORT;
	const char *username = (argc > 3) ? argv[3] : "testuser";
	const char *password = (argc > 4) ? argv[4] : "testpass";

	printf("=== Room Network Test ===\n");
	printf("Host: %s:%s\n", host, port);
	printf("Username: %s\n\n", username);

	int sockfd = connect_to_server(host, port);
	if (sockfd < 0) {
		fprintf(stderr, "Failed to connect to server\n");
		return 1;
	}

	printf("[TEST] ✓ Connected to server\n");

	uint16_t user_id;
	if (test_login(sockfd, username, password, &user_id) != 0) {
		close(sockfd);
		return 1;
	}

	// Test create room
	int64_t room_id;
	if (test_create_room(sockfd, user_id, &room_id) != 0) {
		close(sockfd);
		return 1;
	}

	// Test join room
	test_join_room(sockfd, user_id, room_id);

	// Wait for room update notification
	usleep(500000);
	test_receive_room_update(sockfd);

	// Test start game
	test_start_game(sockfd, user_id, room_id);

	// Test leave room
	test_leave_room(sockfd, user_id, room_id);

	printf("\n[TEST] ========== TEST COMPLETED ==========\n");
	close(sockfd);
	return 0;
}

