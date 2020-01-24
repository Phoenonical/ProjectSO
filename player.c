#include "common.h"
#include "semaphoreSO.h"

#define TERMINATE kill(getppid(),SIGINT)

struct SO_Flag{
  int Row;
  int Col;
  int Points;
}*Flags;

void PlacePawn(int i); /* Place pawn on the playing field */
void CreatePawn(int PlayerTurn, int i); /* Create the pawn process */
void InteractwPawn(); /* Send destination to pawn.. */
void handle_signal(int signal);
void CleanTargets();
int CatchFlags(); /* Scan the board for flags and memorize them in the SO_FLAG structure */

int TOT_PLAYERS;
int TOT_PAWNS;
int TOT_POINTS;
int MAX_WIDTH;
int MAX_HEIGHT;
int MIN_FLAGS;
int MAX_FLAGS;
int MAX_MOVES;

struct Cell *Chessboard; /* The playing field */
struct Scoreboard *ScoreTable;
struct Destination *MyTarget; /* Which flag am I'm aiming for with which pawn? */
int ChessboardSemaphoresID;
int *myPawns; /* Storing pawn PIDs */
int myPID;
int myTurn;
int TargetID;
int ScoreTableID;
int Newround=0;
int SemID;


int main(int argc, char *argv[]){
  int i,j,NFlags,isPursued=0;
  int shmID;
  struct sigaction sa; /* Structure for later signal catching */
  sigset_t  my_mask;
  myTurn=atoi(argv[1]);
  TOT_PLAYERS=ConfigParser("./Settings.conf", "TOT_PLAYERS");
	TOT_PAWNS=ConfigParser("./Settings.conf", "TOT_PAWNS");
	TOT_POINTS=ConfigParser("./Settings.conf", "TOT_POINTS");
	MAX_WIDTH=ConfigParser("./Settings.conf", "MAX_WIDTH");
	MAX_HEIGHT=ConfigParser("./Settings.conf", "MAX_HEIGHT");
	MAX_FLAGS=ConfigParser("./Settings.conf", "MAX_FLAGS");
	MIN_FLAGS=ConfigParser("./Settings.conf", "MIN_FLAGS");
  MAX_MOVES=ConfigParser("./Settings.conf", "MAX_MOVES");

  Flags = malloc(sizeof(struct SO_Flag)*MAX_FLAGS);

  TargetID = SharedMemID(ftok("./pawn",myTurn), sizeof(struct Destination)*TOT_PAWNS);
  MyTarget = AttachMem(TargetID);

  for(i=0;i<TOT_PAWNS;i++) MyTarget[i].Fuel=MAX_MOVES;

  ChessboardSemaphoresID = Semaphore(ftok("./master.c",64),MAX_HEIGHT*MAX_WIDTH);
  ScoreTableID = SharedMemID(ftok("./master",1),sizeof(struct Scoreboard)*TOT_PLAYERS);
  ScoreTable = AttachMem(ScoreTableID);



  /* Set Signal handler */
  bzero(&sa, sizeof(sa));
	sa.sa_handler = handle_signal;
  sa.sa_flags = SA_NODEFER;

  sigemptyset(&my_mask);        /* do not mask any signal */
  sa.sa_mask = my_mask;

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGKILL, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);
  sigaction(SIGCHLD, &sa, NULL);




  myPID=getpid();
  shmID = SharedMemID(ftok("./",70),sizeof(struct Cell)*MAX_HEIGHT*MAX_WIDTH);

  Chessboard = AttachMem(shmID);

  SemID = Semaphore(ftok("./player.c",68), 0);
  Logn("Player semaphoreID",ID);
  myPawns=malloc(sizeof(int)*TOT_PAWNS);
  for(i=0;i<TOT_PAWNS;i++){

    while(!compare_Sem(SemID,0,myTurn));
    Logn("Player's turn", myTurn);
    Wait(2);
    PlacePawn(i);

    Logn("Current Semaphore",semctl(ID, 0, GETVAL));
    release_Sem(SemID, 0);

    wait_Sem(SemID,0);

    Log("Done waiting, starting new turn");
  }

  sleep(1);
  wait_Sem(SemID,4);
  CleanTargets();
  InteractwPawn();
  release_Sem(SemID,2);
  wait_Sem(SemID,3); /* Wait until Master starts the game */
  /* So it begins - Thèoden Ednew, King of Rohan */
  Log("Beginning");

  for(i=0;i<TOT_PAWNS;i++){ /* Wakey wakey pawns */
    kill(myPawns[i], SIGUSR1);
  }


    Logn("Player's turn", myPID);


    while(1){
      if(Newround){
        wait_Sem(SemID,0);
        CleanTargets();
        InteractwPawn();
        for(i=0;i<TOT_PAWNS;i++){
          kill(myPawns[i], SIGUSR1);
        }
        release_Sem(SemID,2);
        Newround=0;
      }
      pause();
    }


}

