// Simple multi-client test program
// Compile: gcc -o test_multiclient test_multiclient.c -Wall
// Usage: ./test_multiclient [num_clients] [host] [port]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define DEFAULT_CLIENTS 5
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

static int send_login_packet(int sockfd, int client_id) {
    char json[256];
    snprintf(json, sizeof(json), "{\"username\":\"testuser%d\",\"password\":\"testpass\"}", client_id);
    uint32_t json_len = strlen(json);

    typedef struct {
        uint16_t cmd;
        uint16_t user_id;
        uint32_t length;
    } PacketHeader;

    PacketHeader hdr;
    hdr.cmd = htons(0x0103);  // CMD_REQ_LOGIN
    hdr.user_id = htons(0);
    hdr.length = htonl(json_len);

    if (send(sockfd, &hdr, sizeof(hdr), 0) != sizeof(hdr)) return -1;
    if (send(sockfd, json, json_len, 0) != (ssize_t)json_len) return -1;

    return 0;
}

static void client_process(int client_id, const char *host, const char *port) {
    int sockfd = connect_to_server(host, port);
    if (sockfd < 0) {
        fprintf(stderr, "Client %d: Failed to connect\n", client_id);
        exit(1);
    }

    printf("Client %d: Connected\n", client_id);
    send_login_packet(sockfd, client_id);
    printf("Client %d: Sent login packet\n", client_id);

    char buffer[1024];
    ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        printf("Client %d: Received response (%zd bytes)\n", client_id, n);
    }

    sleep(2);
    close(sockfd);
    printf("Client %d: Disconnected\n", client_id);
    exit(0);
}

int main(int argc, char *argv[]) {
    int num_clients = argc > 1 ? atoi(argv[1]) : DEFAULT_CLIENTS;
    const char *host = argc > 2 ? argv[2] : DEFAULT_HOST;
    const char *port = argc > 3 ? argv[3] : DEFAULT_PORT;

    printf("Testing %d clients on %s:%s\n\n", num_clients, host, port);

    for (int i = 1; i <= num_clients; i++) {
        if (fork() == 0) {
            client_process(i, host, port);
        }
        usleep(100000); // 100ms delay
    }

    for (int i = 0; i < num_clients; i++) {
        wait(NULL);
    }

    printf("\nTest completed!\n");
    return 0;
}