all: client
	
client: chase_client/client.c chase_client/client_library.h chase_client/client_library.c header.h
	gcc -Wall -pedantic chase_client/client_library.c chase_client/client.c -g -o client_exec -lncurses -lpthread

clean:
	rm client