void CleanTargets(){
  int i;
  for(i=0;i<TOT_PAWNS;i++){
        MyTarget[i].Distance=MAX_INT;
        MyTarget[i].DestinationRow=-1;
        MyTarget[i].DestinationCol=-1;
  }
}

void InteractwPawn(){
   int i,j,NFlags,Distance,Index;
   int Row,Col,FlagRow,FlagCol;
   struct Destination closest;
   NFlags=CatchFlags();
   /*printf("NFlags: %d\n", NFlags);*/
   for(i=0;i<NFlags;i++){
     closest.Distance=MAX_INT;
     FlagRow=Flags[i].Row;
     FlagCol=Flags[i].Col;
     for(j=0;j<TOT_PAWNS;j++){
       Distance=0;

       Row=MyTarget[j].SourceRow;
       Col=MyTarget[j].SourceCol;


       if(Row>FlagRow) Distance+=Row-FlagRow;
       else if(Row<FlagRow) Distance+=FlagRow-Row;
       else if(Row==FlagRow) Distance+=0;

       if(Col>FlagCol) Distance+=Col-FlagCol;
       else if(Col<FlagCol) Distance+=FlagCol-Col;
       else if(Col==FlagCol) Distance+=0;

       if(Distance<closest.Distance){
         if(MyTarget[j].Fuel>Distance && MyTarget[j].Distance==MAX_INT){ /* Check if the pawn has enough fuel and if it was already assigned a flag */
           closest.Distance=Distance;
           closest.DestinationRow=FlagRow;
           closest.DestinationCol=FlagCol;
           Index=j;
          }
       }
     }
     if(closest.Distance!=MAX_INT){
       /*printf("Target assigned, Row %d, Col %d, Distance %d, Fuel %d\n",closest.DestinationRow,closest.DestinationCol,closest.Distance,MyTarget[Index].Fuel);*/
       MyTarget[Index].Distance=closest.Distance;
       MyTarget[Index].DestinationRow=closest.DestinationRow;
       MyTarget[Index].DestinationCol=closest.DestinationCol;
     }
   }



}

int CatchFlags(){
  int i,j,l;
  l=0;
  for(i=0;i<MAX_HEIGHT;i++){
    for(j=0;j<MAX_WIDTH;j++){
      if(Chessboard[i*MAX_WIDTH+j].Symbol=='F'){
        Flags[l].Row=i;
        Flags[l].Col=j;
        Flags[l].Points=Chessboard[i*MAX_WIDTH+j].Att.Points;
        l++;
      }/* END OF if(Chessboard[i*MAX_WIDTH+j].Symbol=='F') */
    }/* END OF for(j=0;j<MAX_WIDTH;j++) */
  }/* END OF for(i=0;i<MAX_HEIGHT;i++) */
  return l;
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
    Chessboard[randRow*MAX_WIDTH+randCol].Symbol='P'; /* Placing a pawn in a random location */
    CreatePawn(myTurn, i); /* Creating the process pawn */
    /*Wait(1);*/
    MyTarget[i].SourceRow=randRow;
    MyTarget[i].SourceCol=randCol;
    Log("Placed pawn at");
    Logn("Row",randRow);
    Logn("Column",randCol);
}

void CreatePawn(int PlayerTurn, int i){
  char *(args)[5];
  char iTurn[10];
  char Plturn[10];
  char MasterPID[10];
  sprintf(iTurn,"%d",i);
  sprintf(Plturn,"%d",PlayerTurn);
  sprintf(MasterPID,"%d",getppid());

  args[0] = MasterPID;
  args[1] = iTurn;
  args[2] = Plturn;
  args[3] = NULL;

	switch(myPawns[i]=fork()){
	case -1: printf("ERROR: Error creating fork\n"); TERMINATE; break;
	case 0:  execve("./pawn", args, NULL); TERMINATE; break;
	default: Wait(1); break;
	}
}

void handle_signal(int signal){
  int i;
	Logn("Signal", signal);
  /*printf("Signal %d from %d\n", signal, getpid());*/

  if(signal==SIGSEGV || signal==SIGILL || signal==SIGFPE || signal==SIGKILL){
    int status;
    printf("Player %d has just died\n", getpid());
    for(i=0;i<TOT_PAWNS;i++){ kill(myPawns[i], SIGINT);}
    while(wait(&status) != -1);
    shmctl(TargetID,IPC_RMID,NULL);
    exit(EXIT_FAILURE);
  }

  if(signal==SIGINT){
  int status;
  for(i=0;i<TOT_PAWNS;i++){ kill(myPawns[i], SIGINT);}
  while(wait(&status) != -1);
  /*free(myPawns);
  free(Flags);*/
  /*remove_Sem(MessageSemaphoreID);*/
  shmctl(TargetID,IPC_RMID,NULL);
  exit(EXIT_SUCCESS);
  }

  if(signal==SIGUSR1){
    Newround=1;
  }
  if(signal==SIGUSR2){
    pause();
  }
}
