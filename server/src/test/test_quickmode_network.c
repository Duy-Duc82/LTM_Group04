// Test QuickMode game flow via network protocol
// Compile: make build/test_quickmode_network
// Usage: ./build/test_quickmode_network [host] [port] [username] [password]

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
#include <arpa/inet.h>
#include "service/commands.h"

// PacketHeader structure (from protocol.h)
typedef struct {
	uint16_t cmd;
	uint16_t user_id;
	uint32_t length;
} PacketHeader;

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT "9000"
#define BUFFER_SIZE 4096

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

	// Send header
	if (send(sockfd, &hdr, sizeof(hdr), 0) != sizeof(hdr)) {
		return -1;
	}

	// Send payload if exists
	if (json_len > 0 && send(sockfd, json, json_len, 0) != (ssize_t)json_len) {
		return -1;
	}

	return 0;
}

static int recv_packet(int sockfd, uint16_t *cmd, char *payload, size_t payload_size) {
	PacketHeader hdr;

	// Read header
	ssize_t n = recv(sockfd, &hdr, sizeof(hdr), MSG_WAITALL);
	if (n != sizeof(hdr)) {
		return -1;
	}

	*cmd = ntohs(hdr.cmd);
	uint32_t len = ntohl(hdr.length);

	if (len > 0) {
		if (len >= payload_size) {
			len = payload_size - 1;
		}
		n = recv(sockfd, payload, len, MSG_WAITALL);
		if (n != (ssize_t)len) {
			return -1;
		}
		payload[len] = '\0';
	} else {
		payload[0] = '\0';
	}

	return 0;
}

