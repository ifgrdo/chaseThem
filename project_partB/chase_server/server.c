/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "server_library.h"

void *prizes_function(void *arg);
void *bots_function(void *arg);
void *client_function(void *arg);

pthread_mutex_t balls_mutex = PTHREAD_MUTEX_INITIALIZER;


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


int main(int argc, char **argv){

    if(argc != 3){
        printf(" ./server server_adress server_port\n\n");
        exit(0);
    }

    int SOCK_PORT = atoi(argv[2]);
    char SERVER_ADDR[50];
    strcpy(SERVER_ADDR, argv[1]);

    srand(time(0));

    //socket creation
    int sock_fd, new_sock_fd;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1){
        perror("socket: ");
        exit(-1);
    }

    //server_addr
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SOCK_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    //binding
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("bind");
        exit(-1);
    }

    //listening
    if (listen(sock_fd, 10) == -1){
        perror("listen");
        exit(-1);
    }

    //startes curses mode
    initscr();			
    cbreak();				
    keypad(stdscr, TRUE);		
    noecho();
    refresh();
    curs_set(0);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    signal (SIGWINCH, NULL);

    //window that contains the sun_path of the client and the name of the character controlled by him
    ID_win = newwin(4, WINDOW_SIZE, 0, 0);
    box(ID_win, 0 , 0);	
    wrefresh(ID_win);	

    //window where balls/bots move
    game_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 4, 0);
    box(game_win, 0 , 0);	
    keypad(game_win, true);
    wrefresh(game_win);

    //window that contains the health of each ball
    message_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE + 4, 0);
    box(message_win, 0 , 0);
    wrefresh(message_win);

    //client_addr
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    
    //add the 5 initial prizes
    for (int i = 0; i < 5; i++){
        add_prize();
    }

    //add bots to the field
    add_bots();

    message rand;
    update(&rand);

    draw();
 
    //launch the prizes thread and the bots thread
    pthread_t prizes_id, bots_id;

    if(pthread_create(&prizes_id, NULL, prizes_function, NULL) != 0){
        perror("thread_creation");
        exit(-1);
    }

    if(pthread_create(&bots_id, NULL, bots_function, NULL) != 0){
        perror("thread_creation");
        exit(-1);
    }


    //while loop that waits for clients connections
    while(1){
        
        client_addr_size = sizeof(struct sockaddr_in);
        if ((new_sock_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &client_addr_size)) == -1){
            perror("accept");
            exit(-1);
        }

        //checks if the number of connected clients has already achieved the maximum value (26, which corresponds to the alphabet A...Z)
        if (number_balls < MAX_CLIENTS){

            int *info = malloc(sizeof(int));
            *info = new_sock_fd;

            //create one thread for each new client that connects to the server
            pthread_t thread_id;

            if(pthread_create(&thread_id, NULL, client_function, info) != 0){
                perror("thread_creation");
                exit(-1);
            }       

            //sends message to the client indicating that there is free space so the client can connect
            message can_connect;
            update(&can_connect);
            can_connect.type = 0;  

            if(send(new_sock_fd, &can_connect, sizeof(message), 0) == -1){
                perror("send");
                exit(-1);
            }
        }

        else{
            //no free space, so the client can't connect
            message cannot_connect;
            cannot_connect.type = -1;

            if(send(new_sock_fd, &cannot_connect, sizeof(message), 0) == -1){
                perror("send");
                exit(-1);
            }
            
            close(new_sock_fd);
        }
    }


    curs_set(1);
    endwin();
    
    //close(sock_fd);

    exit(0);
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void *prizes_function(void *arg){

    //message that updates the field of the game after adding a prize
    struct message message;
    message.type = 3;

    while(1){

        if(pthread_mutex_lock(&prizes_mutex) != 0){
            perror("mutex_lock");
            exit(-1);
        }
        
        //if the maximum number of prizes is reached, then the thread sleeps until receiving a signal from the function remove_prize(): this signal indicates that a prize was eaten so there is need to add a new one
        if(number_prizes == MAX_PRIZES){
            if (pthread_cond_wait(&prizes_cond_variable, &prizes_mutex) != 0){
                perror("cond_wait");
                exit(-1);
            }
        }
       
        if(pthread_mutex_unlock(&prizes_mutex) != 0){
            perror("mutex_unlock");
            exit(-1);
        }

        sleep(5);
        add_prize();

        draw();

        //sends message to all clients indicating the change in the field
        update(&message);
        send_messages(message, "all", -1);
    }
    
    return NULL;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void *bots_function(void *arg){

    //message that updates the field of the game after moving a bot
    struct message message;
    message.type = 3;

    int rand, key[NUM_BOTS], bot_directions[NUM_BOTS];

    while(1){

        sleep(3);

        for(int i = 0; i < NUM_BOTS; i++){

            //random direction to each bot
            rand = random() % 4;

            switch(rand){
                case 0:
                    key[i] = KEY_UP;
                    break;
                case 1:
                    key[i] = KEY_RIGHT;
                    break;
                case 2:
                    key[i] = KEY_DOWN;
                    break;
                case 3:
                    key[i] = KEY_LEFT;
                    break;
            }

            bot_directions[i] = key[i];
        }

        move_bot(bot_directions);

        draw();

        //sends message to all clients indicating the change in the field
        update(&message);
        send_messages(message, "all", -1);
    }

    return NULL;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void *client_function(void *arg){
    
    int sock_fd = *(int *)arg;
    int ball_name;

    struct message message;

    //while loops that handles interactions with the clients
    while(1){

        if(recv(sock_fd, &message, sizeof(message), 0) == -1){
            perror("recv");
            exit(-1);
        }

        //gets the current ball/sock in the respetives linked lists
        ball *current_ball = balls;
        sock *current_sock = socks;
        
        for(int i = 0; i < get_index(sock_fd); i++){
            current_ball = current_ball->next;
            current_sock = current_sock->next;
        }

        
        //checks if the current client socket is waiting due to an health_0 message received previously
        //if so, that client ressurects because he has clicked on a key during the waiting interval
        if(socks != NULL){

            if(current_sock->status == 0){

                current_ball->info.health = 10;
                current_sock->status = 1;
            }
        }

        //connection message received
        //adds ball to the field
        if (message.type == 0){

            ball_name = new_name();
            
            if(pthread_mutex_lock(&balls_mutex) != 0){
                perror("mutex_lock");
                exit(-1);
            }

            add_ball(ball_name, sock_fd);

            if(pthread_mutex_unlock(&balls_mutex) != 0){
                perror("mutex_unlock");
                exit(-1);
            }
            
            message.type = 1;
        }


        if(get_index(sock_fd) == -1){
            
            pthread_cancel(pthread_self());
        }


        else{

            //every client is informed after a client has connected and the field of each one of them is updated
            if (message.type == 1){
                draw();

                update(&message);
                send_messages(message, "all", -1);
            }


            //ball_move message received
            //a field_status message will be sent to everyone after setting message.type = 3
            if (message.type == 2){
            
                if(message.can_move == 1){ 

                    move_ball(message.key, get_index(sock_fd));
                    message.type = 3; 
                }
            }


            //field_status message
            //after the client has moved, all the other clients will be informed and receive a status field message, except the ones who are waiting after an health_0 message
            //checks if the health of the client changed to zero as a result of the current movement 
            if (message.type == 3){
                
                draw();

                update(&message);
                send_messages(message, "all", -1);

                if(current_ball->info.health == 0){
                    message.type = 4;
                }
            }


            //health_0 message
            //the current client enters in a waiting mode, so it will not receive messages from other clients during that period
            //if after entering in this mode the client tries to move then it will exit the waiting mode and goes back to the normal one
            //a health_0 message is sent only to this client
            if (message.type == 4){

                current_sock->status = 0;
                send_messages(message, "one", sock_fd);
            }


            //disconnect message
            //removes ball from field
            //sends message to all clients informing of this disconnection and updates their fields accordingly
            if (message.type == 5){

                if(pthread_mutex_lock(&balls_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }

                remove_ball(get_index(sock_fd));

                if(pthread_mutex_unlock(&balls_mutex) != 0){
                    perror("mutex_unlock");
                    exit(-1);
                }

                draw();

                update(&message);
                send_messages(message, "all", -1);

                pthread_cancel(pthread_self());
            }
        }   
    }   

    return NULL;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

