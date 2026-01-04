// Test room chat in WAITING state: length limit and rate limit
// Build: make build/test_room_waiting_chat
// Run: ./build/test_room_waiting_chat [host] [port] [user1] [pass1] [user2] [pass2]

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

static int64_t extract_room_id(const char *json) {
    const char *p = strstr(json, "\"room_id\"");
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    while (*p == ' ' || *p == ':') p++;
    return atoll(p);
}

static int login_user(int sockfd, const char *username, const char *password, uint16_t *out_user_id) {
    char json[256];
    snprintf(json, sizeof(json), "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    if (send_packet(sockfd, CMD_REQ_LOGIN, 0, json) != 0) return -1;

    uint16_t cmd;
    char resp[512];
    if (recv_packet(sockfd, &cmd, resp, sizeof(resp)) != 0) return -1;
    if (cmd != CMD_RES_LOGIN) {
        fprintf(stderr, "LOGIN failed: cmd=0x%04x payload=%s\n", cmd, resp);
        return -1;
    }

    const char *p = strstr(resp, "\"user_id\"");
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    while (*p == ' ' || *p == ':') p++;
    *out_user_id = (uint16_t)atoi(p);
    return 0;
}

static int create_room(int sockfd, uint16_t user_id, int64_t *out_room_id) {
    if (send_packet(sockfd, CMD_REQ_CREATE_ROOM, user_id, "{}") != 0) return -1;
    uint16_t cmd;
    char resp[512];
    if (recv_packet(sockfd, &cmd, resp, sizeof(resp)) != 0) return -1;
    if (cmd != CMD_RES_CREATE_ROOM) {
        fprintf(stderr, "CREATE_ROOM failed: cmd=0x%04x payload=%s\n", cmd, resp);
        return -1;
    }
    *out_room_id = extract_room_id(resp);
    return (*out_room_id > 0) ? 0 : -1;
}

static int join_room(int sockfd, uint16_t user_id, int64_t room_id) {
    char json[256];
    snprintf(json, sizeof(json), "{\"room_id\":%lld}", (long long)room_id);
    if (send_packet(sockfd, CMD_REQ_JOIN_ROOM, user_id, json) != 0) return -1;
    uint16_t cmd;
    char resp[512];
    if (recv_packet(sockfd, &cmd, resp, sizeof(resp)) != 0) return -1;
    if (cmd != CMD_RES_JOIN_ROOM) {
        fprintf(stderr, "JOIN_ROOM failed: cmd=0x%04x payload=%s\n", cmd, resp);
        return -1;
    }
    return 0;
}

static int send_room_chat(int sockfd, uint16_t user_id, int64_t room_id, const char *msg, uint16_t *out_cmd, char *resp, size_t resp_sz) {
    char json[512];
    snprintf(json, sizeof(json), "{\"room_id\":%lld,\"message\":\"%s\"}", (long long)room_id, msg);
    if (send_packet(sockfd, CMD_REQ_SEND_ROOM_CHAT, user_id, json) != 0) return -1;
    
    // May receive NOTIFY_ROOM_CHAT before RES_SEND_ROOM_CHAT, so keep reading until we get the response
    int attempts = 0;
    while (attempts < 10) {
        if (recv_packet(sockfd, out_cmd, resp, resp_sz) != 0) return -1;
        if (*out_cmd == CMD_RES_SEND_ROOM_CHAT) return 0;
        if (*out_cmd == CMD_NOTIFY_ROOM_CHAT) {
            attempts++;
            continue; // Drain broadcast, keep reading
        }
        return 0; // Other command, return as-is for caller to handle
    }
    return -1; // Too many notifications without response
}

int main(int argc, char *argv[]) {
    const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
    const char *port = (argc > 2) ? argv[2] : DEFAULT_PORT;
    const char *user1 = (argc > 3) ? argv[3] : "alice";
    const char *pass1 = (argc > 4) ? argv[4] : "alice123";
    const char *user2 = (argc > 5) ? argv[5] : "bob";
    const char *pass2 = (argc > 6) ? argv[6] : "bob123";

    int s1 = connect_to_server(host, port);
    int s2 = connect_to_server(host, port);
    if (s1 < 0 || s2 < 0) {
        fprintf(stderr, "Failed to connect sockets\n");
        return 1;
    }

    uint16_t uid1 = 0, uid2 = 0;
    if (login_user(s1, user1, pass1, &uid1) != 0) {
        fprintf(stderr, "Login user1 failed\n");
        return 1;
    }
    if (login_user(s2, user2, pass2, &uid2) != 0) {
        fprintf(stderr, "Login user2 failed\n");
        return 1;
    }

    int64_t room_id = 0;
    if (create_room(s1, uid1, &room_id) != 0) {
        fprintf(stderr, "Create room failed\n");
        return 1;
    }
    if (join_room(s2, uid2, room_id) != 0) {
        fprintf(stderr, "Join room failed\n");
        return 1;
    }

    // Drain any pending NOTIFY_ROOM_UPDATE from both sockets after join
    fd_set readfds;
    struct timeval tv;
    uint16_t drain_cmd;
    char drain_buf[512];
    for (int sock = 0; sock < 2; sock++) {
        int fd = (sock == 0) ? s1 : s2;
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms
            int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
            if (ret <= 0 || !FD_ISSET(fd, &readfds)) break;
            if (recv_packet(fd, &drain_cmd, drain_buf, sizeof(drain_buf)) != 0) break;
        }
    }

    printf("[TEST] Send valid short message\n");
    uint16_t cmd;
    char resp[512];
    if (send_room_chat(s1, uid1, room_id, "hello", &cmd, resp, sizeof(resp)) != 0) {
        fprintf(stderr, "Room chat send failed (network error)\n");
        return 1;
    }
    if (cmd != CMD_RES_SEND_ROOM_CHAT) {
        fprintf(stderr, "Room chat unexpected response: cmd=0x%04x, payload=%s\n", cmd, resp);
        return 1;
    }
    printf("[TEST] ✓ Valid message sent successfully\n");

    printf("[TEST] Send over-length message (expect MESSAGE_TOO_LONG)\n");
    char longmsg[256];
    memset(longmsg, 'a', sizeof(longmsg));
    longmsg[210] = '\0';
    if (send_room_chat(s1, uid1, room_id, longmsg, &cmd, resp, sizeof(resp)) != 0 || cmd != CMD_RES_SEND_ROOM_CHAT || strstr(resp, "MESSAGE_TOO_LONG") == NULL) {
        fprintf(stderr, "Expected MESSAGE_TOO_LONG, got cmd=0x%04x payload=%s\n", cmd, resp);
        return 1;
    }

    printf("[TEST] Send 6 quick messages (expect RATE_LIMITED on or before 6th)\n");
    int rate_hit = 0;
    for (int i = 0; i < 6; i++) {
        char text[32];
        snprintf(text, sizeof(text), "m%d", i);
        if (send_room_chat(s1, uid1, room_id, text, &cmd, resp, sizeof(resp)) != 0) {
            fprintf(stderr, "Send failed at %d\n", i);
            return 1;
        }
        if (strstr(resp, "RATE_LIMITED")) {
            rate_hit = 1;
            break;
        }
        usleep(200000); // 0.2s spacing
    }
    if (!rate_hit) {
        fprintf(stderr, "Rate limit not triggered\n");
        return 1;
    }

    printf("[TEST] All checks passed\n");
    close(s1);
    close(s2);
    return 0;
}
