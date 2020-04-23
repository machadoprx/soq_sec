#include "client.h"

void set_cmd_line() {
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void get_keys(ec_t *curve, big_t *session, big_t *p, uint8_t *pbk_str, uint32_t *shared_key) {
    memset(pbk_str, 0, 130);
    ecp_t shared_point, pbk;
    big_t k, affine;
    
    big_rnd_dig(&k);
    k.value[7] &= 0xfffffffull; // mask for cut below curve order
    
    ecp_mul_cst(curve, &curve->G, &k, p, &pbk);
    ecp_mul_cst(curve, &pbk, session, p, &shared_point);
    
    ecp_get_afn(&shared_point, p, &affine);

    uint32_t hash[16];
    for (int i = 0; i < 16; i++)
        hash[i] = (uint32_t)affine.value[i];
    chacha_block(hash);
    memcpy(shared_key, hash, sizeof(uint32_t) * 8);

    big_to_str(&pbk.X, (char*)pbk_str);
    big_to_str(&pbk.Z, (char*)(pbk_str + 65));
}

int send_handler(soq_sec *sock, ec_t *curve, big_t *p, big_t *session_key) {
    
    set_cmd_line();
    uint32_t shared_key[8]; 
    uint32_t nonce[3] = {0x0, 0x4d, 0x0};

    uint8_t sender_pbk[130];
    uint8_t plain_text[BUFF_SIZE];
    uint8_t cipher[BUFF_SIZE];
    uint8_t msg[BUFF_SIZE + 130];
    big_to_hex(session_key);

    while (true) {
        memset(msg, 0, BUFF_SIZE + 130);
        memset(plain_text, 0, BUFF_SIZE);
        memset(cipher, 0, BUFF_SIZE);

        if (fscanf(stdin, "\n%[^\n]", plain_text) > 0) {
            get_keys(curve, session_key, p, sender_pbk, shared_key);
            chacha_enc(shared_key, nonce, plain_text, cipher, BUFF_SIZE);
            
            memcpy(msg, sender_pbk, 130);
            memcpy(msg + 130, cipher, BUFF_SIZE);

            send_wait(sock->socket_desc, msg, BUFF_SIZE + 130, 250, 5);
            fprintf(stdout, "%s\n", plain_text);
        }
    }
    return OK;
}

void get_shared_secret(ec_t *curve, ecp_t *sender_pbk, big_t *p, big_t *session_key, uint32_t *shared_key) {
    big_t affine;
    ecp_t shared_point;
    
    ecp_mul_cst(curve, sender_pbk, session_key, p, &shared_point);
    ecp_get_afn(&shared_point, p, &affine);

    uint32_t hash[16];
    for (int i = 0; i < 16; i++)
        hash[i] = (uint32_t)affine.value[i];
    chacha_block(hash);
    memcpy(shared_key, hash, sizeof(uint32_t) * 8);
}

void str_to_ecp(uint8_t str[130], ecp_t *point) {
    ecp_null(point);
    uint8_t X[65];
    uint8_t Z[65];
    memcpy(X, str, 65);
    memcpy(Z, str + 65, 65);
    hex_to_big((char*)X, &point->X);
    hex_to_big((char*)Z, &point->Z);
}

int recv_handler(soq_sec *sock, ec_t *curve, big_t *p, big_t *session_key) {

    ecp_t sender_pbk;
    
    uint32_t shared_key[8];
    uint32_t nonce[3] = {0x0, 0x4d, 0x0};
    
    uint8_t plain_text[BUFF_SIZE];
    uint8_t msg[BUFF_SIZE + 130];

    while (true) {

        memset(msg, 0, BUFF_SIZE + 130);
        memset(plain_text, 0, BUFF_SIZE);

        sleep(1);
        if (recv_wait(sock->socket_desc, msg, BUFF_SIZE + 130, 250, 5) > 0) {
            uint8_t point_str[130];
            memcpy(point_str, msg, 130);
            str_to_ecp(point_str, &sender_pbk);
            get_shared_secret(curve, &sender_pbk, p, session_key, shared_key);

            chacha_enc(shared_key, nonce, msg + 130, plain_text, BUFF_SIZE);
            fprintf(stdout, "%s\n", plain_text);
        }
        else {
            fprintf(stdout, "disconnected from server\n"); 
            break;
        }
    }
    close(sock->socket_desc);
    wait(NULL);
    return OK;
}

int connect_socket(soq_sec *sock) {
    if (connect(sock->socket_desc, (struct sockaddr *)&sock->channel, sizeof(sock->channel)) < 0) {
        return CONNECT;
    }

    big_t p;
    big_null(&p);
    
    memcpy(p.value, P25519, sizeof(dig_t) * 8);
    ec_t curve;
    ec_init_c25519(curve);

    uint8_t user_name[20];
    uint8_t session_key[65];
    memset(user_name, 0, 20);
    memset(session_key, 0, 65);

    fprintf(stdout, "insert user name: ");
    fscanf(stdin, "%s", (char*)user_name);
    fflush(NULL);

    // send new user nickname
    send_wait(sock->socket_desc, user_name, 20, 500, 5);
    recv_wait(sock->socket_desc, session_key, 65, 500, 5);
    big_t session;
    hex_to_big((char*)session_key, &session);

    pid_t child_pid = fork();

    if (child_pid == 0) {
        send_handler(sock, &curve, &p, &session);
    }
    else if (child_pid > 0){
        recv_handler(sock, &curve, &p, &session);
    }

    return OK;
}