#include "server.h"

int write_to_client(soq_sec *host, int server_socket, int client_socket, fd_set *active, int id, uint8_t buffer[], uint8_t ephem_pbk[], int len, int max) {

    int user_index = client_socket - server_socket - 1;

    for (int i = server_socket + 1; i <= max; i++) {
        if (FD_ISSET(i, active)) {
            send_wait(i, buffer, len, 500, 5);
            usleep(250);
            send_wait(i, ephem_pbk, 130, 500, 5);
            usleep(250);
            send_wait(i, host->connected_clients.at(user_index).user_name, 20, 500, 5);
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
    uint32_t rand_user = std::rand() & 0xffffffff;
    sprintf((char*)user.user_name, "user%d", rand_user);
    user.socket_desc = client;
    user.user_info = *client_name;
    host->connected_clients.push_back(user);

    send_wait(client, host->session_pvk, 65, 500, 5);
    FD_SET(client, active_fd_set);
    if (client > (*fd_max)) {
        (*fd_max) = client;
    }

    std::cout << user.user_name << ", port " << ntohs(client_name->sin6_port) << " joined\n";
    return OK;
}

void adm_handler() {
    while (true) {
        uint8_t command[20];
        sleep(1);
        std::cin >> command;
        std::cout << command << " received\n";
    }
}

int clients_handler(soq_sec *host, int chunk_size) {

    int server_socket = host->socket_desc;
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
                    uint8_t buffer[chunk_size];
                    uint8_t ephem_pbk[130];
                    if (recv_wait(i, buffer, chunk_size, 500, 5) < 0) {
                        close(i);   
                        FD_CLR(i, &active_fd_set);
                    }
                    else {
                        usleep(250);
                        recv_wait(i, ephem_pbk, 130, 500, 5);
                        int id = (int)ntohs(client_name.sin6_port);
                        write_to_client(host, server_socket, i, &active_fd_set, id, buffer, ephem_pbk, chunk_size, fd_max);
                    }
                }
            }
        }
    }
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

    std::srand(int(time(NULL)));
    big_t m;
    big_rnd_dig(&m);
    m.value[7] &= 0xfffffffull;
    big_to_str(&m, (char*)host->session_pvk);

    pid_t child = fork();
    if (child == 0) {
        clients_handler(host, chunk_size);
    }
    else if (child > 0) {
        adm_handler();
    }

    return status;
}