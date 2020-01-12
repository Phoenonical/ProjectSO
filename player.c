#include "common.h"
#include "semaphoreSO.h"

#define TERMINATE kill(getppid(),SIGINT)

void PlacePawn(int i); /* Place pawn on the playing field */
void CreatePawn(int RandRow, int RandCol, int PlayerTurn, int i); /* Create the pawn process */
char* tostring(int Num);
void InteractwPawn(); /* Ask pawn which flag is closest, send instructions and etc... */
struct Distance FindClosest();
void handle_signal(int signal);
int Move();

int TOT_PLAYERS;
int TOT_PAWNS;
int TOT_POINTS;
int MAX_WIDTH;
int MAX_HEIGHT;
int MIN_FLAGS;
int MAX_FLAGS;

struct Cell *buff; /* The playing field */
struct Message *MessageBuffer;
struct Scoreboard *ScoreTable;
struct Distance MyTarget; /* Which flag am I'm aiming for with which pawn? */
int * myPawns; /* Storing pawn PIDs */
int myPID;
int myTurn;
int MessageSemaphoreID;
int ChessboardSemaphoresID;
int MessageQueueID;
int ScoreTableID;
int CheckforClosest=1;
int ID;


int main(int argc, char *argv[]){
  int i,j;
  int shmID;
  struct sigaction sa; /* Structure for later signal catching */
  myTurn=atoi(argv[1]);
  TOT_PLAYERS=ConfigParser("./Settings.conf", "TOT_PLAYERS");
	TOT_PAWNS=ConfigParser("./Settings.conf", "TOT_PAWNS");
	TOT_POINTS=ConfigParser("./Settings.conf", "TOT_POINTS");
	MAX_WIDTH=ConfigParser("./Settings.conf", "MAX_WIDTH");
	MAX_HEIGHT=ConfigParser("./Settings.conf", "MAX_HEIGHT");
	MAX_FLAGS=ConfigParser("./Settings.conf", "MAX_FLAGS");
	MIN_FLAGS=ConfigParser("./Settings.conf", "MIN_FLAGS");

  MessageQueueID = SharedMemID(ftok("./pawn",65), sizeof(struct Message));
  MessageBuffer = AttachMem(MessageQueueID);
  MessageSemaphoreID = Semaphore(ftok("./pawn",myTurn), TOT_PAWNS);
  for(i=0;i<TOT_PAWNS;i++) init_Sem(MessageSemaphoreID,i,1); /* For Message reading */
  ChessboardSemaphoresID = Semaphore(ftok("./master.c",64),MAX_HEIGHT*MAX_WIDTH);
  ScoreTableID = SharedMemID(ftok("./master",1),sizeof(struct Scoreboard)*TOT_PLAYERS);
  ScoreTable = AttachMem(ScoreTableID);



  /* Set Signal handler */
  bzero(&sa, sizeof(sa));
	sa.sa_handler = handle_signal;

  sigaction(SIGINT, &sa, NULL);
  /*sigaction(SIGUSR1, &sa, NULL);*/

  signal(SIGUSR1,handle_signal);



  myPID=getpid();
  shmID = SharedMemID(ftok("./",70),sizeof(struct Cell)*MAX_HEIGHT*MAX_WIDTH);

  buff = AttachMem(shmID);

  ID = Semaphore(ftok("./player.c",68), 0);
  Logn("Player semaphoreID",ID);
  myPawns=malloc(sizeof(*myPawns)*TOT_PAWNS);
  for(i=0;i<TOT_PAWNS;i++){

    while(!compare_Sem(ID,0,myTurn));
    /*lock_Sem(ID, 0);*/
    Logn("Player's turn", myTurn);
    Wait(2);
    PlacePawn(i);
    /*Wait(1);*/
    Logn("Current Semaphore",semctl(ID, 0, GETVAL));
    release_Sem(ID, 0);

    wait_Sem(ID,0);


    /*release_Sem(ID,1);
    Logn("Semaphore value",semctl(ID, 1, GETVAL));
    wait_Sem(ID,1);*/
    Log("Done waiting, starting new turn");
  }

  for(i=0;i<TOT_PAWNS;i++)
  Logn("Pid", myPawns[i]);


  wait_Sem(ID,3); /* Wait until Master starts the game */
  /* So it begins - Thèoden Ednew, King of Rohan */
  Log("Beginning");
  while(1){

    while(!compare_Sem(ID,0,myTurn));

    /*lock_Sem(ID, 0);*/
    Logn("Player's turn", myPID);
    /*printf("Player turn %d\n", myTurn);*/
    /*printf("Player's turn %d\n", myPID);*/
    Wait(1);
    InteractwPawn();
    /*Wait(1);*/

    release_Sem(ID, 0);
    Logn("Incremented Semaphore",semctl(ID, 0, GETVAL));
    /*printf("Player %d is waiting\n", myTurn);*/
    wait_Sem(ID,0);

    /*lock_Sem(ID, 0);
    Log("Locked");
    Logn("Player's turn", myPID);
    InteractwPawn();
    release_Sem(ID, 0);
    Log("Unlocked");
    release_Sem(ID, 1);
    wait_Sem(ID,1);*/
    Wait(5);
  }
}

