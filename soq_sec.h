#include <sys/fcntl.h>
#include <fcntl.h>
#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

typedef unsigned char byte_t;
enum socket_type{SERVER, CLIENT};
enum error_t{OK, END, SELECT, ACCEPT, LISTEN, BIND, SOCKET, CONNECT, PARSE, CONVERSION, REVERSE, READ, WRITE};

typedef struct _soq_sec {
    uint16_t port;
    int32_t socket_desc;
    struct sockaddr_in6 channel;
    char address[30];
    enum socket_type type;
} soq_sec;

int connect_socket(soq_sec *sock);
int write_to_server(soq_sec *sock, char *msg);
void display_error(int e);
int make_soq_sec(soq_sec *res, enum socket_type type, const char *address, const uint16_t port);
int read_from_client(int filedes, struct sockaddr_in6 client_name, size_t chunk_size);
int start_listen(soq_sec *host, int connections, size_t chunk_size, int (*callback)(int, struct sockaddr_in6, size_t));