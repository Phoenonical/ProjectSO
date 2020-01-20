#include "common.h"
#include "semaphoreSO.h"

int MAX_WIDTH;
int MAX_HEIGHT;
int MAX_MOVES;
int TOT_PLAYERS;
int MIN_HOLD_NSEC;
int TOT_PAWNS;
struct Cell *buff;
struct Message *MessageBuffer;
struct Scoreboard *ScoreTable;
struct Destination *MyTarget;
int waiting=1;
int PawnCol;
int PawnRow;
int ParentPID;
int SemID;
int MasterPID;
int PawnPID;
int MessageSemaphoreID;
int myTurn;
int PlrTurn;
int ChessboardSemaphoresID;
int ScoreTableID;
int TargetID;
struct sigaction sa; /* Structure for later signal catching */

void handle_signal(int signal);
int Move();

int main(int argc, char* argv[]){
  int shmID;
  int TRow,RCol;
  char Moved=0;

  bzero(&sa, sizeof(sa));
  sa.sa_handler = handle_signal;

  sigaction(SIGINT, &sa, NULL);
  signal(SIGUSR2,handle_signal);

  MAX_WIDTH=ConfigParser("./Settings.conf", "MAX_WIDTH");
  MAX_HEIGHT=ConfigParser("./Settings.conf", "MAX_HEIGHT");
  MAX_MOVES=ConfigParser("./Settings.conf", "MAX_MOVES");
  TOT_PLAYERS=ConfigParser("./Settings.conf", "TOT_PLAYERS");
  MIN_HOLD_NSEC=ConfigParser("./Settings.conf", "MIN_HOLD_NSEC");
  TOT_PAWNS=ConfigParser("./Settings.conf", "TOT_PAWNS");

  if(argc<5){printf("ERROR: Not enough arguments for PAWN\n"); exit(EXIT_FAILURE);}
  PawnCol = atoi(argv[0]);
  PawnRow = atoi(argv[1]);
  MasterPID = atoi(argv[2]);
  myTurn = atoi(argv[3]);
  PlrTurn = atoi(argv[4]);
  PawnPID = getpid();
  ParentPID = getppid();

  /*printf("PawnPID: %d, argc: %d\n",getpid(), argc);
  printf("Col %d\n", PawnCol);
  printf("Row %d\n", PawnRow);*/

  ScoreTableID = SharedMemID(ftok("./master",1),0);
  ScoreTable = AttachMem(ScoreTableID);
  SemID = Semaphore(ftok("./player.c",68), 5);
  shmID = SharedMemID(ftok("./",70),sizeof(struct Cell)*MAX_HEIGHT*MAX_WIDTH);
  buff = AttachMem(shmID);
  buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
  buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;


  TargetID = SharedMemID(ftok("./pawn",PlrTurn), sizeof(struct Destination)*TOT_PAWNS);
  MyTarget = AttachMem(TargetID);

  /*MessageSemaphoreID = Semaphore(ftok("./pawn",PlrTurn), 0);*/
  ChessboardSemaphoresID = Semaphore(ftok("./master.c",64),MAX_HEIGHT*MAX_WIDTH);

  wait_Sem(SemID,3);

  while(1){
    Move();
    wait_Sem(SemID,1);
    /*printf("Later Semaphore in pawn is %d\n", semctl(MessageSemaphoreID, myTurn, GETVAL));*/
  }

}




int Move(){
  int CaughtFlag=0;
  char Moved=0;
  struct timespec toWait;
  /*lock_Sem(SemID,1,0);*/
  toWait.tv_sec=0;
  toWait.tv_nsec=MIN_HOLD_NSEC;
  if(MAX_MOVES>0 && MyTarget[myTurn].Distance!=MAX_INT && MAX_MOVES>MyTarget[myTurn].Distance){
      if(PawnRow<MyTarget[myTurn].DestinationRow && Moved==0 && PawnRow<MAX_HEIGHT)
      if(lock_Sem(ChessboardSemaphoresID,(PawnRow+1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)!=-1){
          if(buff[(PawnRow+1)*MAX_WIDTH+PawnCol].Symbol=='F'){CaughtFlag=1;}
              buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
              buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
              release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
              PawnRow++;
              Moved=1;
          }

      if(PawnRow>MyTarget[myTurn].DestinationRow && Moved==0 && PawnRow>0)
      if(lock_Sem(ChessboardSemaphoresID,(PawnRow-1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)!=-1){
            if(buff[(PawnRow-1)*MAX_WIDTH+PawnCol].Symbol=='F'){CaughtFlag=1;}
              buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
              buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
              release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
              PawnRow--;
              Moved=1;
          }

      if(PawnCol<MyTarget[myTurn].DestinationCol && Moved==0 && PawnCol<MAX_WIDTH)
      if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol+1),IPC_NOWAIT)!=-1){
            if(buff[PawnRow*MAX_WIDTH+(PawnCol+1)].Symbol=='F'){CaughtFlag=1;}
              buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
              buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
              release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
              PawnCol++;
              Moved=1;
          }

      if(PawnCol>MyTarget[myTurn].DestinationCol && Moved==0 && PawnCol>0)
      if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol-1),IPC_NOWAIT)!=-1){
              if(buff[PawnRow*MAX_WIDTH+(PawnCol-1)].Symbol=='F'){CaughtFlag=1;}
              buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
              buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
              release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
              PawnCol--;
              Moved=1;
              }


    } else{/*release_Sem(SemID,1);*/return FAIL;}

  if(Moved){
    buff[PawnRow*MAX_WIDTH+PawnCol].Symbol='P';
    MAX_MOVES--;
    ScoreTable[PlrTurn-1].Moves++;
    if(CaughtFlag){
      MyTarget[myTurn].Distance=MAX_INT;
      ScoreTable[PlrTurn-1].Score+=buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points;
      buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
      buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;
      kill(MasterPID,SIGUSR1);
    }else{
      buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
      buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;}
  }else {/*release_Sem(SemID,1);*/return FAIL;}
  nanosleep(&toWait,NULL);
  /*release_Sem(SemID,1);*/
  return SUCCESS;
}


void handle_signal(int signal){
  if(signal==SIGINT)
  exit(EXIT_SUCCESS);
	Logn("Signal", signal);
}
