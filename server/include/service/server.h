// socket, event loop
#ifndef SERVER_H
#define SERVER_H

// start a simple server bound to bind_addr (NULL for any) and port (string), e.g. "9000".
// Returns 0 on success or -1 on failure.
int start_server(const char *bind_addr, const char *portstr);

#endif