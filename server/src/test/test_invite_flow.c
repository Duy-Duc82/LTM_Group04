// Test invite friend to room flow
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdbool.h>
#include "service/commands.h"

#define BUFFER_SIZE 8192
#define SERVER_PORT 9000

// Packet header structure (8 bytes)
typedef struct {
	uint16_t cmd;
	uint16_t user_id;
	uint32_t payload_len;
} __attribute__((packed)) PacketHeader;

// Connect to server
static int connect_to_server(const char *host, int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
		perror("inet_pton");
		close(sockfd);
		return -1;
	}

	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(sockfd);
		return -1;
	}

	printf("[CLIENT] Connected to %s:%d (fd=%d)\n", host, port, sockfd);
	return sockfd;
}

// Send packet
static int send_packet(int sockfd, uint16_t cmd, uint16_t user_id, const char *json) {
	PacketHeader hdr;
	hdr.cmd = htons(cmd);
	hdr.user_id = htons(user_id);
	uint32_t len = json ? (uint32_t)strlen(json) : 0;
	hdr.payload_len = htonl(len);

	if (send(sockfd, &hdr, sizeof(hdr), 0) != sizeof(hdr)) {
		perror("send header");
		return -1;
	}
	if (len > 0) {
		if (send(sockfd, json, len, 0) != (ssize_t)len) {
			perror("send payload");
			return -1;
		}
	}
	printf("[CLIENT] Sent CMD=0x%04x, user_id=%d, len=%u\n", cmd, user_id, len);
	if (json) printf("[CLIENT] Payload: %s\n", json);
	return 0;
}

// Receive packet
static int recv_packet(int sockfd, uint16_t *cmd, uint16_t *user_id, char **payload, uint32_t *payload_len) {
	PacketHeader hdr;
	ssize_t n = recv(sockfd, &hdr, sizeof(hdr), 0);
	if (n != sizeof(hdr)) {
		if (n == 0) {
			printf("[CLIENT] Server closed connection\n");
		} else {
			perror("recv header");
		}
		return -1;
	}

	*cmd = ntohs(hdr.cmd);
	*user_id = ntohs(hdr.user_id);
	*payload_len = ntohl(hdr.payload_len);

	printf("[CLIENT] Received CMD=0x%04x, user_id=%d, len=%u\n", *cmd, *user_id, *payload_len);

	if (*payload_len > 0) {
		*payload = malloc(*payload_len + 1);
		if (!*payload) {
			perror("malloc");
			return -1;
		}
		n = recv(sockfd, *payload, *payload_len, 0);
		if (n != (ssize_t)*payload_len) {
			perror("recv payload");
			free(*payload);
			return -1;
		}
		(*payload)[*payload_len] = '\0';
		printf("[CLIENT] Payload: %s\n", *payload);
	} else {
		*payload = NULL;
	}

	return 0;
}

// Receive packets until we get the expected command (skip notifications)
static int recv_packet_expect(int sockfd, uint16_t expected_cmd, uint16_t *cmd, 
                               uint16_t *user_id, char **payload, uint32_t *payload_len) {
	int max_tries = 10;
	for (int i = 0; i < max_tries; i++) {
		if (recv_packet(sockfd, cmd, user_id, payload, payload_len) != 0) {
			return -1;
		}
		
		// If this is the expected command, return success
		if (*cmd == expected_cmd) {
			return 0;
		}
		
		// Otherwise, it's a notification - free and continue
		printf("[CLIENT] Skipping notification CMD=0x%04x\n", *cmd);
		if (*payload) {
			free(*payload);
			*payload = NULL;
		}
	}
	
	printf("✗ Did not receive expected CMD=0x%04x after %d tries\n", expected_cmd, max_tries);
	return -1;
}

// Parse JSON to extract a field (simple string search)
static int64_t extract_int64(const char *json, const char *key) {
	char search[128];
	snprintf(search, sizeof(search), "\"%s\":", key);
	const char *p = strstr(json, search);
	if (!p) return 0;
	p += strlen(search);
	while (*p == ' ' || *p == '\t') p++;
	return atoll(p);
}

