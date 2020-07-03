#ifndef SERVER_H
#define SERVER_H
#include "socket_common.h"

int connect_socket(soqueto *sock);
int read_from_client(int, uint8_t[], int*, size_t);
int start_listen(soqueto *host, size_t chunk_size);

#endif
