/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "client_library.h"

void *send_function(void *arg);
void *recv_function(void *arg);
void *sleep_function(void *arg);

pthread_mutex_t waiting_mutex = PTHREAD_MUTEX_INITIALIZER;


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


int main(int argc, char **argv){

    if(argc != 3){
        printf(" ./server server_adress server_port\n\n");
        exit(0);
    }

    int SOCK_PORT = atoi(argv[2]);
    char SERVER_ADDR[50];
    strcpy(SERVER_ADDR, argv[1]);


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
  
    //window where balls/bots move
    game_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 4, 0);
    box(game_win, 0 , 0);	
    keypad(game_win, true);

    //window that contains the health of each ball
    message_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE + 4, 0);
    box(message_win, 0 , 0);



    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1){
        perror("socket");
        exit(-1);
    }
    
    //server_addr
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SOCK_PORT);

    if (inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr) < 1){
        printf("no valid address\n");
		exit(-1);
    }

    //connect
    if(connect(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
        perror("connect");
        exit(-1);
    }


    //receives a message from the server indicating if there is available space to play
    message first_message;

    if(recv(sock_fd, &first_message, sizeof(first_message), 0) == -1){
        perror("recv");
        exit(-1);
    }

    //if there is space (type == 0), then a connection message is sent to the server and the send_function and recv_function threads are created
    //otherwise, the client disconnects and the socket is closed 
    if(first_message.type == 0){

        for(int i = 0; i < NUM_BOTS; i++){
            previous_bots[i] = first_message.bots[i];
        }

        wrefresh(ID_win);
        wrefresh(game_win);
        wrefresh(message_win);

        //send connection message
        message connect_message;
        connect_message.type = 0;

        if(send(sock_fd, &connect_message, sizeof(connect_message), 0) == -1){
            perror("send");
            exit(-1);
        }
    
        if(pthread_create(&id_send, NULL, send_function, &sock_fd) != 0){
            perror("thread_creation");
            exit(-1);
        }

        if(pthread_create(&id_recv, NULL, recv_function, &sock_fd) != 0){
            perror("thread_creation");
            exit(-1);
        }

        if(pthread_join(id_recv, NULL) != 0){
            perror("thread_join");
            exit(-1);
        } 
        
        if(pthread_join(id_send, NULL) != 0){
            perror("thread_join");
            exit(-1);
        }
    }

    curs_set(1);
    endwin();
   
    close(sock_fd);

    exit(0);
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

//thread responsible for sending messages to the server
void *send_function(void *arg){

    int sock_fd = *(int *)arg;
    int key;

    while(1){
        
        //reads key 
        key = wgetch(game_win); 

        //if the client was waiting (as a result of an health_0 message) and then clicks on a key, the thread that was sleeping for 10s is cancelled
        if (is_waiting){

            if(pthread_cancel(id_sleep) != 0){
                perror("cancel");
                exit(-1);
            }

            if(pthread_mutex_lock(&waiting_mutex) != 0){
                perror("mutex_lock");
                exit(-1);
            }

            is_waiting = 0;

            if(pthread_mutex_unlock(&waiting_mutex) != 0){
                perror("mutex_unlock");
                exit(-1);
            }
        }


        //if Q is pressed, the client sends a disconnect message to the server (type = 5)
        //the recv_function thread and the send_function threads are cancelled
        if(key == 'Q'){
            message disconnect_message;
            disconnect_message.type = 5;

            if(send(sock_fd, &disconnect_message, sizeof(disconnect_message), 0) == -1){
                perror("send");
                exit(-1);
            }
            
            pthread_cancel(id_send);
            pthread_cancel(id_recv);
        }

        //otherwise, a ball_move message (type = 2) is sent to the server
        //checks if the pressed key is an arrow: if yes, the ball can move, otherwise it cannot 
        else{
            message ball_move_message;
            ball_move_message.type = 2;
            ball_move_message.key = key;

            //checks if the pressed key is one of the four arrows
            if((key == KEY_LEFT || key == KEY_DOWN || key == KEY_UP || key == KEY_RIGHT)){
                ball_move_message.can_move = 1; 
            }

            //if the pressed key is neither an arrow nor Q 
            else{
                ball_move_message.can_move = 0; 
            }
            
        
            if(send(sock_fd, &ball_move_message, sizeof(ball_move_message), 0) == -1){
                perror("send");
                exit(-1);
            }
        }
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

//thread responsible for receiving messages from the server
void *recv_function(void *arg){

    int sock_fd = *(int *)arg;

    while(1){

        //the received_message is a structure message, so it contains the relevant info about the field at each moment: number of prizes and balls, and the information relevant about the prizes, balls and bots
        message received_message; 

        if(recv(sock_fd, &received_message, sizeof(received_message), 0) == -1){
            perror("recv");
            exit(-1);
        }

        //creation of a ball_info array whose size corresponds to the current number of connected clients
        ball_info balls[received_message.number_balls];

        //the entries of this array are assigned in a for loop, receiving a ball_info structure from each connected client
        for(int i = 0; i < received_message.number_balls; i++){

            if(recv(sock_fd, &balls[i], sizeof(ball_info), 0) == -1){
                perror("recv");
                exit(-1);
            }
        }

        //if the received message is an health_0 message, then the client waits and does not receive updates from other clients move during 10 seconds
        //if within those 10 seconds this client does not try to move then it is disconnected and removed from the field
        if(received_message.type == 4){

            if(pthread_mutex_lock(&waiting_mutex) != 0){
                perror("mutex_lock");
                exit(-1);
            }

            is_waiting = 1;

            if(pthread_mutex_unlock(&waiting_mutex) != 0){
                perror("mutex_unlock");
                exit(-1);
            }

            //creation of the sleep_function thread
            if(pthread_create(&id_sleep, NULL, sleep_function, &sock_fd) != 0){
                perror("thread_creation");
                exit(-1);
            }
        }


        //otherwise, the field is updated by drawing the balls and bots in the new positions
        else{
            
            //deletes the balls, bots and prizes from the previous positiions
            for(int i = 0; i < previous_number_balls; i++){
                draw_ball(game_win, &previous_balls[i], false);
                mvwprintw(message_win, i+1, 1, "                   "); 
            }

            for(int i = 0; i < previous_number_prizes; i++){
                draw_prize(game_win, &previous_prizes[i], false);
            }

            for(int i = 0; i < NUM_BOTS; i++){
                draw_bot(game_win, &previous_bots[i], false);
            }


            //draws the balls, bots and prizes accordingly to the new info received
            for(int i = 0; i < received_message.number_balls; i++){
                draw_ball(game_win, &balls[i], true);
                mvwprintw(message_win, i+1, 1, "%c: (%d, %d) %d", balls[i].c, balls[i].x, balls[i].y, balls[i].health);
            }            

            for(int i = 0; i < received_message.number_prizes; i++){
                draw_prize(game_win, &received_message.prizes[i], true);
            }

            for(int i = 0; i < NUM_BOTS; i++){
                draw_bot(game_win, &received_message.bots[i], true);
            }

            wrefresh(message_win);
            wrefresh(game_win);

            if (previous_number_balls == 0){
               
                mvwprintw(ID_win, 2, 1, "client: %c", balls[received_message.number_balls - 1].c);
                wrefresh(ID_win);
            }


            //assign the current field info to the previous variables 
            for(int i = 0; i < received_message.number_prizes; i++){
                previous_prizes[i] = received_message.prizes[i];
            }

            for(int i = 0; i < NUM_BOTS; i++){
                previous_bots[i] = received_message.bots[i];
            }

            free(previous_balls);
            previous_balls = (ball_info *) malloc(sizeof(ball_info) * received_message.number_balls);

            for(int i = 0; i < received_message.number_balls; i++){
                previous_balls[i] = balls[i];
            }

            previous_number_balls = received_message.number_balls;
            previous_number_prizes = received_message.number_prizes;            
        }
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//thread created after receiving a health_0 message 
void *sleep_function(void *arg){

    int sock_fd = *(int *)arg;

    sleep(10);

    //after the 10 seconds, a disconnect message is sent to the server
    message disconnect_message;
    disconnect_message.type = 5;

    if(send(sock_fd, &disconnect_message, sizeof(disconnect_message), 0) == -1){
        perror("send");
        exit(-1);
    }
    
    //cancels the send_function thread and the recv_function thread
    pthread_cancel(id_send);
    pthread_cancel(id_recv);

    return NULL;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/
