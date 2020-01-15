#include "common.h"
#include "semaphoreSO.h"

#define TERMINATE kill(getppid(),SIGINT)

void PlacePawn(int i); /* Place pawn on the playing field */
void CreatePawn(int RandRow, int RandCol, int PlayerTurn, int i); /* Create the pawn process */
char* tostring(int Num);
void InteractwPawn(); /* Ask pawn which flag is closest, send instructions and etc... */
struct Distance FindClosest();
void handle_signal(int signal);
void CleanTargets();

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
struct Distance *MyTarget; /* Which flag am I'm aiming for with which pawn? */
struct PawnInfo *myPawns; /* Storing pawn PIDs */
int myPID;
int myTurn;
int MessageSemaphoreID;
int ChessboardSemaphoresID;
int TargetID
int ScoreTableID;
int Newround=1;
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

  TargetID = SharedMemID(ftok("./pawn",myTurn), sizeof(struct Destination)*TOT_PAWNS);
  MyTarget = AttachMem(TargetID);

  MessageSemaphoreID = Semaphore(ftok("./pawn",myTurn), TOT_PAWNS);
  for(i=0;i<TOT_PAWNS;i++) init_Sem(MessageSemaphoreID,i,0); /* For Message reading */
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

  for(i=0;i<TOT_PAWNS;i++){
  MyTarget[i].distance=MAX_INT;
  MyTarget[i].Row=MAX_INT;
  MyTarget[i].Col=MAX_INT;
  Logn("Pid", myPawns[i]);
  }

  InteractwPawn();
  wait_Sem(ID,3); /* Wait until Master starts the game */
  /* So it begins - Thèoden Ednew, King of Rohan */
  Log("Beginning");

    /*while(!compare_Sem(ID,0,myTurn));*/

    Logn("Player's turn", myPID);


    /*release_Sem(ID, 0);*/
    /*Logn("Incremented Semaphore",semctl(ID, 0, GETVAL));*/

    while(1){
      if(Newround){
        CleanTargets();
        InteractwPawn();
      }
    }

    /*wait_Sem(ID,0);*/

}

void CleanTargets(){
  for(i=0;i<TOT_PAWNS;i++){
    for(j=0;j<TOT_PAWNS;j++){
      if(MyTarget[i].Destination>MyTarget[j]->Destination && MyTarget[i].Row==MyTarget[j]->Row && MyTarget[i].Col==MyTarget[j].Col){
        MyTarget[i].Destination=MAX_INT;
        MyTarget[i].Row=-1;
        MyTarget[i].Col=-1;
      }
    }
  }
}

void InteractwPawn(){
   char Moved=0;
   int i,j;
   struct Destination temp;
   for(i=0;i<TOT_PAWNS;i++){
     MyTarget[i] = FindClosest(i);
   }
}

struct Destination FindClosest(int Index){
  int i,j,l,k,Index;
  char NottoCheck[TOT_PAWNS];
  struct Destination closest;
  int distance;
  for(i=0;i<MAX_HEIGHT;i++){
    for(j=0;j<MAX_WIDTH;j++){
      if(buff[i*MAX_WIDTH+j].Symbol=='F'){
          for(k=0;k<TOT_PAWNS;k++) NottoCheck[k]=0;
          do{
            closest.Distance=MAX_INT;

            for(l=0;l<TOT_PAWNS;l++){
            distance=0;
            if(myPawns[l]->Row>i) distance+=Row-i;
            else if(myPawns[l]->Row<i) distance+=i-Row;
            else if(myPawns[l]->Row==i) distance+=0;

            if(myPawns[l]->Col>j) distance+=Col-j;
            else if(myPawns[l]->Col<j) distance+=j-Col;
            else if(myPawns[l]->Col==j) distance+=0;

            if(closest.Distance>distance && NottoCheck[l]){
              closest.Distance=distance;
              closest.DestinationRow=i;
              closest.DestinationCol=j;
              Index=l;
            }
          }

          if(MyTarget[Index].Distance>closest.Distance){
            MyTarget[Index]->Distance=closest.Distance;
            MyTarget[Index]->DestinationRow=closest.DestinationRow;
            MyTarget[Index]->DestinationCol=closest.DestinationCol;

          }else NottoCheck[Index]=1;
        }while(NottoCheck[Index]);

      }
    }
  }
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
    myPawns[i]->Row=randRow;
    myPawns[i]->Col=randCol;
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

	switch(myPawns[i]->PID=fork()){
	case -1: printf("ERROR: Error creating fork\n"); TERMINATE; break;
	case 0:  execve("./pawn", args, NULL); TERMINATE; break;
	/*case 0: execvp(args[0],args); exit(EXIT_FAILURE);*/
	default: i++; Wait(1); break;
	}
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
    for(i=0;i<TOT_PAWNS;i++)
    if(MyTarget[i].Distance!=MAX_INT)
      if(buff[MyTarget[i].DestinationRow*MAX_WIDTH+MyTarget[i].DestinationCol].Symbol!='F')
        MyTarget[i].Distance=MAX_INT;
  }
  if(signal==SIGUSR2){
    Newround=1;
  }
}
