#include "socket_common.h"

int make_soq_sec(soq_sec *res, enum socket_type type, const char* address, const uint16_t port) {

    strcpy((char*)res->address, address);

    res->type = type;
    res->port = port;
    res->socket_desc = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (res->socket_desc < 0) {
        return SOCKET;
    }
    
    res->channel.sin6_family = AF_INET6;
    res->channel.sin6_port = htons(port);
    if (inet_pton(AF_INET6, address, &res->channel.sin6_addr) < 0) {
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

void display_error(int e) {
    std::string msg;
    switch(e) {
        case OK: return;
        case LISTEN: msg = "error start lister"; break;
        case END: msg = "reached eof"; break;
        case SELECT: msg = "error selecting client sockets"; break;
        case ACCEPT: msg = "error accepting client"; break;
        case BIND: msg = "error binding socket"; break;
        case SOCKET: msg = "error creating socket"; break;
        case CONNECT: msg = "error connecting to server"; break;
        case PARSE: msg = "error parsing ip"; break;
        case CONVERSION: msg = "error converting ip"; break;
        case REVERSE: msg = "no reverse name for ip"; break;
        case READ: msg = "error reading"; break;
        case WRITE: msg = "error writing"; break;
        default: msg = "undefined behaviour"; break;
    }
    std::cerr << msg << '\n';
}