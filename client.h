#ifndef CLIENT_H
#define CLIENT_H
#include "socket_common.h"

extern "C" {
    #include <termios.h>
    #include <unistd.h>
}

enum cmd_t{CON_SERVER, QUIT, PING, JOIN, NICKNAME, KICK, MUTE, UNMUTE, WHOIS, NOT_FOUND};

int read_from_server(soq_sec *sock, uint8_t *buffer, int len);
int write_to_server(soq_sec *sock, uint8_t *msg, int len);
int connect_socket(soq_sec *sock);

#endif