void InteractwPawn(){
 char Moved=0;
 if(CheckforClosest){
   /*printf("Checking for closest, turn %d\n",myTurn);*/
   MyTarget = FindClosest();
   CheckforClosest=0;
  }

 Logn("Distance Target", MyTarget.Distance);

 /*printf("Distance target: %d\n", MyTarget.Distance);*/

 if((MyTarget.DestinationRow>MyTarget.SourceRow) && MyTarget.SourceRow<MAX_HEIGHT){if(Move(GOUP,MyTarget.PawnTurn)!=-1) Moved=1;}
 if(MyTarget.DestinationRow<MyTarget.SourceRow && Moved==0 && MyTarget.SourceRow>0){if(Move(GODOWN,MyTarget.PawnTurn)!=-1) Moved=1;}
 if(MyTarget.DestinationCol>MyTarget.SourceCol && Moved==0 && MyTarget.SourceCol<MAX_WIDTH){if(Move(GORIGHT,MyTarget.PawnTurn)!=-1) Moved=1;}
 if(MyTarget.DestinationCol<MyTarget.SourceCol && Moved==0 && MyTarget.SourceCol>0){if(Move(GOLEFT,MyTarget.PawnTurn)!=-1) Moved=1;}
 if(Moved==0){ /* Can't move */
    /* Let's force a poor pawn to move at random */
  /* time_t t;
   srand((unsigned) time(&t));
   int Index;
   do{
   Index=rand()%TOT_PAWNS;
 }while(Moved==0)*/
}
}

int Move(int Direction, int Index){
  /* Trying to tell my pawn to move */
  /*printf("index %d\n", Index);*/
  Logn("Sending move order to",myPawns[Index]);
  printf("Sending move order to %d\n", myPawns[Index]);
  printf("Source Row: %d, Col: %d\n", MyTarget.SourceRow, MyTarget.SourceCol);
  printf("Destination Row: %d, Col: %d\n", MyTarget.DestinationRow, MyTarget.DestinationCol);
  printf("Direction: %d\n", Direction);
  MessageBuffer->mtype=1; /* 1 is: command */
  /*printf("(player-move) type modified %d\n", MessageBuffer->mtype);*/
  MessageBuffer->message.command=Direction; /* Giving a direction */
  /*release_Sem(MessageSemaphoreID,i);*/ /* Pawn may read and write to buffer */
  init_Sem(MessageSemaphoreID,Index,1);
  kill(myPawns[Index],SIGUSR2); /* Why is it called "Kill"? I'm not killing my process */
  /*lock_Sem(MessageSemaphoreID,Index,0);*/
  wait_Sem(MessageSemaphoreID,Index);
  /*init_Sem(MessageSemaphoreID,Index,1);*/

  return MessageBuffer->mtype;
  /*lock_Sem(MessageSemaphoreID,i);*/ /* Block the pawn from reading again */
}

void PlacePawn(int i){
  time_t t; /* Here's a genius idea, I'll use the time as my srand seed */
  int randRow;
  int randCol;
  srand((unsigned) time(&t)); /* Intializes random number generator */
    Log("Check for pawn");
    do{
      randRow = (rand() % MAX_HEIGHT);
      randCol = (rand() % MAX_WIDTH);
    }while(lock_Sem(ChessboardSemaphoresID,randRow*MAX_WIDTH+randCol,IPC_NOWAIT)==-1); /* If it returns with a -1, we assume the cell is OCCUPIED */
  /*  }while(buff[randRow*MAX_WIDTH+randCol].Symbol!=' '); *//* if the cell is ALREADY occupied, we re-randomize our row and column numbers */
    /*Wait(1);*/
    buff[randRow*MAX_WIDTH+randCol].Symbol='P'; /* Placing a pawn in a random location */
    CreatePawn(randRow, randCol, myTurn, i); /* Creating the process pawn */
    /*Wait(1);*/
    Log("Placed pawn at");
    Logn("Row",randRow);
    Logn("Column",randCol);
}

