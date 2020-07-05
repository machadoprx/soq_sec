#include "client.h"

int send_handler(soqueto *sock) {

    uint8_t msg[BUFF_SIZE];
    memset(msg, 0, BUFF_SIZE);
    
    while (true) {
        if (fscanf(stdin, "\n%[^\n]", msg) > 0) {
            send_wait(sock->socket_desc, msg, BUFF_SIZE, 250, 5);
            if (strcmp((char*)msg, "/quit") == 0)
                break;
            memset(msg, 0, BUFF_SIZE);
        }
    }
    wait(NULL);
    return OK;
}

int recv_handler(soqueto *sock) {

    uint8_t msg[BUFF_SIZE];
    memset(msg, 0, BUFF_SIZE);
    
    while (true) {
        if (recv_wait(sock->socket_desc, msg, BUFF_SIZE, 250, 5) > 0) {
            cout << msg << '\n';
            memset(msg, 0, BUFF_SIZE);
        }
        else {
            cout << "disconnected from server, closing connection...\n";
            break;
        }
    }
    close(sock->socket_desc);
    signal(SIGALRM, catch_alarm);
    alarm(2);
    wait(NULL);
    return OK;
}

int connect_socket(soqueto *sock) {

    if (connect(sock->socket_desc, (struct sockaddr *)&sock->channel, sizeof(sock->channel)) < 0) {
        return CONNECT;
    }

    signal(SIGINT, SIG_IGN);
    char greetings[BUFF_SIZE];
    recv_wait(sock->socket_desc, (uint8_t*)greetings, BUFF_SIZE, 250, 5);
    cout << greetings;

    pid_t child_pid = fork();

    if (child_pid == 0) {
        send_handler(sock);
    }
    else if (child_pid > 0){
        recv_handler(sock);
    }

    return OK;
}