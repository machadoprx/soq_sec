all:
	g++ -Wall main.cpp socket_common.cpp client.cpp server.cpp -o irc_server