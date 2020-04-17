#include "soq_sec.h"
#include <termios.h>
#include <unistd.h>

#define BUFF_SIZE 4096

#include "MeekSpeak-Crypto/bn/bn.h"
#include "MeekSpeak-Crypto/ecc/ecc_25519.h"

void set_cmd_line() {
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

int main(int argc, char *argv[]) {
    
    ecp_t session_key;
    big_t p, np, k;

    uint32_t counter = 1;
    uint32_t nonce[3];
    uint32_t private_key[8];
    big_null(&p); big_null(&np);
    
    memcpy(p.value, P25519, sizeof(dig_t) * 8);
    memcpy(np.value, N25519, sizeof(dig_t) * 8);

    ec_t curve;
    ec_init_c25519(curve);
    nonce[0] = 0x0;
    nonce[1] = 0x4d;
    nonce[2] = 0x0;

    soq_sec sock;
    if (strcmp("server", argv[1]) == 0) {
	    display_error(make_soq_sec(&sock, SERVER, argv[2], atoi(argv[3])));
        
        // get session pbk
        big_rnd_dig(&k);
        ecp_mul_cst(&curve, &curve.G, &k, &p, &session_key);

        // write to server struct 
        big_to_str(&session_key.X, (char*)sock.pbk_str[0]);
        big_to_str(&session_key.Z, (char*)sock.pbk_str[1]);

	    display_error(start_listen(&sock, BUFF_SIZE));
    }
    else if (strcmp("client", argv[1]) == 0) {

    	display_error(make_soq_sec(&sock, CLIENT, argv[2], atoi(argv[3])));
    	display_error(connect_socket(&sock));

        uint8_t str_pbk[2][65];
        int get_x = read_from_server(&sock, str_pbk[0], 65);
        int get_z = read_from_server(&sock, str_pbk[1], 65);
        if (get_x != OK || get_z != OK) {
            display_error(get_x);
            exit(0);
        }

        hex_to_big((char*)str_pbk[0], session_key.X);
        hex_to_big((char*)str_pbk[1], session_key.Z);

        big_rnd_dig(&k);
        for (int i = 0; i < 8; i++) {
            private_key[i] = (uint32_t)k.value[i];
        }

        uint8_t plain_text[BUFF_SIZE];
        uint8_t cipher[BUFF_SIZE];
        uint8_t buffer[BUFF_SIZE];

        pid_t child_pid = fork();

        if (child_pid == 0) {

            uint8_t nick[20];
            fprintf(stdout, "insert your nickname : ");
            fscanf(stdin, "%s", nick);

            uint32_t shared_key[8]; // shared private key between clients (not server), how?
            ecp_t user_pbk;
            uint8_t user_pbk_str[2][65];
            ecp_mul_cst(&curve, &session_key, &k, &p, &user_pbk);

            big_to_str(&user_pbk.X, (char*)user_pbk_str[0]);
            big_to_str(&user_pbk.Z, (char*)user_pbk_str[1]);

            // set terminal settings
            set_cmd_line();

            while (true) {

                memset(buffer, 0, BUFF_SIZE);
                memset(plain_text, 0, BUFF_SIZE);
                int err = fscanf(stdin, "\n%[^\n]", buffer);
                sprintf((char*)plain_text, "%s : %s", nick, buffer);

                // get receiver pbk 
                // end to end crypto (no mac yet)
                // chacha_enc(key_stream, nonce_cut, counter, plain_text, cipher, BUFF_SIZE);

                if (err > 0) {
                    fprintf(stdout, "%s\n", plain_text);
                    display_error(write_to_server(&sock, plain_text, BUFF_SIZE)); // no encode
                    counter++; // not thread safe
                }
            }
        }
        else if (child_pid > 0){
            while (true) {
                memset(cipher, 0, BUFF_SIZE);
                memset(plain_text, 0, BUFF_SIZE);

                if (read_from_server(&sock, plain_text, BUFF_SIZE) == OK) {
                    // decrypt text
                    //chacha_enc(key, nonce, 0, cipher, plain_text, BUFF_SIZE);
                    fprintf(stdout, "%s\n", plain_text);
                }
                else {
                    fprintf(stdout, "disconnected from server\n");
                    break;
                }
            }
            close(sock.socket_desc);
            wait(NULL);
        }
    }
}