// Test Chat features via network protocol
// Compile: make build/test_chat_network
// Usage: ./build/test_chat_network [host] [port] [username] [password]

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
	// Simple extraction: look for "user_id": number
	const char *p = strstr(json, "\"user_id\"");
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	// Skip spaces
	while (*p == ' ' || *p == ':') p++;
	return atoll(p);
}

static int64_t extract_room_id(const char *json) {
	// Simple extraction: look for "room_id": number
	const char *p = strstr(json, "\"room_id\"");
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	// Skip spaces
	while (*p == ' ' || *p == ':') p++;
	return atoll(p);
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
		printf("[TEST] Response: %s\n", response);
		// Check if response contains error
		if (strstr(response, "\"error\"") != NULL) {
			fprintf(stderr, "[TEST] ✗ LOGIN failed: %s\n", response);
			return -1;
		}
		printf("[TEST] ✓ LOGIN successful\n");
		// Extract user_id from response
		int64_t user_id_64 = extract_user_id(response);
		if (user_id_64 > 0 && user_id_64 <= UINT16_MAX) {
			*user_id = (uint16_t)user_id_64;
			printf("[TEST] Extracted user_id: %d\n", *user_id);
		} else {
			fprintf(stderr, "[TEST] ✗ Could not extract user_id from response\n");
			return -1;
		}
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ LOGIN failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_send_dm(int sockfd, uint16_t user_id, int64_t to_user_id, const char *message) {
	char json[512];
	snprintf(json, sizeof(json), "{\"to_user_id\":%lld,\"message\":\"%s\"}", 
	         (long long)to_user_id, message);

	printf("\n[TEST] ========== SEND DM ==========\n");
	printf("[TEST] Sending DM to user %lld: %s\n", (long long)to_user_id, message);
	if (send_packet(sockfd, CMD_REQ_SEND_DM, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send SEND_DM\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive SEND_DM response\n");
		return -1;
	}

	if (cmd == CMD_RES_SEND_DM) {
		printf("[TEST] ✓ DM sent successfully\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Send DM failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_receive_dm(int sockfd) {
	printf("\n[TEST] ========== RECEIVE DM ==========\n");
	
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
			if (cmd == CMD_NOTIFY_DM) {
				printf("[TEST] ✓ DM received\n");
				printf("[TEST] Message: %s\n", response);
				return 0;
			}
		}
	}
	
	printf("[TEST] No DM received (timeout or not available)\n");
	return 0;
}

static int test_send_room_chat(int sockfd, uint16_t user_id, int64_t room_id, const char *message) {
	char json[512];
	snprintf(json, sizeof(json), "{\"room_id\":%lld,\"message\":\"%s\"}", 
	         (long long)room_id, message);

	printf("\n[TEST] ========== SEND ROOM CHAT ==========\n");
	printf("[TEST] Sending room chat to room %lld: %s\n", (long long)room_id, message);
	if (send_packet(sockfd, CMD_REQ_SEND_ROOM_CHAT, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send SEND_ROOM_CHAT\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive SEND_ROOM_CHAT response\n");
		return -1;
	}

	if (cmd == CMD_RES_SEND_ROOM_CHAT) {
		printf("[TEST] ✓ Room chat sent successfully\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Send room chat failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_receive_room_chat(int sockfd) {
	printf("\n[TEST] ========== RECEIVE ROOM CHAT ==========\n");
	
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
			if (cmd == CMD_NOTIFY_ROOM_CHAT) {
				printf("[TEST] ✓ Room chat received\n");
				printf("[TEST] Message: %s\n", response);
				return 0;
			}
		}
	}
	
	printf("[TEST] No room chat received (timeout or not available)\n");
	return 0;
}

static int test_create_room(int sockfd, uint16_t user_id, int64_t *room_id) {
	printf("\n[TEST] ========== CREATE ROOM ==========\n");
	if (send_packet(sockfd, CMD_REQ_CREATE_ROOM, user_id, "{}") != 0) {
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
		int64_t room_id_extracted = extract_room_id(response);
		if (room_id_extracted > 0) {
			*room_id = room_id_extracted;
			printf("[TEST] ✓ Room created successfully\n");
			printf("[TEST] Response: %s\n", response);
			printf("[TEST] Room ID: %lld\n", (long long)*room_id);
			return 0;
		} else {
			fprintf(stderr, "[TEST] ✗ Could not extract room_id from response\n");
			return -1;
		}
	} else {
		fprintf(stderr, "[TEST] ✗ Create room failed: cmd=0x%04x, response: %s\n", cmd, response);
		return -1;
	}
}

static int test_fetch_offline(int sockfd, uint16_t user_id) {
	printf("\n[TEST] ========== FETCH OFFLINE MESSAGES ==========\n");
	if (send_packet(sockfd, CMD_REQ_FETCH_OFFLINE, user_id, "{}") != 0) {
		fprintf(stderr, "[TEST] Failed to send FETCH_OFFLINE\n");
		return -1;
	}

	uint16_t cmd;
	char response[2048];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive FETCH_OFFLINE response\n");
		return -1;
	}

	if (cmd == CMD_RES_FETCH_OFFLINE) {
		printf("[TEST] ✓ Offline messages fetched\n");
		printf("[TEST] Messages: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Fetch offline failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

int main(int argc, char *argv[]) {
	const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
	const char *port = (argc > 2) ? argv[2] : DEFAULT_PORT;
	const char *username = (argc > 3) ? argv[3] : "testuser1";
	const char *password = (argc > 4) ? argv[4] : "testpass1";

	printf("=== Chat Network Test ===\n");
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

	// Test send DM
	test_send_dm(sockfd, user_id, 2, "Hello from test!");

	// Test receive DM (if any)
	usleep(500000);
	test_receive_dm(sockfd);

	// Create a room first before sending room chat
	int64_t room_id = 0;
	if (test_create_room(sockfd, user_id, &room_id) == 0) {
		// Test send room chat to the created room
		test_send_room_chat(sockfd, user_id, room_id, "Hello room!");

		// Test receive room chat
		usleep(500000);
		test_receive_room_chat(sockfd);
	} else {
		fprintf(stderr, "[TEST] Warning: Could not create room, skipping room chat test\n");
	}

	// Test fetch offline messages
	test_fetch_offline(sockfd, user_id);

	printf("\n[TEST] ========== TEST COMPLETED ==========\n");
	close(sockfd);
	return 0;
}

