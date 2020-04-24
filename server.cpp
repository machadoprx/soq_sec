#include "server.h"

int get_index_by_desc(vector<client_t> list, int desc) {
    int user_index = 0;
    for (auto const& user : list) {
        if (user.socket_desc == desc)
            break;
        user_index++;
    }
    return user_index;
}

int get_index_by_name(vector<client_t> list, char *name) {
    int user_index = 0;
    for (auto const& user : list) {
        if (strcmp(name, (char*)user.user_name) == 0)
            break;
        user_index++;
    }
    return user_index;
}

int write_to_client(soq_sec *host, int server_socket, int client_socket, fd_set *active, int id, uint8_t buffer[], uint8_t ephem_pbk[], int len, int max) {

    int user_index = get_index_by_desc(host->connected_clients, client_socket);

    uint8_t sender_name[20];
    memcpy(sender_name, host->connected_clients.at(user_index).user_name, 20);

    for (auto const& client : host->connected_clients) {
        if (FD_ISSET(client.socket_desc, active)) {
            send_wait(client.socket_desc, buffer, len, 500, 5);
            usleep(250);
            send_wait(client.socket_desc, ephem_pbk, 130, 500, 5);
            usleep(250);
            send_wait(client.socket_desc, sender_name, 20, 500, 5);
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

    //shared_ptr<client_t> user_ptr = make_shared<client_t>();
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

void manage_handler(soq_sec *host, uint8_t *buffer, int chunk_size, int command, int client_desc, int server_desc, fd_set *active_fd_set) {
    // enum cmd_t{CON_SERVER, QUIT, PING, JOIN, NICKNAME, KICK, MUTE, UNMUTE, WHOIS, NOT_FOUND};
    int user_index = 0;
    for (auto const& user : host->connected_clients) {
        if (user.socket_desc == client_desc)
            break;
        user_index++;
    }

    uint8_t pang[chunk_size];
    client_t &user = host->connected_clients.at(user_index);
    uint8_t target[20];

    if (command == KICK || command == MUTE || command == UNMUTE || command == WHOIS) {
        if (user.admin == false){
            return;
        }
        else {
            sscanf((char*)buffer, "%*s %s", (char*)target);
        }
    }

    switch(command) {
        case PING:
            //strcpy((char*)pang, "pong");
            cout << "pong\n";
            //send_wait(client_desc, pang, chunk_size, 500, 5);
            break;

        case QUIT:
            if (user.chan.compare("NONE") != 0) {
                if (host->channels[user.chan].size() > 1) {
                    int mem_index = get_index_by_desc(host->channels[user.chan], user.socket_desc);
                    host->channels[user.chan].erase(host->channels[user.chan].begin() + mem_index);
                    if (user.admin == true) {
                        host->channels[user.chan].at(0).admin = true;
                    }
                }
                else {
                    host->channels[user.chan].clear();
                }
            }
            host->connected_clients.erase(host->connected_clients.begin() + user_index);
            close(client_desc);   
            FD_CLR(client_desc, active_fd_set);
            break;

        case NICKNAME:
            sscanf((char*)buffer, "%*s %s", (char*)user.user_name);
            break;
            
        case KICK:
            int target_index = get_index_by_name(host->channels[user.chan], (char*)target);
            host->channels[user.chan].erase(host->channels[user.chan].begin() + target_index);
            target_index = get_index_by_name(host->connected_clients, (char*)target);
            client_t &targ = host->connected_clients.at(target_index);
            targ.chan = "NONE";
            targ.muted = false;
            break;
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
                    if (recv_wait(i, buffer, chunk_size, 500, 5) < 0) {
                        //host->connected_clients.erase(host->connected_clients.begin() + (i - server_socket - 1)); // call quit
                        close(i);   
                        FD_CLR(i, &active_fd_set);
                    }
                    else {
                        if (buffer[0] == '/') {
                            char cmd_str[15];
                            sscanf((char*)buffer, "%s", cmd_str);
                            int cmd = get_command(cmd_str);
                            manage_handler(host, buffer, chunk_size, cmd, i, server_socket, &active_fd_set);
                            usleep(250);
                            continue;
                        }
                        usleep(250);
                        uint8_t ephem_pbk[130];
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

    clients_handler(host, chunk_size);

    return status;
}