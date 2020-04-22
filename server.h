#ifndef SERVER_H
#define SERVER_H
#include "socket_common.h"

int connect_socket(soq_sec *sock);
int read_from_client(int, uint8_t[], int*, size_t);
int start_listen(soq_sec *host, size_t chunk_size);

#endif