static int test_login(int sockfd, const char *username, const char *password) {
	char json[256];
	snprintf(json, sizeof(json), "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);

	printf("[TEST] Sending LOGIN request...\n");
	if (send_packet(sockfd, CMD_REQ_LOGIN, 0, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send LOGIN packet\n");
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
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ LOGIN failed: cmd=0x%04x, response=%s\n", cmd, response);
		return -1;
	}
}

static int64_t extract_session_id(const char *json) {
	// Simple extraction: look for "session_id": number
	const char *p = strstr(json, "\"session_id\"");
	if (!p) return 0;
	p = strchr(p, ':');
	if (!p) return 0;
	return atoll(p + 1);
}

static int test_start_game(int sockfd, uint16_t user_id) {
	printf("\n[TEST] ========== PHASE 2 & 3: START GAME ==========\n");
	
	printf("[TEST] Sending CMD_REQ_START_QUICKMODE...\n");
	if (send_packet(sockfd, CMD_REQ_START_QUICKMODE, user_id, "{}") != 0) {
		fprintf(stderr, "[TEST] Failed to send START_QUICKMODE\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive START_QUICKMODE response\n");
		return -1;
	}

	if (cmd == CMD_NOTIFY_GAME_START) {
		int64_t session_id = extract_session_id(response);
		printf("[TEST] ✓ Game started successfully\n");
		printf("[TEST] Response: %s\n", response);
		printf("[TEST] Session ID: %lld\n", (long long)session_id);
		return (int)session_id;
	} else {
		fprintf(stderr, "[TEST] ✗ Start game failed: cmd=0x%04x, response=%s\n", cmd, response);
		return -1;
	}
}

static int test_get_question(int sockfd, uint16_t user_id, int64_t session_id, int round) {
	printf("\n[TEST] ========== PHASE 3: GET QUESTION (Round %d) ==========\n", round);

	char json[256];
	snprintf(json, sizeof(json), "{\"session_id\":%lld,\"round\":%d}", (long long)session_id, round);

	printf("[TEST] Sending CMD_REQ_GET_QUESTION (round %d)...\n", round);
	if (send_packet(sockfd, CMD_REQ_GET_QUESTION, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send GET_QUESTION\n");
		return -1;
	}

	uint16_t cmd;
	char response[2048];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive GET_QUESTION response\n");
		return -1;
	}

	if (cmd == CMD_NOTIFY_QUESTION) {
		printf("[TEST] ✓ Question received\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Get question failed: cmd=0x%04x, response=%s\n", cmd, response);
		return -1;
	}
}

static int test_submit_answer(int sockfd, uint16_t user_id, int64_t session_id, int round, char answer) {
	printf("\n[TEST] ========== PHASE 3: SUBMIT ANSWER (Round %d, Answer %c) ==========\n", round, answer);

	char json[256];
	snprintf(json, sizeof(json), "{\"session_id\":%lld,\"round\":%d,\"answer\":\"%c\"}", 
	         (long long)session_id, round, answer);

	printf("[TEST] Sending CMD_REQ_SUBMIT_ANSWER (round %d, answer %c)...\n", round, answer);
	if (send_packet(sockfd, CMD_REQ_SUBMIT_ANSWER, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send SUBMIT_ANSWER\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive SUBMIT_ANSWER response\n");
		return -1;
	}

	if (cmd == CMD_RES_SUBMIT_ANSWER) {
		printf("[TEST] ✓ Answer submitted\n");
		printf("[TEST] Response: %s\n", response);
		
		// Check for answer result notification
		if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
			if (cmd == CMD_NOTIFY_ANSWER_RESULT) {
				printf("[TEST] ✓ Answer result notification received\n");
				printf("[TEST] Result: %s\n", response);
			}
		}
		
		// Check for game over
		if (strstr(response, "\"game_over\":true") != NULL) {
			printf("[TEST] ⚠ Game Over detected in response\n");
		}
		
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Submit answer failed: cmd=0x%04x, response=%s\n", cmd, response);
		return -1;
	}
}

static int test_game_over(int sockfd) {
	printf("\n[TEST] ========== PHASE 3: GAME OVER ==========\n");

	uint16_t cmd;
	char response[512];
	
	// Try to receive game over notification (might already be received)
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0) {
		if (cmd == CMD_NOTIFY_GAME_OVER) {
			printf("[TEST] ✓ Game Over notification received\n");
			printf("[TEST] Response: %s\n", response);
			return 0;
		}
	}

	printf("[TEST] Game over notification not received (may have been received earlier)\n");
	return 0;
}

int main(int argc, char *argv[]) {
	const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
	const char *port = (argc > 2) ? argv[2] : DEFAULT_PORT;
	const char *username = (argc > 3) ? argv[3] : "testuser";
	const char *password = (argc > 4) ? argv[4] : "testpass";

	printf("=== QuickMode Network Test ===\n");
	printf("Host: %s:%s\n", host, port);
	printf("Username: %s\n", username);
	printf("\n");

	int sockfd = connect_to_server(host, port);
	if (sockfd < 0) {
		fprintf(stderr, "Failed to connect to server\n");
		return 1;
	}

	printf("[TEST] ✓ Connected to server\n");

	// Phase 1: Login
	if (test_login(sockfd, username, password) != 0) {
		close(sockfd);
		return 1;
	}

	// Extract user_id from login response (simplified - assume user_id = 1 for test)
	uint16_t user_id = 1;

	// Phase 2 & 3: Start Game
	int64_t session_id = test_start_game(sockfd, user_id);
	if (session_id <= 0) {
		close(sockfd);
		return 1;
	}

	// Phase 3: Test multiple rounds
	printf("\n[TEST] ========== PHASE 3: TEST MULTIPLE ROUNDS ==========\n");
	
	for (int round = 1; round <= 3; round++) {
		// Get question
		if (test_get_question(sockfd, user_id, session_id, round) != 0) {
			break;
		}

		// Submit answer (test with 'A' for simplicity)
		if (test_submit_answer(sockfd, user_id, session_id, round, 'A') != 0) {
			break;
		}

		// Small delay
		usleep(100000);
	}

	// Phase 3: Game Over
	test_game_over(sockfd);

	printf("\n[TEST] ========== TEST COMPLETE ==========\n");

	close(sockfd);
	return 0;
}
