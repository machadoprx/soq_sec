#include "socket_common.h"
#include "server.h"
#include "client.h"

int main(int argc, char *argv[]) {
    soqueto sock;
    if (strcmp("server", argv[1]) == 0) {
        if (argc < 4) {
            cout << "You need to specify the arguments ./irc_server [type] [ip address] [port]\n";
            exit(0);
        }
        int make_err = make_soqueto(&sock, SERVER, argv[2], atoi(argv[3]));
        if (make_err != OK){
            cout << "Error creating socket " << make_err << "\n";
            exit(0);
        }
	    int list_err = start_listen(&sock, BUFF_SIZE);
        if (list_err != OK){
            cout << "Error listening " << list_err << "\n";
            exit(0);
        }
    }
    else if (strcmp("client", argv[1]) == 0) {
        if (argc != 4) {
            cout << "You need to specify the arguments ./irc_server [type] [ip address] [port]\n";
            exit(0);
        }
    	make_soqueto(&sock, CLIENT, argv[2], atoi(argv[3]));
        string buf;
        do {
            cout << "Type /connect to start\n";
            cin >> buf;
            if (buf.compare("/connect") == 0) {
                int conn_err = connect_socket(&sock);
                if (conn_err != OK) {
                    cout << "Error connecting " << conn_err << "\n";
                }
                break;
            }
        } while (true);
    }
}