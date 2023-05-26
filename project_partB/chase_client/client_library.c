/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "client_library.h"

int previous_number_balls = 0;
int previous_number_prizes = 0;

int is_waiting;

pthread_t id_send, id_recv;
pthread_t id_sleep;

ball_info *previous_balls;
prize previous_prizes[MAX_PRIZES]; 
bot previous_bots[NUM_BOTS];

WINDOW *ID_win, *game_win, *message_win;


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
    waddch(win,ch);
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
    waddch(win,ch);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/