void CreatePawn(int randRow, int randCol, int PlayerTurn, int i){
  char *(args)[6];
  char iTurn[10];
  char Plturn[10];
  char Col[10];
  char MasterPID[10];
  char Row[10];
  sprintf(iTurn,"%d",i);
  sprintf(Plturn,"%d",PlayerTurn);
  sprintf(Col,"%d",randCol);
  sprintf(Row,"%d",randRow);
  sprintf(MasterPID,"%d",getppid());

/*  iTurn=tostring(i);
  PlTurn=tostring(PlayerTurn);
  Col=tostring(RandCol);
  Row=tostring(RandRow);
  tostring(PlayerTurn);*/

  /*{Col,Row,PlayerPID,iTurn,Plturn,NULL};*/
	args[0] = Col;
  args[1] = Row;
  args[2] = MasterPID;
  args[3] = iTurn;
  args[4] = Plturn;
  args[5] = NULL;

	switch(myPawns[i]=fork()){
	case -1: printf("ERROR: Error creating fork\n"); TERMINATE; break;
	case 0:  execve("./pawn", args, NULL); TERMINATE; break;
	/*case 0: execvp(args[0],args); exit(EXIT_FAILURE);*/
	default: i++; Wait(1); break;
	}
}

struct Distance FindClosest(){
  /* First of all, let's ask which pawn is closest to a flag */
  int i;
  struct Distance closest;
  closest.Distance=MAX_INT;
  for(i=0;i<TOT_PAWNS;i++){
  /*  printf("Sending message to %d\n",myPawns[i]);*/
    Logn("Sending message to",myPawns[i]);
    MessageBuffer->mtype=1; /* 1 is: command */
  /*  printf("(player-FindClosest) type modified %d\n", MessageBuffer->mtype);*/
    MessageBuffer->message.command=0; /* 0 is: Closest flag */
    /*printf("(player-FindClosest) command modified %d\n", MessageBuffer->message.command);*/
    /*release_Sem(MessageSemaphoreID,i);*/ /* Pawn may read and write to buffer */
  /*  printf("I is %d\n",i);*/
    /*printf("Semaphore is %d\n", semctl(MessageSemaphoreID, 0, GETVAL));*/
    init_Sem(MessageSemaphoreID,i,1);
    /*printf("Turn is %d, PawnPID is %d\n",i,myPawns[i]);*/
    kill(myPawns[i],SIGUSR2); /* Why is it called "Kill"? I'm not killing my process */
    /*sleep(1);*/
    /*printf("Later Semaphore is %d\n", semctl(MessageSemaphoreID, i, GETVAL));*/
    wait_Sem(MessageSemaphoreID,i); /* I hate this part */
    /*printf("Laterer Semaphore is %d\n", semctl(MessageSemaphoreID, i, GETVAL));*/
    /*sleep(1);*/
    /*while(!compare_Sem(MessageSemaphoreID,i,1));*/ /* Wait until pawn is done writing */
    /*lock_Sem(MessageSemaphoreID,i);*/ /* Block the pawn from reading again */
    if(MessageBuffer->mtype==2)
    if(closest.Distance>MessageBuffer->message.Loc.Distance){
      closest.Distance=MessageBuffer->message.Loc.Distance;
      closest.DestinationRow=MessageBuffer->message.Loc.DestinationRow;
      closest.DestinationCol=MessageBuffer->message.Loc.DestinationCol;
      closest.SourceRow=MessageBuffer->message.Loc.SourceRow;
      closest.SourceCol=MessageBuffer->message.Loc.SourceCol;
      closest.PawnTurn=i;
    }else; else{printf("ERROR: (Player) Message error message type is %d\n",MessageBuffer->mtype); sleep(3);}
  }
  /*printf("Distance %d\n", closest.Distance);*/
  return closest;
}

void handle_signal(int signal){
  int i;
	Logn("Signal", signal);
  /*printf("Signal %d\n", signal);*/
  if(signal==2){
  int status;
  for(i=0;i<TOT_PLAYERS;i++) kill(myPawns[i], 2);
  while(myPawns[i] = wait(&status) != -1){i++;}
  remove_Sem(MessageSemaphoreID);
  exit(EXIT_SUCCESS);
  }
  if(signal==SIGUSR1){
    if(buff[MyTarget.DestinationRow*MAX_WIDTH+MyTarget.DestinationCol].Symbol!='F')
    CheckforClosest=1;
    lock_Sem(ID,2,0);
  }
}
