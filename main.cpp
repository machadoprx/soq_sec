#include "socket_common.h"
#include "server.h"
#include "client.h"

int main(int argc, char *argv[]) {
    soq_sec sock;
    if (strcmp("server", argv[1]) == 0) {
        int make_err = make_soq_sec(&sock, SERVER, argv[2], atoi(argv[3]));
        if (make_err != OK){
            display_error(make_err);
            exit(0);
        }
	    display_error(start_listen(&sock, BUFF_SIZE));
    }
    else if (strcmp("client", argv[1]) == 0) {
    	display_error(make_soq_sec(&sock, CLIENT, argv[2], atoi(argv[3])));
    	display_error(connect_socket(&sock));
    }
}