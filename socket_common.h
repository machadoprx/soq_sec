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
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include <random>
#include <sstream>

#define MAX_CONN 50
#define BUFF_SIZE 4096

using namespace std;

enum socket_type{SERVER, CLIENT};
enum err_t{OK, END, SELECT, ACCEPT, LISTEN, BIND, SOCKET, CONNECT, PARSE, CONVERSION, REVERSE, READ, WRITE};
enum cmd_t{CON_SERVER, QUIT, PING, JOIN, NICKNAME, KICK, MUTE, UNMUTE, WHOIS, NOT_FOUND};

typedef struct _client {
    int32_t desc;
    uint8_t name[20];
    uint8_t chan[20];
    struct sockaddr_in info;
} client_t;

typedef struct _channel_t {
    int32_t admin_desc;
    uint8_t name[20];
    vector<shared_ptr<client_t>> members;
    vector<shared_ptr<client_t>> muted;
} channel_t;

typedef struct _soqueto {
    uint16_t port;
    int32_t socket_desc;
    struct sockaddr_in channel;
    uint8_t address[17];
    enum socket_type type;
    vector<shared_ptr<client_t>> clients;
    vector<shared_ptr<channel_t>> channels;
} soqueto;

void catch_alarm (int sig);
int make_soqueto(soqueto *res, enum socket_type type, const char *address, const uint16_t port);
int send_wait(int sock, uint8_t *data, int len, int sleep_time, int tries);
int recv_wait(int sock, uint8_t *data, int len, int sleep_time, int tries);
int get_command(const char* command);

#endif