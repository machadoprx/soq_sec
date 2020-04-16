#include "soq_sec.h"
#define BUFF_SIZE 4096
#define MAX_CONN 100

#include "MeekSpeak-Crypto/bn/bn.h"
#include "MeekSpeak-Crypto/ecc/ecc_25519.h"

void get_pbk(ec_t *curve, big_t *p, big_t *np, ecp_t *public_key, big_t *k) {
    big_rnd_dig(k);
    
    k->value[7] &= 0x7FFFFFFFull;
    while (big_gth_uns(k, np) > 0) {
        big_rnd_dig(k);
        k->value[7] &= 0x7FFFFFFFull;
    }

    ecp_mul_cst(curve, &curve->G, k, p, public_key);
}

int main(int argc, char *argv[]) {

    //chacha_enc(uint32_t key[], uint32_t nonce[], uint32_t counter, uint8_t *plain, uint8_t *cipher, int len);
    /*big_t p, np, k, nonce, public_key;
    big_null(&p); big_null(&np);
    
    memcpy(p.value, P25519, sizeof(dig_t) * 8);
    memcpy(np.value, N25519, sizeof(dig_t) * 8);

    ec_t curve;
    ec_init_c25519(curve);

    big_rnd_dig(&nonce);*/

    soq_sec sock;
    if (strcmp("server", argv[1]) == 0) {
	    display_error(make_soq_sec(&sock, SERVER, argv[2], atoi(argv[3])));
	    display_error(start_listen(&sock, MAX_CONN, BUFF_SIZE, read_from_client));
    }
    else if (strcmp("client", argv[1]) == 0){
    	display_error(make_soq_sec(&sock, CLIENT, argv[2], atoi(argv[3])));
    	display_error(connect_socket(&sock));
    	while (true) {
            uint8_t msg[BUFF_SIZE];
            uint8_t buffer[BUFF_SIZE];
            if (read_from_server(&sock, buffer) == OK) {
                buffer[100] = '\0';
                fprintf(stdout, "%s", buffer);
            }
	    	fscanf(stdin, "\n%[^\n]", msg);
	    	if (strcmp(msg, "exit") == 0) break;
			display_error(write_to_server(&sock, msg));
    	}
		close(sock.socket_desc);
    }
}