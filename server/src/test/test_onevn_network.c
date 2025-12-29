// Test 1vN game features via network protocol
// Compile: make build/test_onevn_network
// Usage: ./build/test_onevn_network [host] [port] [username] [password]

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

static int test_create_room(int sockfd, uint16_t user_id, int64_t *room_id) {
	printf("\n[TEST] ========== CREATE ROOM ==========\n");
	// Create room with default config (5 easy, 5 medium, 5 hard)
	char json[256];
	snprintf(json, sizeof(json), "{}");
	
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

static int test_start_game(int sockfd, uint16_t user_id, int64_t room_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"room_id\":%lld}", (long long)room_id);

	printf("\n[TEST] ========== START 1VN GAME ==========\n");
	if (send_packet(sockfd, CMD_REQ_START_GAME, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send START_GAME\n");
		return -1;
	}

	// Receive start game response (may be CMD_RES_START_GAME or CMD_NOTIFY_GAME_START_1VN)
	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive START_GAME response\n");
		return -1;
	}

	if (cmd == CMD_NOTIFY_GAME_START_1VN || cmd == CMD_RES_START_GAME) {
		printf("[TEST] ✓ Game started\n");
		printf("[TEST] Response: %s\n", response);
		
		// Continue reading packets for a short time to catch CMD_NOTIFY_QUESTION_1VN if it comes immediately
		// This handles the case where server sends multiple packets quickly
		fd_set readfds;
		struct timeval timeout;
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000; // 500ms timeout
		
		int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
		if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
			// There's more data, but don't read it here - let test_receive_question handle it
			// Just return success so test can continue
		}
		
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Start game failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_receive_question(int sockfd) {
	printf("\n[TEST] ========== RECEIVE QUESTION ==========\n");
	
	// Try to read packet immediately (may already be in buffer)
	uint16_t cmd;
	char response[2048];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
		if (cmd == CMD_NOTIFY_QUESTION_1VN) {
			printf("[TEST] ✓ Question received\n");
			printf("[TEST] Question: %s\n", response);
			return 0;
		} else {
			printf("[TEST] Received cmd=0x%04x (not question), continuing to wait...\n", cmd);
		}
	}
	
	// If no packet in buffer, wait for it
	fd_set readfds;
	struct timeval timeout;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
	if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
		if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
			if (cmd == CMD_NOTIFY_QUESTION_1VN) {
				printf("[TEST] ✓ Question received\n");
				printf("[TEST] Question: %s\n", response);
				return 0;
			} else {
				printf("[TEST] Received unexpected cmd=0x%04x: %s\n", cmd, response);
			}
		}
	}
	
	printf("[TEST] No question received (timeout or not available)\n");
	return -1;
}

static int test_submit_answer(int sockfd, uint16_t user_id, int round, char answer) {
	char json[512];
	snprintf(json, sizeof(json), "{\"round\":%d,\"answer\":\"%c\",\"time_ms\":1500}", round, answer);

	printf("\n[TEST] ========== SUBMIT ANSWER ==========\n");
	printf("[TEST] Submitting answer '%c' for round %d\n", answer, round);
	if (send_packet(sockfd, CMD_REQ_SUBMIT_ANSWER_1VN, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send SUBMIT_ANSWER_1VN\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive SUBMIT_ANSWER_1VN response\n");
		return -1;
	}

	if (cmd == CMD_RES_SUBMIT_ANSWER_1VN) {
		printf("[TEST] ✓ Answer submitted\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Submit answer failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_receive_elimination(int sockfd) {
	printf("\n[TEST] ========== RECEIVE ELIMINATION ==========\n");
	
	fd_set readfds;
	struct timeval timeout;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
	if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
		uint16_t cmd;
		char response[512];
		if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
			if (cmd == CMD_NOTIFY_ELIMINATION) {
				printf("[TEST] ✓ Elimination notification received\n");
				printf("[TEST] Elimination: %s\n", response);
				return 0;
			}
		}
	}
	
	printf("[TEST] No elimination received (timeout or not available)\n");
	return 0;
}

static int test_receive_game_over(int sockfd) {
	printf("\n[TEST] ========== RECEIVE GAME OVER ==========\n");
	
	fd_set readfds;
	struct timeval timeout;
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
	if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
		uint16_t cmd;
		char response[2048];
		if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
			if (cmd == CMD_NOTIFY_GAME_OVER_1VN) {
				printf("[TEST] ✓ Game Over notification received\n");
				printf("[TEST] Results: %s\n", response);
				return 0;
			}
		}
	}
	
	printf("[TEST] No game over notification received (timeout or not available)\n");
	return 0;
}

int main(int argc, char *argv[]) {
	const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
	const char *port = (argc > 2) ? argv[2] : DEFAULT_PORT;
	const char *username = (argc > 3) ? argv[3] : "testuser1";
	const char *password = (argc > 4) ? argv[4] : "testpass1";

	printf("=== 1vN Network Test ===\n");
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

	// Create a room first (user becomes owner)
	int64_t room_id = 0;
	if (test_create_room(sockfd, user_id, &room_id) != 0) {
		fprintf(stderr, "[TEST] Failed to create room, cannot continue\n");
		close(sockfd);
		return 1;
	}

	// Note: For 1vN game, we need at least 2 players.
	// Print room_id so client 2 can join (for 2-client testing)
	printf("\n[TEST] ========== WAITING FOR PLAYER 2 ==========\n");
	printf("[TEST] Room ID: %lld (client 2 should join this room)\n", (long long)room_id);
	printf("[TEST] Waiting 8 seconds for client 2 to join...\n");
	fflush(stdout);
	usleep(8000000);  // Wait 8 seconds for client 2 to join and process

	// Test start game
	if (test_start_game(sockfd, user_id, room_id) != 0) {
		close(sockfd);
		return 1;
	}

	// Test receive question
	if (test_receive_question(sockfd) != 0) {
		close(sockfd);
		return 1;
	}

	// Test submit answer
	test_submit_answer(sockfd, user_id, 1, 'A');

	// Wait for elimination notification
	usleep(1000000);
	test_receive_elimination(sockfd);

	// Wait for game over
	test_receive_game_over(sockfd);

	printf("\n[TEST] ========== TEST COMPLETED ==========\n");
	close(sockfd);
	return 0;
}

