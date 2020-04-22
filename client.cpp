#include "client.h"

void set_cmd_line() {
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

int read_from_server(soq_sec *sock, uint8_t *buffer, int len) {
    if (recv(sock->socket_desc, buffer, len, 0) <= 0){
        return READ;
    }
    return OK;
}

int write_to_server(soq_sec *sock, uint8_t *msg, int len) {
    if (send(sock->socket_desc, msg, len, 0) <= 0){
        return WRITE;
    }
    return OK;
}

int write_handler(soq_sec *sock, ec_t *curve, big_t *p, big_t *np, big_t *k) {
    
    set_cmd_line();

    uint32_t shared_key[8]; // shared private key between clients (not server), how?
    // agree key
    for (int i = 0; i < 8; ++i) {
        shared_key[i] = i;
    }
    uint32_t nonce[3] = {0x0, 0x4d, 0x0};

    uint8_t plain_text[BUFF_SIZE];
    uint8_t cipher[BUFF_SIZE];
    uint8_t buffer[BUFF_SIZE];

    while (true) {
        memset(buffer, 0, BUFF_SIZE);
        memset(plain_text, 0, BUFF_SIZE);
        if (fscanf(stdin, "\n%[^\n]", plain_text) > 0) {
            //display_error(write_to_server(sock, (uint8_t *)"REQ_KEY", BUFF_SIZE)); 
            // get session_key(recv)
            chacha_enc(shared_key, nonce, plain_text, cipher, BUFF_SIZE);
            fprintf(stdout, "%s\n", plain_text);
            display_error(write_to_server(sock, cipher, BUFF_SIZE)); 
        }
    }
    return OK;
}

int recv_handler(soq_sec *sock, ec_t *curve, big_t *p, big_t *np, big_t *k) {

    uint32_t shared_key[8]; // shared private key between clients (not server), how?
    // agree key

    for (int i = 0; i < 8; ++i) {
        shared_key[i] = i;
    }
    uint32_t nonce[3] = {0x0, 0x4d, 0x0};

    uint8_t plain_text[BUFF_SIZE];
    uint8_t msg[BUFF_SIZE];

    while (true) {
        if (read_from_server(sock, msg, BUFF_SIZE) == OK) {
            memset(plain_text, 0, BUFF_SIZE);
            // get session_key
            chacha_enc(shared_key, nonce, msg, plain_text, BUFF_SIZE);
            fprintf(stdout, "%s\n", plain_text);
            memset(msg, 0, BUFF_SIZE);
        }
        else {
            fprintf(stdout, "disconnected from server\n"); 
            //fprintf(stderr, "trying to reconnect\n");
            break;
        }
    }
    close(sock->socket_desc);
    wait(NULL);
    return OK;
}

void upd_shared_key(soq_sec *sock, int size, ec_t *curve, big_t *p, big_t *k) {
    for (int i = 0; i < size; i++) {
        uint8_t point[130];
        ecp_t old_shared;
        ecp_t new_shared;
        recv_wait(sock->socket_desc, point, 130, 1, 3);
        hex_to_big((char*)point, &old_shared.X);
        hex_to_big((char*)(point + 65), &old_shared.Z);
        ecp_mul_cst(curve, &old_shared, k, p, &new_shared);
        big_to_str(&new_shared.X, (char*)point);
        big_to_str(&new_shared.Z, (char*)(point + 65));
        send_wait(sock->socket_desc, point, 130, 1, 3);
    }
}

int connect_socket(soq_sec *sock) {
    if (connect(sock->socket_desc, (struct sockaddr *)&sock->channel, sizeof(sock->channel)) < 0) {
        return CONNECT;
    }

    big_t p, np, k;
    big_null(&p); big_null(&np);
    
    memcpy(p.value, P25519, sizeof(dig_t) * 8);
    memcpy(np.value, N25519, sizeof(dig_t) * 8);
    ec_t curve;
    ec_init_c25519(curve);
    
    uint8_t pvk[65];
    uint8_t user_name[20];

    system("head -c 4096 /dev/urandom | sha256sum | cut -b1-55 >> priv.key"); // must be less than curve order (n)
    FILE *fp_key = fopen("priv.key", "r");
    fgets((char*)pvk, 65, fp_key);
    fclose(fp_key);
    system("rm priv.key");

    hex_to_big((char*)pvk, &k);

    std::cout << "insert your nickname : ";
    std::cin >> user_name;

    // send new user nickname
    uint8_t size[1];
    send_wait(sock->socket_desc, user_name, 20, 1, 3);
    recv_wait(sock->socket_desc, size, 1, 1, 3);

    // get partial_shared_key (without user own pvk)
    upd_shared_key(sock, size[0], &curve, &p, &k);

    pid_t child_pid = fork();

    if (child_pid == 0) {
        // set terminal settings
        write_handler(sock, &curve, &p, &np, &k);
    }
    else if (child_pid > 0){
        recv_handler(sock, &curve, &p, &np, &k);
    }

    return OK;
}