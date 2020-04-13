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
    
    if (type == SERVER) {
        int on = 1;
        setsockopt(res->socket_desc, SOL_SOCKET, SO_REUSEADDR, (byte_t*) &on, sizeof(int));
        if (bind(res->socket_desc, (struct sockaddr *) &res->channel, sizeof(res->channel)) < 0) {
            return BIND;
        }
    }

    if (inet_pton(AF_INET6, address, &res->channel.sin6_addr) < 0) {
        return CONVERSION;
    }
    return OK;
}

int read_from_client(int filedes, struct sockaddr_in6 client_name, size_t chunk_size) {
    byte_t buffer[chunk_size];
    int nbytes = read(filedes, buffer, chunk_size);
    if (nbytes < 0) {
        return READ;
    }
    else if (nbytes == 0)
        return END;
    else {
        buffer[nbytes] = '\0';
        fprintf(stdout, "bitch %hd : \"%s\" \n", ntohs(client_name.sin6_port), buffer);
        return OK;
    }
}

int start_listen(soq_sec *host, int connections, size_t chunk_size , int (*callback)(int, struct sockaddr_in6, size_t)) {
    
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
    int max_conn = connections > FD_SETSIZE ? FD_SETSIZE : connections;

    while (true) {
        read_fd_set = active_fd_set;
        if (select (max_conn, &read_fd_set, NULL, NULL, NULL) < 0) {
            status = SELECT;
        }
        for (int i = 0; i < max_conn; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                struct sockaddr_in6 client_name;
                if (i == server_socket) {
                    socklen_t client_size = sizeof(client_name);
                    int client = accept(server_socket, (struct sockaddr *) &client_name, &client_size);
                    if (client < 0) {
                        status = ACCEPT;
                        continue;
                    }
                    fprintf(stdout,
                            "new bitch, port %hd joined\n",
                            ntohs(client_name.sin6_port));
                    FD_SET(client, &active_fd_set);
                }
                else {
                    if (callback(i, client_name, chunk_size) == READ) {
                        close(i);
                        FD_CLR(i, &active_fd_set);
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

int write_to_server(soq_sec *sock, char *msg) {
    if (write(sock->socket_desc, msg, strlen(msg) + 1) < 0){
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