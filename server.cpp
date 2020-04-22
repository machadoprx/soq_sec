#include "server.h"

int read_from_client(int client_des, uint8_t buffer[], int *n, size_t chunk_size) {
    int nbytes = recv(client_des, buffer, chunk_size, 0);
    *n = nbytes;
    if (nbytes < 0)
        return READ;
    else if (nbytes == 0)
        return END;
    else return OK;
}

int write_to_client(soq_sec *host, int server_socket, int client_socket, fd_set *active, int id, uint8_t buffer[], int len, int max) {
    for (int i = 0; i <= max; i++) {
        if (FD_ISSET(i, active)) {
            if (i != server_socket && i != client_socket) {
                if (send(i, buffer, len, 0) < 0){
                    continue;
                }
                else {
                    std::cout << "msg sent to " << host->connected_clients[i].user_name << "\n";
                }
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
    recv_wait(client, user.user_name, 20, 1, 3);
    user.socket_desc = client;

    //copy new partial key
    memcpy(user.partial_key, host->server_key, 130);

    //send number of iter
    uint8_t size[1];
    size[0] = (uint8_t)host->connected_clients.size() + 1;
    send_wait(client, size, 1, 1, 3);

    // update server new key
    send_wait(client, host->server_key, 130, 1, 3);
    // after mul by new client private key:
    recv_wait(client, host->server_key, 130, 1, 3);
    // update clients key
    for (int i = 4; i < size[0] + 3; i++) {
        send_wait(client, host->connected_clients[i].partial_key, 130, 1, 3);
        // after mul by new client private key:
        recv_wait(client, host->connected_clients[i].partial_key, 130, 1, 3);
    }

    host->connected_clients[client] = user;
    
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
    ec_t curve;
    ec_init_c25519(curve);
    big_to_str(&curve.G.X, (char*)host->server_key);
    big_to_str(&curve.G.Z, (char*)(host->server_key + 65));

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
                    int n;
                    uint8_t buffer[chunk_size];
                    if (read_from_client(i, buffer, &n, chunk_size) != OK) {
                        close(i);   
                        FD_CLR(i, &active_fd_set);
                    }
                    else {
                        int id = (int)ntohs(client_name.sin6_port);
                        write_to_client(host, server_socket, i, &active_fd_set, id, buffer, n, fd_max);
                    }
                }
            }
        }
    }
    return status;
}