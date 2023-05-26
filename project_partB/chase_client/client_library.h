/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include "../header.h"

extern int previous_number_balls;
extern int previous_number_prizes;

extern prize previous_prizes[MAX_PRIZES]; 
extern bot previous_bots[NUM_BOTS];
extern ball_info *previous_balls;

extern WINDOW *ID_win, *game_win, *message_win;

extern pthread_t id_send, id_recv;
extern pthread_t id_sleep;

extern int is_waiting;

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

//draw
void draw_ball(WINDOW *, ball_info *, int);     //draws ball
void draw_prize(WINDOW *, prize *, int);        //draws prize
void draw_bot(WINDOW *, bot *, int);            //draws bot


/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

