#include "client.h"

void set_cmd_line(int show) {
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    if (show == 1) {
        newt.c_lflag |= ECHO;
    }
    else if (show == 0) {
        newt.c_lflag &= ~ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

int send_handler(soqueto *sock) {

    uint8_t msg[BUFF_SIZE];

    while (true) {

        memset(msg, 0, BUFF_SIZE);

        usleep(250);
        
        if (fscanf(stdin, "\n%[^\n]", msg) > 0) {
            if (msg[0] == '/') {
                char cmd_str[15];
                sscanf((char*)msg, "%s", cmd_str);
                int cmd = get_command(cmd_str);
                
                if (cmd != NOT_FOUND) {
                    send_wait(sock->socket_desc, msg, BUFF_SIZE, 250, 5);
                    if (cmd == QUIT) {
                        usleep(500);
                        break;
                    }
                } 
                continue;
            }
            send_wait(sock->socket_desc, msg, BUFF_SIZE, 250, 5);
            usleep(250);
        }
    }
    wait(NULL);
    return OK;
}

int recv_handler(soqueto *sock) {

    uint8_t msg[BUFF_SIZE];

    while (true) {
        memset(msg, 0, BUFF_SIZE);
        if (recv_wait(sock->socket_desc, msg, BUFF_SIZE, 250, 5) > 0) {
            char check_sender[20];
            sscanf((char*)msg, "%s %*s", check_sender);
            if (strcmp(check_sender, "server") == 0) {
                cout << msg << '\n';
                continue;
            }
            cout << msg << '\n';
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