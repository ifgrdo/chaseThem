/*---------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#define WINDOW_SIZE 23
#define NUM_BOTS 10
#define MAX_PRIZES 10


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <pthread.h>

#include <ncurses.h>


typedef struct ball_info{
    int x, y;                   //coordinates of the ball
    char c;                     //name of the ball
    int health;                 //health of the ball
} ball_info;


typedef struct ball{
    ball_info info;             //struct defined above
    struct ball *next;          //next ball of the list
} ball;


typedef struct sock{
    int value;                  //sock_fd
    int status;                 //0: waiting, 1: active
    struct sock *next;          //next socket of the list
} sock;


typedef struct bot{
    int x, y;                   //coordinates of the ball
} bot;


typedef struct prize{
    int x, y;                   //coordinates of the prize 
    int value;                  //value of the prize
} prize;


typedef struct moving_info{     
    int is_occupied;            //indicates if the position to where the object tries to move is occupied (by a ball, bot or prize)
    int index_rammed_ball;      //if it is occupied by a ball, we need to store its index in the array of balls in order to decrease its health
    int prize_value;            //if it is occupied by a prize, we need to store its value and index in order to, respectively, increase the health of the ball and then remove the prize from the list
    int index_prize;            //index of the prize (in the prizes array) that was in the place to where a ball has moved
} moving_info;


typedef struct message{
    int number_balls;
    int number_prizes;
    
    prize prizes[MAX_PRIZES];       //prizes array
    bot bots[NUM_BOTS];             //bots array

    int can_move;                   //can_move = 1 when the pressed key is an arrow, can_move = 0 otherwise
    int key;                        //pressed key 
    int type;                       //message type

} message;

