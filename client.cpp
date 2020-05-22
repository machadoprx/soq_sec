#include "client.h"

uint32_t nonce[] = {0x0, 0x4d, 0x0};

void set_cmd_line() {
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

void gen_ephemeral_keys(ec_t *curve, big_t *p, uint8_t *pbk_str, big_t *ps_key, uint32_t *shared_key) {
    memset(pbk_str, 0, 130);
    ecp_t shared_point, pbk;
    big_t k, affine;
    
    big_rnd_dig(&k);
    // big_mul_25519(&m, ps_key, p, &k);
    // mask random bytes for using as key
    k.value[0] &= 0xFFFFFFF8u;
    k.value[7] &= 0x7FFFFFFFu;
    k.value[7] |= 0x40000000u;

    ecp_mul_cst(curve, &curve->G, &k, p, &pbk);
    ecp_mul_cst(curve, &pbk, ps_key, p, &shared_point);
    
    ecp_get_afn(&shared_point, p, &affine);

    uint32_t hash[16];
    for (int i = 0; i < 16; i++)
        hash[i] = affine.value[i];
    chacha_block(hash);
    memcpy(shared_key, hash, sizeof(uint32_t) * 8);

    big_to_str(&pbk.X, (char*)pbk_str);
    big_to_str(&pbk.Z, (char*)(pbk_str + 65));
}

int send_handler(soq_sec *sock, ec_t *curve, big_t *p, big_t *ps_key) {
    //set_cmd_line();
    uint32_t shared_key[8]; 

    uint8_t sender_pbk[130];
    uint8_t plain_text[BUFF_SIZE];
    uint8_t cipher[BUFF_SIZE];

    while (true) {

        memset(plain_text, 0, BUFF_SIZE);
        memset(cipher, 0, BUFF_SIZE);

        usleep(250);
        
        if (fscanf(stdin, "\n%[^\n]", plain_text) > 0) {

            if (plain_text[0] == '/') {
                char cmd_str[15];
                sscanf((char*)plain_text, "%s", cmd_str);
                int cmd = get_command(cmd_str);
                
                if (cmd != NOT_FOUND) {
                    send_wait(sock->socket_desc, plain_text, BUFF_SIZE, 250, 5);
                    usleep(500);
                    if (cmd == QUIT) break;
                } 
                continue;
            }

            gen_ephemeral_keys(curve, p, sender_pbk, ps_key, shared_key);
            chacha_enc(shared_key, nonce, plain_text, cipher, BUFF_SIZE);

            send_wait(sock->socket_desc, cipher, BUFF_SIZE, 250, 5);
            usleep(250);
            send_wait(sock->socket_desc, sender_pbk, 130, 250, 5);
        }
    }
    wait(NULL);
    return OK;
}

void get_shared_secret(ec_t *curve, ecp_t *sender_pbk, big_t *p, big_t *ps_key, uint32_t *shared_key) {
    big_t affine;
    ecp_t shared_point;
    //big_t 
    
    ecp_mul_cst(curve, sender_pbk, ps_key, p, &shared_point);
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

int recv_handler(soq_sec *sock, ec_t *curve, big_t *p, big_t *ps_key) {

    ecp_t sender_pbk;
    
    uint32_t shared_key[8];
    
    uint8_t plain_text[BUFF_SIZE];
    uint8_t cipher[BUFF_SIZE];
    memset(cipher, 0, BUFF_SIZE);

    while (true) {
        usleep(250);
        memset(cipher, 0, BUFF_SIZE);
        if (recv_wait(sock->socket_desc, cipher, BUFF_SIZE, 250, 5) > 0) {
            char check_sender[20];
            sscanf((char*)cipher, "%s %*s", check_sender);
            if (strcmp(check_sender, "server") == 0) {
                cout << cipher << '\n';
                continue;
            }
            usleep(250);
            uint8_t point_str[130];
            recv_wait(sock->socket_desc, point_str, 130, 250, 5);

            str_to_ecp(point_str, &sender_pbk);
            get_shared_secret(curve, &sender_pbk, p, ps_key, shared_key);
            
            uint8_t user[20];
            memset(plain_text, 0, BUFF_SIZE);

            recv_wait(sock->socket_desc, user, 20, 250, 5);
            chacha_enc(shared_key, nonce, cipher, plain_text, BUFF_SIZE);
            cout << user << " : " << plain_text << '\n';
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

int connect_socket(soq_sec *sock) {

    if (connect(sock->socket_desc, (struct sockaddr *)&sock->channel, sizeof(sock->channel)) < 0) {
        return CONNECT;
    }

    big_t p;
    big_null(&p);
    
    memcpy(p.value, P25519, sizeof(dig_t) * 8);
    ec_t curve;
    ec_init_c25519(curve);

    // psk + ephemeral keys -> not repeating nonce
    string psk;
    while (psk.size() < 8 || psk.size() > 16) {
        cout << "Type a shared secret for decrypting and encrypting your messages (only alpha-numeric) : ";
        cin >> psk;
    }
    char get_passwd[100];
    sprintf(get_passwd, "mkpasswd -m sha-512 password -s \"%s\" | sha256sum | cut -b1-64 > passwd", psk.c_str());
    system(get_passwd);
    FILE *fp = fopen("passwd", "r");
    fgets(get_passwd, 65, fp);
    fclose(fp);
    system("rm passwd");
    big_t ps_key;
    hex_to_big(get_passwd, &ps_key);
    ps_key.value[0] &= 0xFFFFFFF8u;
    ps_key.value[7] &= 0x7FFFFFFFu;
    ps_key.value[7] |= 0x40000000u;

    signal(SIGINT, SIG_IGN);
    pid_t child_pid = fork();

    if (child_pid == 0) {
        send_handler(sock, &curve, &p, &ps_key);
    }
    else if (child_pid > 0){
        recv_handler(sock, &curve, &p, &ps_key);
    }

    return OK;
}