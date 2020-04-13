#include "soq_sec.h"
#define BUFF_SIZE 4096
#define MAX_CONN 100

int main(int argc, char *argv[]) {
	// argv1 = type, argv2 = ip, argv3 = port 
    soq_sec sock;
    if (strcmp("server", argv[1]) == 0) {
	    display_error(make_soq_sec(&sock, SERVER, argv[2], atoi(argv[3])));
	    display_error(start_listen(&sock, MAX_CONN, BUFF_SIZE, read_from_client));
    }
    else if (strcmp("client", argv[1]) == 0){
    	display_error(make_soq_sec(&sock, CLIENT, argv[2], atoi(argv[3])));
    	display_error(connect_socket(&sock));
    	char msg[BUFF_SIZE];
    	while (true) {
	    	scanf("\n%[^\n]", msg);
	    	if (strcmp(msg, "exit") == 0) break;
			display_error(write_to_server(&sock, msg));
    	}
		close(sock.socket_desc);
    }
}