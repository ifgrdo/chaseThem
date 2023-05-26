/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "server_library.h"

int number_balls = 0, previous_number_balls = 0; //variables with the current number of balls and the maximum one allowed
int number_prizes = 0, previous_number_prizes = 0; //current number of prizes and the maximum allowed 

prize prizes[MAX_PRIZES]; 
bot bots[NUM_BOTS]; 
sock *socks = NULL;
ball *balls = NULL;

ball *previous_balls; 
prize previous_prizes[MAX_PRIZES]; 
bot previous_bots[NUM_BOTS];

WINDOW *ID_win, *game_win, *message_win;

pthread_cond_t prizes_cond_variable = PTHREAD_COND_INITIALIZER;
pthread_mutex_t prizes_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t update_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t health_mutex = PTHREAD_MUTEX_INITIALIZER;


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//creates a new name
char new_name(){

    while(1){

        int random = (rand() % (90 - 65 + 1)) + 65; //65 corresponds to A and 90 to Z
        int is_repeated = check_repeated_names(random);
        if(is_repeated == 0){ //if that character does not exist, the function returns it
            char c = (char) random;
            return c;
        }
    } 
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//checks if 'name' already exists 
int check_repeated_names(char name){

    int is_repeated = 0;
    ball *current_ball = balls;

    for (int i = 0; i < number_balls; i++){

        if(current_ball->info.c == name){
            is_repeated = 1;
            break;
        }

        current_ball = current_ball->next;
    }
    
    return is_repeated;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//creates a new ball when a connection message is received if the number of current balls is not higher than the maximum allowed
void add_ball(char name, int sock_fd){

    //adds ball to the list of balls
    ball *new_ball = (ball *) malloc(sizeof(ball));

    ball_info info = {
        .x = WINDOW_SIZE/2,
        .y = WINDOW_SIZE/2,
        .c = name,
        .health = 10
    };

    new_ball->info = info;
    new_ball->next = NULL;

    if(balls == NULL){
        balls = new_ball;
    }

    else{
        ball *last_ball = balls;

        while(last_ball->next != NULL){
            last_ball = last_ball->next;
        }

        last_ball->next = new_ball;
    }

    //adds socket to the list of sockets 
    sock *new_sock = (sock *) malloc(sizeof(sock));

    new_sock->value = sock_fd;
    new_sock->status = 1;
    new_sock->next = NULL;

    if(socks == NULL){
        socks = new_sock;
    }

    else{
        sock *last_sock = socks;

        while(last_sock->next != NULL){
            last_sock = last_sock->next;
        }

        last_sock->next = new_sock;
    }

    ++ number_balls;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//removes ball when a disconnect or health_0 message is received
void remove_ball (int index){

    ball *aux_ball;
    sock *aux_sock;

    //first ball of the linked list
    if(index == 0){

        aux_ball = balls;
        aux_sock = socks;
        balls = balls->next;
        socks = socks->next;

        free(aux_ball);
        free(aux_sock);
    }

    //last ball of the linked list
    else if(index == number_balls - 1){

        ball *current_ball = balls;

        while(current_ball->next->next != NULL){
            current_ball = current_ball->next;
        }
        aux_ball = current_ball->next;
        current_ball->next = NULL;
        free(aux_ball);


        sock *current_sock = socks;

        while(current_sock->next->next != NULL){
            current_sock = current_sock->next;
        }
        aux_sock = current_sock->next;
        current_sock->next = NULL;
        free(aux_sock);
    }

    //intermediate ball of the linked list
    else{

        ball *current_ball = balls;
        sock *current_sock = socks;


        for(int i = 0; i < index - 1; i++){
            current_ball = current_ball->next;
            current_sock = current_sock->next;
        }
        aux_ball = current_ball->next;
        aux_sock = current_sock->next;
        current_ball->next = current_ball->next->next;
        current_sock->next = current_sock->next->next;
        free(aux_ball);
        free(aux_sock);
    }

   
    -- number_balls; //decreases the number of ball;
}



/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//moving balls
void move_ball(int key, int index){   

    ball *moving_ball = balls;
    for (int i = 0; i < index; i++){
        moving_ball = moving_ball->next;
    }
    
    if(key == KEY_UP){
        if(moving_ball->info.y != 1){
            moving_info ball_info = has_ball(moving_ball->info.x, moving_ball->info.y - 1);
            moving_info prize_info = has_prize(moving_ball->info.x, moving_ball->info.y - 1);
            moving_info bot_info = has_bot(moving_ball->info.x, moving_ball->info.y - 1);
            
            //if there is not a bot or a ball in the above cell, the bot moves to there
            if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0){
                -- moving_ball->info.y;
                
                //if there is a prize in that cell, it is eaten and the health of the ball increases 
                if(prize_info.is_occupied == 1){
                    moving_ball->info.health += (prize_info.prize_value - 48);
                    
                    //guarantees that the health can't be higher than 10
                    if(moving_ball->info.health > 10){
                        moving_ball->info.health = 10;
                    }

                    //removes the eaten prize from the array of prizes
                    remove_prize(prize_info.index_prize);
                }
            }
            // if there is a ball in that cell, the health of the rammed ball increases and the health of the ball that tried to move decreases
            else if(ball_info.is_occupied == 1){

                ball *rammed_ball = balls;
                for (int i = 0; i < ball_info.index_rammed_ball; i++){
                    rammed_ball = rammed_ball->next;
                }

                if(pthread_mutex_lock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
                    
                if(rammed_ball->info.health < 10 && rammed_ball->info.health > 0){
                    ++ rammed_ball->info.health;
                }
                if(moving_ball->info.health > 0 && rammed_ball->info.health > 0){
                    -- moving_ball->info.health;
                }

                if(pthread_mutex_unlock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
            }
        }
    }

    if(key == KEY_DOWN){
        if(moving_ball->info.y != WINDOW_SIZE - 2){
            moving_info ball_info = has_ball(moving_ball->info.x, moving_ball->info.y + 1);
            moving_info prize_info = has_prize(moving_ball->info.x, moving_ball->info.y + 1);
            moving_info bot_info = has_bot(moving_ball->info.x, moving_ball->info.y + 1);

            //if there is not a bot or a ball in the below cell, the bot moves to there
            if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0){
                ++ moving_ball->info.y;
                
                //if there is a prize in that cell, it is eaten and the health of the ball increases 
                if(prize_info.is_occupied == 1){
                    moving_ball->info.health += (prize_info.prize_value - 48);

                    //guarantees that the health can't be higher than 10
                    if(moving_ball->info.health > 10){
                        moving_ball->info.health = 10;
                    }

                    //removes the eaten prize from the array of prizes
                    remove_prize(prize_info.index_prize);
                }
            }
            
            // if there is a ball in that cell, the health of the rammed ball increases and the health of the ball that tried to move decreases
            else if(ball_info.is_occupied == 1){

                ball *rammed_ball = balls;
                for (int i = 0; i < ball_info.index_rammed_ball; i++){
                    rammed_ball = rammed_ball->next;
                }

                if(pthread_mutex_lock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
                    
                if(rammed_ball->info.health < 10 && rammed_ball->info.health > 0){
                    ++ rammed_ball->info.health;
                }
                if(moving_ball->info.health > 0 && rammed_ball->info.health > 0){
                    -- moving_ball->info.health;
                }

                if(pthread_mutex_unlock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
            }
        }
    }

    if(key == KEY_LEFT){
        if(moving_ball->info.x != 1){
            moving_info ball_info = has_ball(moving_ball->info.x - 1, moving_ball->info.y);
            moving_info prize_info = has_prize(moving_ball->info.x - 1, moving_ball->info.y);
            moving_info bot_info = has_bot(moving_ball->info.x - 1, moving_ball->info.y);

            //if there is not a bot or a ball in the left cell, the bot moves to there
            if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0){
                -- moving_ball->info.x;

                //if there is a prize in that cell, it is eaten and the health of the ball increases 
                if(prize_info.is_occupied == 1){
                    moving_ball->info.health += (prize_info.prize_value - 48);

                    //guarantees that the health can't be higher than 10
                    if(moving_ball->info.health > 10){
                        moving_ball->info.health = 10;
                    }

                    //removes the eaten prize from the array of prizes
                    remove_prize(prize_info.index_prize);
                }
            }
            
            // if there is a ball in that cell, the health of the rammed ball increases and the health of the ball that tried to move decreases
            else if(ball_info.is_occupied == 1){

                ball *rammed_ball = balls;
                for (int i = 0; i < ball_info.index_rammed_ball; i++){
                    rammed_ball = rammed_ball->next;
                }

                if(pthread_mutex_lock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
                    
                if(rammed_ball->info.health < 10 && rammed_ball->info.health > 0){
                    ++ rammed_ball->info.health;
                }
                if(moving_ball->info.health > 0 && rammed_ball->info.health > 0){
                    -- moving_ball->info.health;
                }

                if(pthread_mutex_unlock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
            }
        }
    }

    if(key == KEY_RIGHT){
        if(moving_ball->info.x != WINDOW_SIZE - 2){
            moving_info ball_info = has_ball(moving_ball->info.x + 1, moving_ball->info.y);
            moving_info prize_info = has_prize(moving_ball->info.x + 1, moving_ball->info.y);
            moving_info bot_info = has_bot(moving_ball->info.x + 1, moving_ball->info.y);

            //if there is not a bot or a ball in the right cell, the bot moves to there
            if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0){
                ++ moving_ball->info.x;

                //if there is a prize in that cell, it is eaten and the health of the ball increases 
                if(prize_info.is_occupied == 1){
                    moving_ball->info.health += (prize_info.prize_value - 48);

                    //guarantees that the health can't be higher than 10
                    if(moving_ball->info.health > 10){
                        moving_ball->info.health = 10;
                    }

                    //removes the eaten prize from the array of prizes
                    remove_prize(prize_info.index_prize);
                }
            }
            
            // if there is a ball in that cell, the health of the rammed ball increases and the health of the ball that tried to move decreases           
            else if(ball_info.is_occupied == 1){

                ball *rammed_ball = balls;
                for (int i = 0; i < ball_info.index_rammed_ball; i++){
                    rammed_ball = rammed_ball->next;
                }

                if(pthread_mutex_lock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
                    
                if(rammed_ball->info.health < 10 && rammed_ball->info.health > 0){
                    ++ rammed_ball->info.health;
                }
                if(moving_ball->info.health > 0 && rammed_ball->info.health > 0){
                    -- moving_ball->info.health;
                }

                if(pthread_mutex_unlock(&health_mutex) != 0){
                    perror("mutex_lock");
                    exit(-1);
                }
            }
        }
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

//copies the list balls to another list
ball *copy(ball *balls){

    if (balls == NULL) {
        return NULL;
    }

    else {
  
        ball* new_ball = (ball *) malloc(sizeof(ball));

        ball_info info = {
        .x = balls->info.x,
        .y = balls->info.y,
        .c = balls->info.c,
        .health = balls->info.health
        };

        new_ball->info = info;
        new_ball->next = copy(balls->next);

        return new_ball;
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//creates a prize whose value is between 1 and 5
prize random_prize(){
    prize p;
    int inf = 1;
    int sup = WINDOW_SIZE - 2;
    p.x = (rand() % (sup-inf+1)) + inf;
    p.y = (rand() % (sup-inf+1)) + inf;

    inf = 49; //ASCII value of 1
    sup = 53; //ASCII value of 5
    p.value = (rand() % (sup-inf+1)) + inf;

    return p;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//adding new prize
void add_prize(){
    prize p;
    int exists_bot = 1, exists_prize = 1, exists_ball = 1;

    //checks if the number of prizes is not higher than 10
    if(number_prizes < 10){
        
        //while the created prize randomly has coordinates that coincide with the coordinates of another prize, bot or ball we have to create another prize
        while(exists_ball == 1 || exists_prize == 1 || exists_bot == 1){
            p = random_prize();
        
            exists_ball = has_ball(p.x, p.y).is_occupied;
            exists_prize = has_prize(p.x, p.y).is_occupied;
            exists_bot = has_bot(p.x, p.y).is_occupied;

            if(p.x == WINDOW_SIZE/2 && p.y == WINDOW_SIZE/2){
                exists_ball = 1;
            }
        }

        //assign the new info to the array of prizes
        prizes[number_prizes].x = p.x;
        prizes[number_prizes].y = p.y;
        prizes[number_prizes].value = p.value;
        ++ number_prizes;
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//removes a prize from the array of prizes
//when a prize is removed, a signal indicating that the prizes_function thread can wake up in order to add new prizes to the field until reaching the maximum of 10 
void remove_prize(int index){

    if(pthread_mutex_lock(&prizes_mutex) != 0){
        perror("mutex_lock");
        exit(-1);
    }
    
    //'index' corresponds to the index of the prize that is going to be removed
    for(int i = index; i < number_prizes; i++){
        prizes[i].x = prizes[i+1].x;
        prizes[i].y = prizes[i+1].y;
        prizes[i].value = prizes[i+1].value;
    }

    -- number_prizes;

    if(pthread_cond_signal(&prizes_cond_variable) != 0){
        perror("cond_signal");
        exit(-1);
    }

    if(pthread_mutex_unlock(&prizes_mutex) != 0){
        perror("mutex_unlock");
        exit(-1);
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//creates a bot
bot random_bot(){
    bot b;
    int inf = 1;
    int sup = WINDOW_SIZE - 2;
    
    b.x = (rand() % (sup - inf + 1)) + inf;
    b.y = (rand() % (sup - inf + 1)) + inf;

    return b;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//adding at once all bots required
void add_bots(){

    bot b[NUM_BOTS];
    int exists_ball , exists_prize, exists_bot;

    for (int i = 0; i < NUM_BOTS; i++){
        
        exists_ball = 1, exists_prize = 1, exists_bot = 1;

        //while the created bot randomly has coordinates that coincide with the coordinates of another prize, bot or ball we have to create another bot
        while(exists_bot == 1 || exists_ball == 1 || exists_prize == 1){
            b[i] = random_bot();

            exists_prize = has_prize(b[i].x, b[i].y).is_occupied;
            exists_bot = has_bot(b[i].x, b[i].y).is_occupied;
            exists_ball = has_ball(b[i].x, b[i].y).is_occupied;

            if(bots[i].x == WINDOW_SIZE/2 && bots[i].y == WINDOW_SIZE/2){
                exists_ball = 1;
            }
        }

        bots[i].x = b[i].x;
        bots[i].y = b[i].y;

    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//moving bots
void move_bot(int *bot_directions){    
    
    int key;

    //all the bots move at the same time
    for(int i = 0; i < NUM_BOTS; i++){

        bot moving_bot = bots[i];
        key = bot_directions[i]; //the move direction of the i-th both
        
        if(key == KEY_UP){
            if(moving_bot.y != 1){
                moving_info ball_info = has_ball(moving_bot.x, moving_bot.y - 1);
                moving_info prize_info = has_prize(moving_bot.x, moving_bot.y - 1);
                moving_info bot_info = has_bot(moving_bot.x, moving_bot.y - 1);
                
                //if there is not a bot, a prize or a ball in the above cell, the bot moves to there
                //note: the bots do not eat prizes
                if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0 && prize_info.is_occupied == 0){
                    -- moving_bot.y;
                }
                //otherwise, if there is a ball in the cell above, the health of that ball decreases
                //if that values reaches 0, the ball disappears
                else if(ball_info.is_occupied == 1){

                    ball *rammed_ball = balls;
                    for (int i = 0; i < ball_info.index_rammed_ball; i++){
                        rammed_ball = rammed_ball->next;
                    }

                    if(pthread_mutex_lock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }

                    if(rammed_ball->info.health > 0){
                        -- rammed_ball->info.health;
                    }

                    if(pthread_mutex_unlock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }
                }
            }
        }

        if(key == KEY_DOWN){
            if(moving_bot.y != WINDOW_SIZE - 2){
                moving_info ball_info = has_ball(moving_bot.x, moving_bot.y + 1);
                moving_info prize_info = has_prize(moving_bot.x, moving_bot.y + 1);
                moving_info bot_info = has_bot(moving_bot.x, moving_bot.y + 1);

                //if there is not a bot, a prize or a ball in the above below, the bot moves to there
                //note: the bots do not eat prizes
                if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0 && prize_info.is_occupied == 0){
                    ++ moving_bot.y;
                }
                //otherwise, if there is a ball in the cell below, the health of that ball decreases
                else if(ball_info.is_occupied == 1){
                    
                    ball *rammed_ball = balls;
                    for (int i = 0; i < ball_info.index_rammed_ball; i++){
                        rammed_ball = rammed_ball->next;
                    }

                    if(pthread_mutex_lock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }

                    if(rammed_ball->info.health > 0){
                        -- rammed_ball->info.health;
                    }

                    if(pthread_mutex_unlock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }
                }
            }
        }

        if(key == KEY_LEFT){
            if(moving_bot.x != 1){
                moving_info ball_info = has_ball(moving_bot.x - 1, moving_bot.y);
                moving_info prize_info = has_prize( moving_bot.x - 1, moving_bot.y);
                moving_info bot_info = has_bot(moving_bot.x - 1, moving_bot.y);

                //if there is not a bot, a prize or a ball in the left cell, the bot moves to there
                //note: the bots do not eat prizes
                if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0 && prize_info.is_occupied == 0){
                    -- moving_bot.x;
                }
                //otherwise, if there is a ball in the left cell, the health of that ball decreases
                else if(ball_info.is_occupied == 1){
                    
                    ball *rammed_ball = balls;
                    for (int i = 0; i < ball_info.index_rammed_ball; i++){
                        rammed_ball = rammed_ball->next;
                    }

                    if(pthread_mutex_lock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }
                    
                    if(rammed_ball->info.health > 0){
                        -- rammed_ball->info.health;
                    }

                    if(pthread_mutex_unlock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }
                }
            }
        }

        if(key == KEY_RIGHT){
            if(moving_bot.x != WINDOW_SIZE - 2){
                moving_info ball_info = has_ball(moving_bot.x + 1, moving_bot.y);
                moving_info prize_info = has_prize(moving_bot.x + 1, moving_bot.y);
                moving_info bot_info = has_bot(moving_bot.x + 1, moving_bot.y);

                //if there is not a bot, a prize or a ball in the right cell, the bot moves to there
                //note: the bots do not eat prizes
                if(ball_info.is_occupied == 0 && bot_info.is_occupied == 0 && prize_info.is_occupied == 0){
                    ++ moving_bot.x;
                }
                //otherwise, if there is a ball in the right cell, the health of that ball decreases
                else if(ball_info.is_occupied == 1){
                    
                    ball *rammed_ball = balls;
                    for (int i = 0; i < ball_info.index_rammed_ball; i++){
                        rammed_ball = rammed_ball->next;
                    }

                    if(pthread_mutex_lock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }
                    
                    if(rammed_ball->info.health > 0){
                        -- rammed_ball->info.health;
                    }

                    if(pthread_mutex_unlock(&health_mutex) != 0){
                        perror("mutex_lock");
                        exit(-1);
                    }
                }
            }
        }
        bots[i] = moving_bot;
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//get index of a socket in the socks list 
int get_index(int sock_fd){

    int index = 0;

    sock *current_sock = socks;

    while(current_sock != NULL){
        if(current_sock->value == sock_fd){
            break;
        }

        ++index;
        current_sock = current_sock->next;
    }

    if (index == number_balls){
        index = -1;
    }

    return index;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void update(message *m){

    //updates the message m with the current info regarding prizes and bots
    for(int i = 0; i < number_prizes; i++){
        m->prizes[i] = prizes[i];
    }

    for(int i = 0; i < NUM_BOTS; i++){
       m->bots[i] = bots[i];
    }

    m->number_balls = number_balls;
    m->number_prizes = number_prizes;


    //the current field info is assigned to the previous field variables
    if(pthread_mutex_lock(&update_mutex) != 0){
        perror("mutex_lock");
        exit(-1);
    }

    for(int i = 0; i < number_prizes; i++){
        previous_prizes[i] = prizes[i];
    }

    for(int i = 0; i < NUM_BOTS; i++){
        previous_bots[i] = bots[i];
    }

    previous_balls = copy(balls);

    previous_number_balls = number_balls;
    previous_number_prizes = number_prizes;

    if(pthread_mutex_unlock(&update_mutex) != 0){
        perror("mutex_unlock");
        exit(-1);
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

//if the option is "all", sends the message m and the info regarding each one of the balls to all sockets (except those who are in the waiting mode) 
//if the option is "one", sends the message m to only the socket sock_dest
void send_messages(message m, char *option, int sock_dest){

    if(strcmp(option, "all") == 0){

        sock *current_sock = socks;

        for(int i = 0; i < number_balls; i++){

            ball *current_ball = balls;

            if(current_sock->status == 1){
                if(send(current_sock->value, &m, sizeof(m), 0) == -1){
                    perror("send");
                    exit(-1);
                }
                    
                for(int j = 0; j < number_balls; j++){
                    if(send(current_sock->value, &current_ball->info, sizeof(ball_info), 0) == -1){
                        perror("send");
                        exit(-1);
                    }
                    current_ball = current_ball->next;
                }
            }

            current_sock = current_sock->next;
        }
    }


    else if(strcmp(option, "one") == 0){

        if(send(sock_dest, &m, sizeof(m), 0) == -1){
            perror("send");
            exit(-1);
        }

        ball *current_ball = balls;
        for(int j = 0; j < number_balls; j++){
            if(send(sock_dest, &current_ball->info, sizeof(ball_info), 0) == -1){
                perror("send");
                exit(-1);
            }
            current_ball = current_ball->next;
        }
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//indicates if the the place to which a ball/bot is trying to move has a ball
moving_info has_ball(int x, int y){

    moving_info has_ball;
    has_ball.is_occupied = 0;
    
    ball *current_ball = balls;
    for(int i = 0; i < number_balls; i++){
        
        if(current_ball->info.x == x && current_ball->info.y == y){
            has_ball.is_occupied = 1;
            has_ball.index_rammed_ball = i;
        }
        current_ball = current_ball->next;
    }
    return has_ball;
}



/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//indicates if the the place to which a ball/bot is trying to move has a prize
moving_info has_prize(int x, int y){

    moving_info has_prize;
    has_prize.is_occupied = 0;

    for(int i = 0; i < number_prizes; i++){

        if(prizes[i].x == x && prizes[i].y == y){
            has_prize.is_occupied = 1;
            has_prize.prize_value = prizes[i].value;
            has_prize.index_prize = i;
        }
    }
    return has_prize;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


//indicates if the the place to which a ball/bot is trying to move has a ball
moving_info has_bot(int x, int y){

    moving_info has_bot;
    has_bot.is_occupied = 0;

    for(int i = 0; i < NUM_BOTS; i++){

        if(bots[i].x == x && bots[i].y == y){
            has_bot.is_occupied = 1;
        }
    }
    return has_bot;
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

void draw_ball(WINDOW *win, ball_info *b, int delete){
    int ch;
    if(delete){
        ch = b->c;
    }else{
        ch = ' ';
    }
    int p_x = b->x;
    int p_y = b->y;;
    wmove(win, p_y, p_x);
    waddch(win, ch);
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void draw_prize(WINDOW *win, prize *prize, int delete){
    int value;
    if(delete){
        value = prize->value;
    }else{
        value = ' ';
    }
    int p_x = prize->x;
    int p_y = prize->y;
    wmove(win, p_y, p_x);
    waddch(win, value);
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void draw_bot(WINDOW *win, bot *bot, int delete){
    int ch;
    if(delete){
        ch = '*';
    }else{
        ch = ' ';
    }
    int p_x = bot->x;
    int p_y = bot->y;
    wmove(win, p_y, p_x);
    waddch(win, ch);
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


void draw(){

    ball *current_ball = previous_balls;

    for(int i = 0; i < previous_number_balls; i++){
        draw_ball(game_win, &current_ball->info, false);
        current_ball = current_ball->next;
        mvwprintw(message_win, i+1, 1, "                   "); 
    }

    for(int i = 0; i < previous_number_prizes; i++){
        draw_prize(game_win, &previous_prizes[i], false);
    }

    for(int i = 0; i < NUM_BOTS; i++){
        draw_bot(game_win, &previous_bots[i], false);
    }


    current_ball = balls;
    for(int i = 0; i < number_balls; i++){
        draw_ball(game_win, &current_ball->info, true);
        mvwprintw(message_win, i+1, 1, "%c: (%d, %d) %d", current_ball->info.c, current_ball->info.x, current_ball->info.y, current_ball->info.health);
        current_ball = current_ball->next;
    }
                
    
    for(int i = 0; i < number_prizes; i++){
        draw_prize(game_win, &prizes[i], true);
    }


    for(int i = 0; i < NUM_BOTS; i++){
        draw_bot(game_win, &bots[i], true);
    }

    wrefresh(message_win);
    wrefresh(game_win);


    if (previous_number_balls == 0){
               
        //mvwprintw(ID_win, 1, 1, "%s", SOCK_ADDRESS);
        mvwprintw(ID_win, 2, 1, "server");

        wrefresh(ID_win);
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