static char *extract_string(const char *json, const char *key) {
	char search[128];
	snprintf(search, sizeof(search), "\"%s\":\"", key);
	const char *p = strstr(json, search);
	if (!p) return NULL;
	p += strlen(search);
	const char *end = strchr(p, '"');
	if (!end) return NULL;
	size_t len = end - p;
	char *result = malloc(len + 1);
	if (!result) return NULL;
	memcpy(result, p, len);
	result[len] = '\0';
	return result;
}

int main(int argc, char **argv) {
	const char *host = argc > 1 ? argv[1] : "127.0.0.1";
	int port = argc > 2 ? atoi(argv[2]) : SERVER_PORT;

	printf("=== TEST INVITE FLOW ===\n");
	printf("Server: %s:%d\n\n", host, port);

	// Connect two clients
	printf("--- Step 1: Connect 2 clients ---\n");
	int fd_alice = connect_to_server(host, port);
	int fd_bob = connect_to_server(host, port);
	
	if (fd_alice < 0 || fd_bob < 0) {
		printf("Failed to connect clients\n");
		return 1;
	}

	uint16_t cmd, user_id;
	char *payload = NULL;
	uint32_t payload_len;

	// Login Alice
	printf("\n--- Step 2: Alice login ---\n");
	send_packet(fd_alice, CMD_REQ_LOGIN, 0, "{\"username\":\"alice\",\"password\":\"alice123\"}");
	if (recv_packet_expect(fd_alice, CMD_RES_LOGIN, &cmd, &user_id, &payload, &payload_len) == 0) {
		int64_t alice_id = extract_int64(payload, "user_id");
		printf("✓ Alice logged in, user_id=%lld\n", (long long)alice_id);
		free(payload);
	} else {
		printf("✗ Alice login failed\n");
		goto cleanup;
	}

	// Login Bob
	printf("\n--- Step 3: Bob login ---\n");
	send_packet(fd_bob, CMD_REQ_LOGIN, 0, "{\"username\":\"bob\",\"password\":\"bob123\"}");
	if (recv_packet_expect(fd_bob, CMD_RES_LOGIN, &cmd, &user_id, &payload, &payload_len) == 0) {
		int64_t bob_id = extract_int64(payload, "user_id");
		printf("✓ Bob logged in, user_id=%lld\n", (long long)bob_id);
		free(payload);
	} else {
		printf("✗ Bob login failed\n");
		goto cleanup;
	}

	// Alice creates room
	printf("\n--- Step 4: Alice creates 1vN room ---\n");
	send_packet(fd_alice, CMD_REQ_CREATE_ROOM, 0, 
		"{\"easy_count\":5,\"medium_count\":5,\"hard_count\":5}");
	
	int64_t room_id = 0;
	if (recv_packet_expect(fd_alice, CMD_RES_CREATE_ROOM, &cmd, &user_id, &payload, &payload_len) == 0) {
		room_id = extract_int64(payload, "room_id");
		printf("✓ Room created, room_id=%lld\n", (long long)room_id);
		free(payload);
	} else {
		printf("✗ Create room failed\n");
		goto cleanup;
	}

	if (room_id == 0) {
		printf("✗ Failed to create room\n");
		goto cleanup;
	}

	// Wait for friend status notifications (optional - may receive after login)
	printf("\n--- Step 5: Check for friend status updates ---\n");
	sleep(1);

	// Alice invites Bob
	printf("\n--- Step 6: Alice invites Bob to room ---\n");
	char invite_json[256];
	snprintf(invite_json, sizeof(invite_json), 
		"{\"room_id\":%lld,\"friend_id\":2}", (long long)room_id);
	send_packet(fd_alice, 0x0412, 0, invite_json);  // CMD_REQ_INVITE_FRIEND
	
	// Alice should get success response
	if (recv_packet(fd_alice, &cmd, &user_id, &payload, &payload_len) == 0) {
		if (cmd == 0x0413) {  // CMD_RES_INVITE_FRIEND
			printf("✓ Alice received invite response\n");
			if (payload) {
				printf("  Response: %s\n", payload);
				free(payload);
			}
		} else {
			printf("✗ Unexpected response: 0x%04x\n", cmd);
			if (payload) free(payload);
		}
	}

	// Bob should receive invite notification
	printf("\n--- Step 7: Bob receives invite notification ---\n");
	bool invite_received = false;
	for (int i = 0; i < 3; i++) {  // Try multiple times
		if (recv_packet(fd_bob, &cmd, &user_id, &payload, &payload_len) == 0) {
			printf("  Bob received CMD=0x%04x\n", cmd);
			if (cmd == 0x0414) {  // CMD_NOTIFY_ROOM_INVITE
				int64_t inv_room_id = extract_int64(payload, "room_id");
				int64_t from_user_id = extract_int64(payload, "from_user_id");
				char *from_username = extract_string(payload, "from_username");
				
				printf("✓ Bob received room invite!\n");
				printf("  room_id=%lld, from_user_id=%lld, from_username=%s\n", 
					(long long)inv_room_id, (long long)from_user_id, 
					from_username ? from_username : "NULL");
				
				if (from_username) free(from_username);
				invite_received = true;
				free(payload);
				break;
			}
			if (payload) free(payload);
		}
		usleep(500000);  // Wait 0.5s between attempts
	}

	if (!invite_received) {
		printf("✗ Bob did not receive invite notification\n");
		goto cleanup;
	}

	// Bob accepts invite
	printf("\n--- Step 8: Bob accepts invite ---\n");
	char respond_json[256];
	snprintf(respond_json, sizeof(respond_json), 
		"{\"room_id\":%lld,\"accept\":1}", (long long)room_id);
	send_packet(fd_bob, 0x0415, 0, respond_json);  // CMD_REQ_RESPOND_INVITE
	
	// Bob should get accept confirmation (MANUAL-JOIN: no auto-join)
	if (recv_packet_expect(fd_bob, 0x0416, &cmd, &user_id, &payload, &payload_len) == 0) {
		printf("✓ Bob received accept confirmation\n");
		printf("  Response: %s\n", payload);
		free(payload);
	} else {
		printf("✗ Accept confirmation failed\n");
		goto cleanup;
	}

	// Bob joins room (MANUAL-JOIN: explicit join after accept)
	printf("\n--- Step 9: Bob manually joins room ---\n");
	char join_json[256];
	snprintf(join_json, sizeof(join_json), "{\"room_id\":%lld}", (long long)room_id);
	send_packet(fd_bob, CMD_REQ_JOIN_ROOM, 0, join_json);
	
	if (recv_packet_expect(fd_bob, CMD_RES_JOIN_ROOM, &cmd, &user_id, &payload, &payload_len) == 0) {
		printf("✓ Bob joined room successfully\n");
		printf("  Response: %s\n", payload);
		free(payload);
	} else {
		printf("✗ Join room failed\n");
		goto cleanup;
	}

	// Check for room update notifications
	printf("\n--- Step 10: Check for room update notifications ---\n");
	sleep(1);
	
	// Alice should receive room update
	while (recv_packet(fd_alice, &cmd, &user_id, &payload, &payload_len) == 0) {
		if (cmd == CMD_NOTIFY_ROOM_UPDATE) {
			printf("✓ Alice received room update notification\n");
			if (payload) {
				printf("  Members: %s\n", payload);
				free(payload);
			}
			break;
		}
		if (payload) free(payload);
	}

	printf("\n=== TEST COMPLETED ===\n");
	printf("✓ Invite flow working correctly!\n");

cleanup:
	close(fd_alice);
	close(fd_bob);
	return 0;
}
