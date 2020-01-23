#include "common.h"
#include "semaphoreSO.h"

#define TERMINATE kill(getppid(),SIGINT)

struct SO_Flag{
  int Row;
  int Col;
  int Points;
}*Flags;

void PlacePawn(int i); /* Place pawn on the playing field */
void CreatePawn(int RandRow, int RandCol, int PlayerTurn, int i); /* Create the pawn process */
char* tostring(int Num);
void InteractwPawn(); /* Ask pawn which flag is closest, send instructions and etc... */
struct Destination FindClosest();
void handle_signal(int signal);
void CleanTargets();
int CatchFlags();

int TOT_PLAYERS;
int TOT_PAWNS;
int TOT_POINTS;
int MAX_WIDTH;
int MAX_HEIGHT;
int MIN_FLAGS;
int MAX_FLAGS;
int MAX_MOVES;

struct Cell *buff; /* The playing field */
struct Message *MessageBuffer;
struct Scoreboard *ScoreTable;
struct Destination *MyTarget; /* Which flag am I'm aiming for with which pawn? */
struct PawnInfo *myPawns; /* Storing pawn PIDs */
int myPID;
int myTurn;
int MessageSemaphoreID;
int ChessboardSemaphoresID;
int TargetID;
int ScoreTableID;
int Newround=0;
int ID;


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

  /*MessageSemaphoreID = Semaphore(ftok("./pawn",myTurn), TOT_PAWNS);*/
  /*for(i=0;i<TOT_PAWNS;i++) init_Sem(MessageSemaphoreID,i,0);*/ /* For Message reading */
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

  /*for(i = 0; i<NSIG; i++){
		if(sigaction(i, &sa, NULL)==-1)
			Logn("Cannot set a user-defined handler for Signal",i);
	}*/

  /*signal(SIGUSR1,handle_signal);*/



  myPID=getpid();
  shmID = SharedMemID(ftok("./",70),sizeof(struct Cell)*MAX_HEIGHT*MAX_WIDTH);

  buff = AttachMem(shmID);

  ID = Semaphore(ftok("./player.c",68), 0);
  Logn("Player semaphoreID",ID);
  myPawns=malloc(sizeof(struct PawnInfo)*TOT_PAWNS);
  for(i=0;i<TOT_PAWNS;i++){

    while(!compare_Sem(ID,0,myTurn));
    Logn("Player's turn", myTurn);
    Wait(2);
    PlacePawn(i);

    Logn("Current Semaphore",semctl(ID, 0, GETVAL));
    release_Sem(ID, 0);

    wait_Sem(ID,0);

    Log("Done waiting, starting new turn");
  }

/*
  for(i=0;i<TOT_PAWNS;i++){
  MyTarget[i].Distance=MAX_INT;
  MyTarget[i].DestinationRow=MAX_INT;
  MyTarget[i].DestinationCol=MAX_INT;
  Logn("Pid", myPawns[i]);
}*/
  sleep(1);
  wait_Sem(ID,4);
  CleanTargets();
  InteractwPawn();
  release_Sem(ID,2);
  wait_Sem(ID,3); /* Wait until Master starts the game */
  /* So it begins - Thèoden Ednew, King of Rohan */
  Log("Beginning");

  for(i=0;i<TOT_PAWNS;i++){ /* Wakey wakey pawns */
    kill(myPawns[i].PID, SIGUSR1);
  }

    /*while(!compare_Sem(ID,0,myTurn));*/

    Logn("Player's turn", myPID);


    /*release_Sem(ID, 0);*/
    /*Logn("Incremented Semaphore",semctl(ID, 0, GETVAL));*/


    while(1){
      if(Newround){
        wait_Sem(ID,0);
        CleanTargets();
        InteractwPawn();
        for(i=0;i<TOT_PAWNS;i++){
          kill(myPawns[i].PID, SIGUSR1);
        }
        release_Sem(ID,2);
        Newround=0;
      }
      pause();
      /*  NFlags=CatchFlags();
        for(i=0;i<NFlags;i++){
          isPursued=1; *//* Assuming the flag is already pursued by a pawn */
          /*for(j=0;j<TOT_PAWNS;j++){
            if(Flags[i].Row!=MyTarget[j].DestinationRow || Flags[i].Col!=MyTarget[j].DestinationCol) isPursued=0;
          }*/

          /* Check if there's a free pawn nearby */

        /*}*/

    }


    /*wait_Sem(ID,0);*/

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
       Row=myPawns[j].Row;
       Col=myPawns[j].Col;

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
      if(buff[i*MAX_WIDTH+j].Symbol=='F'){
        Flags[l].Row=i;
        Flags[l].Col=j;
        Flags[l].Points=buff[i*MAX_WIDTH+j].Att.Points;
        l++;
      }/* END OF if(buff[i*MAX_WIDTH+j].Symbol=='F') */
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
  /*  }while(buff[randRow*MAX_WIDTH+randCol].Symbol!=' '); *//* if the cell is ALREADY occupied, we re-randomize our row and column numbers */
    /*Wait(1);*/
    buff[randRow*MAX_WIDTH+randCol].Symbol='P'; /* Placing a pawn in a random location */
    CreatePawn(randRow, randCol, myTurn, i); /* Creating the process pawn */
    /*Wait(1);*/
    myPawns[i].Row=randRow;
    myPawns[i].Col=randCol;
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

	switch(myPawns[i].PID=fork()){
	case -1: printf("ERROR: Error creating fork\n"); TERMINATE; break;
	case 0:  execve("./pawn", args, NULL); TERMINATE; break;
	/*case 0: execvp(args[0],args); exit(EXIT_FAILURE);*/
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
    for(i=0;i<TOT_PAWNS;i++){ kill(myPawns[i].PID, SIGINT);}
    while(wait(&status) != -1);
    shmctl(TargetID,IPC_RMID,NULL);
    exit(EXIT_FAILURE);
  }

  if(signal==SIGINT){
  int status;
  for(i=0;i<TOT_PAWNS;i++){ kill(myPawns[i].PID, SIGINT);}
  while(wait(&status) != -1);
  /*free(myPawns);
  free(Flags);*/
  /*remove_Sem(MessageSemaphoreID);*/
  shmctl(TargetID,IPC_RMID,NULL);
  exit(EXIT_SUCCESS);
  }

  if(signal==SIGUSR1){
    /*for(i=0;i<TOT_PAWNS;i++)
    if(MyTarget[i].Distance!=MAX_INT){
      /*printf("Target Row %d, target Col %d\n",MyTarget[i].DestinationRow,MyTarget[i].DestinationCol);*/
      /*if(buff[MyTarget[i].DestinationRow*MAX_WIDTH+MyTarget[i].DestinationCol].Symbol!='F')
        MyTarget[i]=FindClosestFlag(i);
      }*/
    Newround=1;
  }
  if(signal==SIGUSR2){
    pause();
  }
}
