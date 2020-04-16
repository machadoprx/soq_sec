#include "soq_sec.h"

int make_soq_sec(soq_sec *res, enum socket_type type, const char* address, const uint16_t port) {

    memset(res, 0, sizeof(*res));
    strcpy(res->address, address);

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
        setsockopt(res->socket_desc, SOL_SOCKET, SO_REUSEADDR, (uint8_t*) &on, sizeof(int));
        if (bind(res->socket_desc, (struct sockaddr *) &res->channel, sizeof(res->channel)) < 0) {
            return BIND;
        }
    }
    return OK;
}

int read_from_client(int client_des, uint8_t buffer[], int *n, size_t chunk_size) {
    int nbytes = recv(client_des, buffer, chunk_size, 0);
    *n = nbytes + 1;

    if (nbytes < 0) {
        return READ;
    }
    else if (nbytes == 0)
        return END;
    else {
        buffer[nbytes] = '\0';
        return OK;
    }
}

int write_to_client(int server_socket, int client_socket, fd_set *active, int id, uint8_t buffer[], int len, int max) {
    for (int i = 0; i <= max; i++) {
        if (FD_ISSET(i, active)) {
            if (i != server_socket && i != client_socket) {
                uint8_t msg[9 + len];
                sprintf(msg, "%d : %s", id, buffer);
                msg[8 + len] = '\0';
                if (send(i, msg, len + 9, 0) < 0)
                    continue;
            }
        }
    }
    return OK;
}

int start_listen(soq_sec *host, int connections, size_t chunk_size , int (*callback)(int, uint8_t[], int*, size_t)) {
    
    int status = OK;

    if (host->type != SERVER) {
        return SERVER;
    }
    int server_socket = host->socket_desc;

    if (listen(server_socket, 1) < 0) {
        return LISTEN;
    }
    fd_set active_fd_set, read_fd_set;
    FD_ZERO(&active_fd_set);
    FD_SET(server_socket, &active_fd_set);
    int fd_max = server_socket;

    while(true) {
        read_fd_set = active_fd_set;
        if (select(fd_max + 1, &read_fd_set, NULL, NULL, NULL) < 0) {
            return SELECT;
        }
        for (int i = 0; i <= fd_max; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                struct sockaddr_in6 client_name;
                if (i == server_socket) {
                    socklen_t client_size = sizeof(client_name);
                    int client = accept(server_socket, (struct sockaddr *) &client_name, &client_size);
                    if (client < 0) {
                        display_error(ACCEPT);
                        continue;
                    }
                    FD_SET(client, &active_fd_set);
                    if (client > fd_max) {
                        fd_max = client;
                    }
                    fprintf(stderr,
                            "new bitch, port %hd joined\n",
                            ntohs(client_name.sin6_port));
                }
                else {
                    int n;
                    uint8_t buffer[chunk_size];
                    if (callback(i, buffer, &n, chunk_size) != OK) {
                        close(i);   
                        FD_CLR(i, &active_fd_set);
                    }
                    else {
                        int id = (int)ntohs(client_name.sin6_port);
                        write_to_client(server_socket, i, &active_fd_set, id, buffer, n, fd_max);
                    }
                }
            }
        }
    }
    return status;
}

void display_error(int e) {
    char *msg;
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
    fprintf(stderr, "%s\n", msg);
}

int read_from_server(soq_sec *sock, uint8_t *buffer) {
    if (recv(sock->socket_desc, buffer, strlen((char*)buffer) + 1, 0) < 0){
        return READ;
    }
    return OK;
}

int write_to_server(soq_sec *sock, uint8_t *msg) {
    if (send(sock->socket_desc, msg, strlen((char*)msg) + 1, 0) < 0){
        return WRITE;
    }
    return OK;
}

int connect_socket(soq_sec *sock) {
    if (connect(sock->socket_desc, (struct sockaddr *)&sock->channel, sizeof(sock->channel)) < 0) {
        return CONNECT;
    }
    return OK;
}