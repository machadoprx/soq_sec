#include "socket_common.h"

int make_soqueto(soqueto *res, enum socket_type type, const char* address, const uint16_t port) {

    strcpy((char*)res->address, address);

    res->type = type;
    res->port = port;
    res->socket_desc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (res->socket_desc < 0) {
        return SOCKET;
    }
    
    res->channel.sin_family = AF_INET;
    res->channel.sin_port = htons(port);
    if (inet_aton(address, &res->channel.sin_addr) < 0) {
        return CONVERSION;
    }

    if (type == SERVER) {
        int on = 1;
        setsockopt(res->socket_desc, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(int));
        if (bind(res->socket_desc, (struct sockaddr *) &res->channel, sizeof(res->channel)) < 0) {
            return BIND;
        }
    }
    return OK;
}

int send_wait(int sock, uint8_t *data, int len, int sleep_time, int tries) {
    int success = -1;
    for (int t = 0; t < tries; t++) {
        success = send(sock, data, len, 0);
        if (success < 0) {
            usleep(sleep_time);
        }
        else if(success > 0) break;
    }
    return success;
}

int recv_wait(int sock, uint8_t *data, int len, int sleep_time, int tries) {
    int success = -1;
    for (int t = 0; t < tries; t++) {
        success = recv(sock, data, len, 0);
        if (success < 0) {
            usleep(sleep_time);
        }
        else if(success > 0) break;
    }
    return success;
}

void catch_alarm (int sig) {
    kill(0, SIGTERM);
}

int get_command(const char* command) {
    if (strcmp(command, "/connect") == 0) {
        return CON_SERVER;
    }
    else if (strcmp(command, "/quit") == 0) {
        return QUIT;
    }
    else if (strcmp(command, "/ping") == 0) {
        return PING;
    }
    else if (strcmp(command, "/join") == 0) {
        return JOIN;
    }
    else if (strcmp(command, "/nickname") == 0) {
        return NICKNAME;
    }
    else if (strcmp(command, "/kick") == 0) {
        return KICK;
    }
    else if (strcmp(command, "/mute") == 0) {
        return MUTE;
    }
    else if (strcmp(command, "/unmute") == 0) {
        return UNMUTE;
    }
    else if (strcmp(command, "/whois") == 0) {
        return WHOIS;
    }
    else {
        return NOT_FOUND;
    }
}