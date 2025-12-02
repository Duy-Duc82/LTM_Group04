// Simple client session helpers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "service/client_session.h"

ClientSession *client_session_new(int socket_fd) {
    ClientSession *s = calloc(1, sizeof(ClientSession));
    if (!s) return NULL;
    s->socket_fd = socket_fd;
    s->user_id = 0;
    s->access_token[0] = '\0';
    return s;
}

void client_session_free(ClientSession *sess) {
    if (!sess) return;
    // If we had ownership of the socket we could close it here;
    // keep it simple: do not close socket here (caller may manage it).
    free(sess);
}

ssize_t client_session_send(ClientSession *sess, const void *buf, size_t len) {
    if (!sess || !buf) return -1;
    if (sess->socket_fd >= 0) {
        ssize_t n = send(sess->socket_fd, buf, len, 0);
        return n;
    }
    // If no socket is present (e.g. unit tests), fall back to stdout
    size_t written = fwrite(buf, 1, len, stdout);
    fflush(stdout);
    return (ssize_t)written;
}
