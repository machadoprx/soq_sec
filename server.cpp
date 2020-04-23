#include "server.h"

int write_to_client(soq_sec *host, int server_socket, int client_socket, fd_set *active, int id, uint8_t buffer[], int len, int max) {
    for (int i = 0; i <= max; i++) {
        if (FD_ISSET(i, active)) {
            if (i != server_socket && i != client_socket) {
                send_wait(i, buffer, len, 500, 5);
            }
        }
    }
    return OK;
}

int connection_handler(soq_sec *host, struct sockaddr_in6 *client_name, int server_socket, fd_set *active_fd_set, int *fd_max) {
    socklen_t client_size = sizeof(*client_name);
    int client = accept(server_socket, (struct sockaddr *) client_name, &client_size);
    
    if (client < 0) {
        return ACCEPT;
    }

    client_t user;
    int len = recv_wait(client, user.user_name, 20, 500, 5);
    user.socket_desc = client;
    if (len < 20){
        return OK;
    }
    host->connected_clients[client] = user;
    send_wait(client, host->session_pvk, 65, 500, 5);
    FD_SET(client, active_fd_set);
    if (client > (*fd_max)) {
        (*fd_max) = client;
    }

    std::cout << user.user_name << ", port " << ntohs(client_name->sin6_port) << " joined\n";
    return OK;
}

int start_listen(soq_sec *host, size_t chunk_size) {
    
    int status = OK;

    if (host->type != SERVER) {
        return SERVER;
    }
    int server_socket = host->socket_desc;

    if (listen(server_socket, 1) < 0) {
        return LISTEN;
    }

    big_t m;
    big_rnd_dig(&m);
    m.value[7] &= 0xfffffffull;
    big_to_str(&m, (char*)host->session_pvk);

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
                    connection_handler(host, &client_name, server_socket, &active_fd_set, &fd_max);
                }
                else {
                    uint8_t buffer[chunk_size + 130];
                    if (recv_wait(i, buffer, chunk_size + 130, 500, 5) < 0) {
                        close(i);   
                        FD_CLR(i, &active_fd_set);
                    }
                    else {
                        int id = (int)ntohs(client_name.sin6_port);
                        write_to_client(host, server_socket, i, &active_fd_set, id, buffer, chunk_size + 130, fd_max);
                    }
                }
            }
        }
    }
    return status;
}