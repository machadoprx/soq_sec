#ifndef SOCK_COMMON_H
#define SOCK_COMMON_H

extern "C" {
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
    #include <signal.h>
    #include <sys/wait.h>
}
#include "MeekSpeak-Crypto/bn/bn.h"
#include "MeekSpeak-Crypto/ecc/ecc_25519.h" 
#include "MeekSpeak-Crypto/hash/hash.h" 
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <random>

#define MAX_CONN 50
#define BUFF_SIZE 4096

enum socket_type{SERVER, CLIENT};
enum err_t{OK, END, SELECT, ACCEPT, LISTEN, BIND, SOCKET, CONNECT, PARSE, CONVERSION, REVERSE, READ, WRITE};

typedef struct _channel {
    std::string name;
    std::vector<std::string> members;
    std::string admin;
    int admin_desc;
} channel_t;

typedef struct _client {
    uint8_t user_name[20];
    uint8_t chan[20] = "NONE";
    int32_t socket_desc;
    struct sockaddr_in6 user_info;
} client_t;

typedef struct _soq_sec {
    uint16_t port;
    int32_t socket_desc;
    struct sockaddr_in6 channel;
    uint8_t session_pvk[65]; //m
    uint8_t address[17];
    enum socket_type type;
    std::vector<client_t> connected_clients;
    std::vector<channel_t> channels;
} soq_sec;

void display_error(int e);
void catch_alarm (int sig);
int make_soq_sec(soq_sec *res, enum socket_type type, const char *address, const uint16_t port);
int send_wait(int sock, uint8_t *data, int len, int sleep_time, int tries);
int recv_wait(int sock, uint8_t *data, int len, int sleep_time, int tries);

#endif