// Test Friends features via network protocol
// Compile: make build/test_friends_network
// Usage: ./build/test_friends_network [host] [port] [username] [password]

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
		printf("[TEST] Response: %s\n", response);
		// Extract user_id from response
		int64_t user_id_64 = extract_user_id(response);
		if (user_id_64 > 0 && user_id_64 <= UINT16_MAX) {
			*user_id = (uint16_t)user_id_64;
			printf("[TEST] Extracted user_id: %d\n", *user_id);
		} else {
			// Fallback: try to get from search results if available
			*user_id = 1; // Default fallback
			printf("[TEST] Warning: Could not extract user_id, using default: %d\n", *user_id);
		}
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ LOGIN failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_search_user(int sockfd, uint16_t user_id, const char *query) {
	char json[256];
	snprintf(json, sizeof(json), "{\"query\":\"%s\",\"limit\":10}", query);

	printf("\n[TEST] ========== SEARCH USER ==========\n");
	printf("[TEST] Searching for: %s\n", query);
	if (send_packet(sockfd, CMD_REQ_SEARCH_USER, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send SEARCH_USER\n");
		return -1;
	}

	uint16_t cmd;
	char response[2048];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive SEARCH_USER response\n");
		return -1;
	}

	if (cmd == CMD_RES_SEARCH_USER) {
		printf("[TEST] ✓ Search successful\n");
		printf("[TEST] Results: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Search failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_add_friend(int sockfd, uint16_t user_id, int64_t friend_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"friend_id\":%lld}", (long long)friend_id);

	printf("\n[TEST] ========== ADD FRIEND ==========\n");
	printf("[TEST] Sending friend request to user %lld\n", (long long)friend_id);
	if (send_packet(sockfd, CMD_REQ_ADD_FRIEND, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send ADD_FRIEND\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive ADD_FRIEND response\n");
		return -1;
	}

	if (cmd == CMD_RES_ADD_FRIEND) {
		printf("[TEST] ✓ Friend request sent\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Add friend failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_list_friends(int sockfd, uint16_t user_id, char *response_out, size_t response_size) {
	printf("\n[TEST] ========== LIST FRIENDS ==========\n");
	if (send_packet(sockfd, CMD_REQ_LIST_FRIENDS, user_id, "{}") != 0) {
		fprintf(stderr, "[TEST] Failed to send LIST_FRIENDS\n");
		return -1;
	}

	uint16_t cmd;
	char response[2048];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive LIST_FRIENDS response\n");
		return -1;
	}

	if (cmd == CMD_RES_LIST_FRIENDS) {
		printf("[TEST] ✓ Friends list received\n");
		printf("[TEST] Friends: %s\n", response);
		if (response_out && response_size > 0) {
			strncpy(response_out, response, response_size - 1);
			response_out[response_size - 1] = '\0';
		}
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ List friends failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_get_friend_info(int sockfd, uint16_t user_id, int64_t friend_id) {
	char json[256];
	snprintf(json, sizeof(json), "{\"friend_id\":%lld}", (long long)friend_id);

	printf("\n[TEST] ========== GET FRIEND INFO ==========\n");
	printf("[TEST] Getting info for friend_id: %lld (current user_id: %d)\n", (long long)friend_id, user_id);
	printf("[TEST] Sending JSON: %s\n", json);
	if (send_packet(sockfd, CMD_REQ_GET_FRIEND_INFO, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send GET_FRIEND_INFO\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive GET_FRIEND_INFO response\n");
		return -1;
	}

	if (cmd == CMD_RES_GET_FRIEND_INFO) {
		printf("[TEST] ✓ Friend info received\n");
		printf("[TEST] Info: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Get friend info failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_get_pending_requests(int sockfd, uint16_t user_id) {
	printf("\n[TEST] ========== GET PENDING REQUESTS ==========\n");
	if (send_packet(sockfd, CMD_REQ_GET_PENDING_REQ, user_id, "{}") != 0) {
		fprintf(stderr, "[TEST] Failed to send GET_PENDING_REQ\n");
		return -1;
	}

	uint16_t cmd;
	char response[2048];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive GET_PENDING_REQ response\n");
		return -1;
	}

	if (cmd == CMD_RES_GET_PENDING_REQ) {
		printf("[TEST] ✓ Pending requests received\n");
		printf("[TEST] Requests: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Get pending requests failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

static int test_invite_friend(int sockfd, uint16_t user_id, int64_t friend_id, int64_t room_id) {
	// If friend_id is 0, try to find alice (should be friend of testuser1)
	if (friend_id == 0) {
		char json[256];
		snprintf(json, sizeof(json), "{\"query\":\"alice\",\"limit\":1}");
		if (send_packet(sockfd, CMD_REQ_SEARCH_USER, user_id, json) == 0) {
			uint16_t cmd;
			char response[512];
			if (recv_packet(sockfd, &cmd, response, sizeof(response)) == 0 && cmd == CMD_RES_SEARCH_USER) {
				friend_id = extract_user_id(response);
			}
		}
		if (friend_id == 0) {
			printf("[TEST] ⚠ Skipping INVITE_FRIEND: No friend_id provided\n");
			return 0;
		}
	}
	
	char json[256];
	snprintf(json, sizeof(json), "{\"friend_id\":%lld,\"room_id\":%lld}", 
	         (long long)friend_id, (long long)room_id);

	printf("\n[TEST] ========== INVITE FRIEND TO ROOM ==========\n");
	printf("[TEST] Inviting friend_id %lld to room %lld\n", (long long)friend_id, (long long)room_id);
	if (send_packet(sockfd, CMD_REQ_INVITE_FRIEND, user_id, json) != 0) {
		fprintf(stderr, "[TEST] Failed to send INVITE_FRIEND\n");
		return -1;
	}

	uint16_t cmd;
	char response[512];
	if (recv_packet(sockfd, &cmd, response, sizeof(response)) != 0) {
		fprintf(stderr, "[TEST] Failed to receive INVITE_FRIEND response\n");
		return -1;
	}

	if (cmd == CMD_RES_INVITE_FRIEND) {
		printf("[TEST] ✓ Friend invited\n");
		printf("[TEST] Response: %s\n", response);
		return 0;
	} else {
		fprintf(stderr, "[TEST] ✗ Invite friend failed: cmd=0x%04x\n", cmd);
		return -1;
	}
}

int main(int argc, char *argv[]) {
	const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
	const char *port = (argc > 2) ? argv[2] : DEFAULT_PORT;
	const char *username = (argc > 3) ? argv[3] : "testuser";
	const char *password = (argc > 4) ? argv[4] : "testpass";

	printf("=== Friends Network Test ===\n");
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

	// Test search user
	test_search_user(sockfd, user_id, "test");

	// Test add friend (use testuser2, which should be user_id 14 based on search results)
	test_add_friend(sockfd, user_id, 14);

	// Test list friends (this will show us friend user_ids)
	char friends_response[2048] = {0};
	test_list_friends(sockfd, user_id, friends_response, sizeof(friends_response));
	
	// Extract first friend's user_id from JSON array for GET_FRIEND_INFO test
	int64_t first_friend_id = 0;
	if (strlen(friends_response) > 0) {
		// Find first user_id in JSON array
		const char *p = strstr(friends_response, "\"user_id\"");
		if (p) {
			p = strchr(p, ':');
			if (p) {
				// Skip spaces and colon
				while (*p == ' ' || *p == ':') p++;
				first_friend_id = atoll(p);
			}
		}
	}
	
	// Test get friend info (use first friend from list, or fallback to 1)
	if (first_friend_id > 0) {
		printf("[TEST] Using friend_id from friends list: %lld\n", (long long)first_friend_id);
		test_get_friend_info(sockfd, user_id, first_friend_id);
	} else {
		printf("[TEST] ⚠ No friends found, trying with friend_id = 1\n");
		test_get_friend_info(sockfd, user_id, 1);
	}

	// Test get pending requests
	test_get_pending_requests(sockfd, user_id);

	// Test invite friend (use alice's user_id = 1, room_id = 1)
	// Note: This will fail with FRIEND_OFFLINE if alice is not logged in, which is expected
	test_invite_friend(sockfd, user_id, 1, 1);

	printf("\n[TEST] ========== TEST COMPLETED ==========\n");
	close(sockfd);
	return 0;
}

