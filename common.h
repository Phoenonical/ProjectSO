#ifndef _COMMON_H_
#define _COMMON_H_

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<time.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<errno.h>

/* Once ready, signal the players with a sempaphore */

/* Once the players have given instructions to their pawn, everyone makes a move, then repeat
Though, after the first instructions, the master is informed and starts the timer */

/* Once a flag is captured, all pawns, players and the master are informed */

/* The players put the pawns down, not the master */

/* The players place their pawns one at a time, The flags must not be placed until the players are done placing their pawns */

/* Pawns can only move in 4 directions, not diagonally and can't go outside of the playing field */

/* Each pawn can do a maximum of moves in it's life span */

/* Uncomment for DEBUG mode OR compile with -DDEBUG */
/*#define DEBUG*/

#ifdef DEBUG /* Necessary for debugging purposes */
/*#define Log(Message, ...) fprintf(stderr, "[LOG] (%s:%d) " Message "\n",__FILE__, __LINE__, ##__VA_ARGS__)*/
#define Logn(Message, val) fprintf(stderr, "[LOGN] (%s:%d) %s: %d\n",__FILE__, __LINE__, Message, val)
#define Log(Message) fprintf(stderr, "[LOG] (%s:%d) %s\n",__FILE__, __LINE__, Message)
#define Wait(Time) sleep(Time);
#else
/*#define Log(Message, ...)*/
#define Logn(Message, val)
#define Log(Message)
#define Wait(Time)
#endif

#define MAX_INT 32767
#define MIN_INT -32768

#define FAIL -1
#define SUCCESS 0

struct Pawn{
	int PIDParent;
	int PIDPawn;
};

union Attribute{
int Points; /* For flag, is 0 or NULL when empty */
struct Pawn pawn; /* For Pawn */
};

/* Defining a Cell structure */
struct Cell{
	char Symbol; /* Can be either a F for Flag, or P for Pawn */
	union Attribute Att;
};

/* Defining a message type */
/*struct Distance{
	int Distance;
	int DestinationCol;
	int DestinationRow;
	int SourceCol;
	int SourceRow;
	int PawnTurn;
};
*/
struct PawnInfo{
	int Row;
	int Col;
	int PID;
};

struct Destination{
	int Distance;
	int DestinationCol;
	int DestinationRow;
	int Fuel;
};

/* Defining a message structre for Player-Pawn communication */
/*union MessageBody{
	int command;
	struct Distance Loc;
};*/

struct Scoreboard{
	int Moves;
	int Score;
};

/* command = ...
 * = 0 (Closest flag)
 * = 1 (Go up)
 * = 2 (Go down)
 * = 3 (Go right)
 * = 4 (Go left)
*/

/*struct Message{
	char mtype;
	union MessageBody message;
};*/

/* mtype = ...
 * = -1 (Error)
 * = 0 (Success)
 * = 1 (Command)
 * = 2 (PawnLocation)
*/


/* Reads a .conf file and checks if YOU (That's right, YOU.) didn't mess up the .conf file
 * Path = The path of the .conf file
 * toGet = Which data to retrieve?
 */
int ConfigParser(char* Path, char* toGet);

char* tostring(int Num);

int SharedMemID(int Key, int size); /* Get the ID of a Shared Memory segment or create a new one */

void* AttachMem(int shmID); /* Return a pointer to a shared memory segment */

int MinMax(int Min, int Max);

#endif /* _COMMON_H_ */
