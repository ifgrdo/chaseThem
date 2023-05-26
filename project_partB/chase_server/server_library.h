/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "../header.h"
#define MAX_CLIENTS 3

extern int number_balls, previous_number_balls; 
extern int number_prizes, previous_number_prizes; 
    
extern prize prizes[MAX_PRIZES], previous_prizes[MAX_PRIZES]; 
extern bot bots[NUM_BOTS], previous_bots[NUM_BOTS]; 
extern ball *balls, *previous_balls;
extern sock *socks;

extern WINDOW *ID_win, *game_win, *message_win;

extern pthread_cond_t prizes_cond_variable;
extern pthread_mutex_t prizes_mutex;


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/


char new_name();                                //creates a new name
int check_repeated_names(char);                 //checks if the name chosen by the client/server already exists

void add_ball(char, int);                       //adds a new ball 
void remove_ball (int);                         //remove ball from field
void move_ball(int, int);                       //move ball
ball* copy(ball *);

prize random_prize();                           //creates a random prize
void add_prize();                               //adds prize to the field 
void remove_prize(int);                         //removes prize from the list after it is eaten

bot random_bot();                               //creates a random bot
void add_bots();                                //adds bots to the field  
void move_bot(int *);                           //moves bot

int get_index(int); 
void update(message *);
void send_messages(message, char*, int);

moving_info has_ball(int, int);                 //checks if the place to which a ball/bot is trying to move has a ball                                 
moving_info has_prize(int, int);                //checks  if the place to which a ball/bot is trying to move has a prize   
moving_info has_bot(int, int);                  //checks if the place to which a ball/bot is trying to move has a bot

void draw_ball(WINDOW *, ball_info *, int);     //draws ball
void draw_prize(WINDOW *, prize *, int);        //draws prize
void draw_bot(WINDOW *, bot *, int);            //draws bot         
void draw();                                    //updates the entire field


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/
