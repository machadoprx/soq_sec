#include "server.h"

template<class T>
int get_index_by_name(vector<shared_ptr<T>> list, char *name) {
    int obj_index = 0;
    bool found = false;

    for (auto const& obj : list) {
        if (strcmp(name, (char*)obj->name) == 0){
            found = true;
            break;
        }
        obj_index++;
    }

    if (found == false)
        obj_index = -1;
    cout << obj_index << '\n';

    return obj_index;
}

template<class T>
int get_index_by_desc(vector<shared_ptr<T>> list, int desc) {
    int obj_index = 0;
    bool found = false;

    for (auto const& obj : list) {
        if (obj->desc == desc){
            found = true;
            break;
        }
        obj_index++;
    }

    if (found == false)
        obj_index = -1;
    cout << obj_index << '\n';
    return obj_index;
}

int write_to_client(soq_sec *host, int sender_index, fd_set *active, uint8_t buffer[], uint8_t ephem_pbk[], int len) {
    
    auto sender = host->clients.at(sender_index);
    if (strcmp((char*)sender->chan, "NONE") == 0) {
        return OK;
    }

    for (auto const& recipient : host->clients) {
        if (FD_ISSET(recipient->desc, active)) {
            if (strcmp((char*)recipient->chan, (char*)sender->chan) == 0) {
                send_wait(recipient->desc, buffer, len, 500, 5);
                usleep(250);
                send_wait(recipient->desc, ephem_pbk, 130, 500, 5);
                usleep(250);
                send_wait(recipient->desc, sender->name, 20, 500, 5);
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

    auto user = make_shared<client_t>();
    auto num = std::rand() & 0xffffffff;
    sprintf((char*)user->name, "user%d", num);

    user->desc = client;
    user->info = *client_name;
    char no_group[20] = "NONE";
    memcpy(user->chan, no_group, 20);
    host->clients.push_back(user);

    send_wait(client, host->session_pvk, 65, 500, 5);
    FD_SET(client, active_fd_set);
    if (client > (*fd_max)) {
        (*fd_max) = client;
    }

    std::cout << user->name << ", ip " << client_name->sin6_addr.__in6_u.__u6_addr16 << " joined\n";
    return OK;
}

void remove_from_chan(soq_sec *host, int user_index) {
    auto user = host->clients.at(user_index);

    int chan_index = get_index_by_name(host->channels, (char*)user->chan);
    auto chan = host->channels.at(chan_index);

    int mem_index = get_index_by_desc(chan->members, user->desc);
    chan->members.erase(chan->members.begin() + mem_index);

    // if chan still have members delete and delegate admin, if necessary
    if (chan->members.size() > 0) {
        if (chan->admin_desc == user->desc) {
            chan->admin_desc = chan->members.at(0)->desc;
        }
    }
    else {
        // remove chan from server if it is now empty
        host->channels.erase(host->channels.begin() + chan_index);
    }
}

void disconnect_client(soq_sec *host, fd_set *active_fd_set, int user_index) {
    auto user = host->clients.at(user_index);

    if (strcmp((char*)user->chan, "NONE") != 0) {  
        remove_from_chan(host, user_index);
    }
    // remove user from server
    host->clients.erase(host->clients.begin() + user_index);
    close(user->desc);
    FD_CLR(user->desc, active_fd_set);
}

void join_client(soq_sec *host, uint8_t chan[20], int user_index) {
    auto user = host->clients.at(user_index);
    // check if user already joinned a channel
    if (strcmp((char*)user->chan, "NONE") != 0) {
        remove_from_chan(host, user_index);
    }
    // if target channel is user's current
    else if (strcmp((char*)user->chan, (char*)chan) == 0) {
        return;
    }

    memcpy(user->chan, chan, 20);
    int chan_index = get_index_by_name(host->channels, (char*)chan);
    // create chan if it does not exist
    if (chan_index == -1) {
        shared_ptr<channel_t> new_chan = make_shared<channel_t>();
        new_chan->admin_desc = user->desc;
        memcpy(new_chan->name, chan, 20);
        new_chan->members.push_back(user);
        host->channels.push_back(new_chan);
    }
    else {
        host->channels.at(chan_index)->members.push_back(user);
    }
}

void manage_handler(soq_sec *host, uint8_t *buffer, int chunk_size, int user_index, fd_set *active_fd_set) {

    char cmd_str[20];
    sscanf((char*)buffer, "%s", cmd_str);
    int command = get_command(cmd_str);

    auto &user = host->clients.at(user_index);

    uint8_t target[20];

    // get command target
    if (command == NICKNAME || command == JOIN || command == KICK 
        || command == MUTE || command == UNMUTE || command == WHOIS) {
        sscanf((char*)buffer, "%*s %s", (char*)target);
    }

    if (command == PING){
        //strcpy((char*)pang, "pong");
        cout << "pong\n";
        //send_wait(client_desc, pang, chunk_size, 500, 5);
    }

    else if (command == QUIT){
        disconnect_client(host, active_fd_set, user_index);
    }

    else if (command == NICKNAME){
        memcpy(user->name, target, 20);
    }
    
    else if (command == JOIN){
        join_client(host, target, user_index);
    }

    /*else if (command == KICK){
        int target_index = get_index_by_name(host->channels[user->chan], (char*)target);
        auto &targ = host->channels[user->chan].at(target_index);
        host->channels[user->chan].erase(host->channels[user->chan].begin() + target_index);
        
        cout << targ->chan << '\n';
        targ->chan = string("NONE");
        targ->muted = false;

        target_index = get_index_by_desc(host->connected_clients, targ->socket_desc);
        auto &t_true = host->connected_clients.at(target_index);

        cout << t_true->chan << '\n';
    }

    else if (command == MUTE){
        int target_index = get_index_by_name(host->connected_clients, (char*)target);
        auto &t_true = host->connected_clients.at(target_index);
        t_true->muted = true;
    }

    else if (command == UNMUTE){
        int target_index = get_index_by_name(host->connected_clients, (char*)target);
        auto &t_true = host->connected_clients.at(target_index);
        t_true->muted = false;
    }

    else if (command == WHOIS){
        int target_index = get_index_by_name(host->channels[user->chan], (char*)target);
        auto &targ = host->channels[user->chan].at(target_index);
        cout << targ->user_info.sin6_addr.__in6_u.__u6_addr16 << '\n';
    }*/

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
                    
                    int user_index = get_index_by_desc(host->clients, i);
                    uint8_t buffer[chunk_size];

                    if (recv_wait(i, buffer, chunk_size, 250, 5) < 0) {
                        disconnect_client(host, &active_fd_set, user_index);
                    }
                    else {
                        if (buffer[0] == '/') {
                            manage_handler(host, buffer, chunk_size, user_index, &active_fd_set);
                        }
                        else {
                            usleep(250);
                            uint8_t ephem_pbk[130];
                            recv_wait(i, ephem_pbk, 130, 250, 5);
                            write_to_client(host, user_index, &active_fd_set, buffer, ephem_pbk, chunk_size);
                        }
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