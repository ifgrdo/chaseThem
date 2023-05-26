all: server

server: chase_server/server.c chase_server/server_library.h chase_server/server_library.c header.h
	gcc -Wall -pedantic chase_server/server_library.c chase_server/server.c -g -o server_exec -lncurses -lpthread

clean:
	rm server

