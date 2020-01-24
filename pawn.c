#include "common.h"
#include "semaphoreSO.h"

#define GOUP 1
#define GODOWN 2
#define GORIGHT 3
#define GOLEFT 4


int MAX_WIDTH;
int MAX_HEIGHT;
int MAX_MOVES;
int TOT_PLAYERS;
int MIN_HOLD_NSEC;
int TOT_PAWNS;

struct Cell *Chessboard;
struct Scoreboard *ScoreTable;
struct Destination *MyTarget;
int PawnCol;
int PawnRow;
int ParentPID;
int SemID;
int MasterPID;
int PawnPID;
int myTurn;
int PlrTurn;
int ChessboardSemaphoresID;
int ScoreTableID;
int TargetID;
struct sigaction sa; /* Structure for later signal catching */

void handle_signal(int signal);
int Move();
int Where(int Direction);
int FetchaTarget();

int main(int argc, char* argv[]){
  int shmID;

  bzero(&sa, sizeof(sa));
  sa.sa_handler = handle_signal;

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGUSR1,&sa, NULL);

  MAX_WIDTH=ConfigParser("./Settings.conf", "MAX_WIDTH");
  MAX_HEIGHT=ConfigParser("./Settings.conf", "MAX_HEIGHT");
  MAX_MOVES=ConfigParser("./Settings.conf", "MAX_MOVES");
  TOT_PLAYERS=ConfigParser("./Settings.conf", "TOT_PLAYERS");
  MIN_HOLD_NSEC=ConfigParser("./Settings.conf", "MIN_HOLD_NSEC");
  TOT_PAWNS=ConfigParser("./Settings.conf", "TOT_PAWNS");

  if(argc<3){printf("ERROR: Not enough arguments for PAWN\n"); exit(EXIT_FAILURE);}
  MasterPID = atoi(argv[0]);
  myTurn = atoi(argv[1]);
  PlrTurn = atoi(argv[2]);
  PawnPID = getpid();
  ParentPID = getppid();

  /*printf("PawnPID: %d, argc: %d\n",getpid(), argc);
  printf("Col %d\n", PawnCol);
  printf("Row %d\n", PawnRow);*/

  ScoreTableID = SharedMemID(ftok("./master",1),0);
  ScoreTable = AttachMem(ScoreTableID);
  SemID = Semaphore(ftok("./player.c",68), 5);
  shmID = SharedMemID(ftok("./",70),sizeof(struct Cell)*MAX_HEIGHT*MAX_WIDTH);
  Chessboard = AttachMem(shmID);

  TargetID = SharedMemID(ftok("./pawn",PlrTurn), sizeof(struct Destination)*TOT_PAWNS);
  MyTarget = AttachMem(TargetID);

  PawnCol = MyTarget[myTurn].SourceCol;
  PawnRow = MyTarget[myTurn].SourceRow;

  Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
  Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;

  pause();

  /*MessageSemaphoreID = Semaphore(ftok("./pawn",PlrTurn), 0);*/
  ChessboardSemaphoresID = Semaphore(ftok("./master.c",64),MAX_HEIGHT*MAX_WIDTH);


  while(1){
    wait_Sem(SemID,1);
    if(MyTarget[myTurn].Distance==MAX_INT){lock_Sem(SemID,5,0); if(FetchaTarget()==-1) pause();} /* Check one more time before sleeping */
    if(MyTarget[myTurn].Distance!=MAX_INT && Check()==-1){lock_Sem(SemID,5,0); if(FetchaTarget()==-1) pause();} /* Check to see if the pawn still has enough fuel, if not, check if another flag is closeby */
    Move();
  }

}

int Check(){ /* Check if it can still reach it's destination */
  int Distance=0;

  if(PawnRow>MyTarget[myTurn].DestinationRow) Distance+=PawnRow-MyTarget[myTurn].DestinationRow;
  else if(PawnRow<MyTarget[myTurn].DestinationRow) Distance+=MyTarget[myTurn].DestinationRow-PawnRow;
  else if(PawnRow==MyTarget[myTurn].DestinationRow) Distance+=0;

  if(PawnCol>MyTarget[myTurn].DestinationCol) Distance+=PawnCol-MyTarget[myTurn].DestinationCol;
  else if(PawnCol<MyTarget[myTurn].DestinationCol) Distance+=MyTarget[myTurn].DestinationCol-PawnCol;
  else if(PawnCol==MyTarget[myTurn].DestinationCol) Distance+=0;

  if(Distance<MyTarget[myTurn].Fuel) return SUCCESS; else return FAIL;

}

int Where(int Direction){ /* Where to move? */
  char CaughtFlag;
  CaughtFlag=0;
    switch (Direction) {
      case GOUP: if(lock_Sem(ChessboardSemaphoresID,(PawnRow+1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)!=-1){
        if(Chessboard[(PawnRow+1)*MAX_WIDTH+PawnCol].Symbol=='F') CaughtFlag=1;
            Chessboard[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
            Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
            release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn processes */
            PawnRow++;
      }else return FAIL; break;
      case GODOWN: if(lock_Sem(ChessboardSemaphoresID,(PawnRow-1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)!=-1){
          if(Chessboard[(PawnRow-1)*MAX_WIDTH+PawnCol].Symbol=='F') CaughtFlag=1;
              Chessboard[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
              Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
              release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn processes */
              PawnRow--;
      }else return FAIL; break;
      case GORIGHT: if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol+1),IPC_NOWAIT)!=-1){
          if(Chessboard[PawnRow*MAX_WIDTH+(PawnCol+1)].Symbol=='F') CaughtFlag=1;
              Chessboard[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
              Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
              release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn processes */
              PawnCol++;
      }else return FAIL; break;
      case GOLEFT: if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol-1),IPC_NOWAIT)!=-1){
          if(Chessboard[PawnRow*MAX_WIDTH+(PawnCol-1)].Symbol=='F') CaughtFlag=1;
              Chessboard[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
              Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
              release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawnprocesses */
              PawnCol--;
      }else return FAIL; break;
      default: return FAIL;break;
    }
      /* The pawn has moved, update it's position, moves and if necessary, the score */
      MyTarget[myTurn].SourceRow=PawnRow;
      MyTarget[myTurn].SourceCol=PawnCol;
      Chessboard[PawnRow*MAX_WIDTH+PawnCol].Symbol='P';
      lock_Sem(SemID,6,0); /* One pawn may modify it at a time */
      MyTarget[myTurn].Fuel--;
      ScoreTable[PlrTurn-1].Moves++;
      release_Sem(SemID,6);
      if(CaughtFlag){
        MyTarget[myTurn].Distance=MAX_INT;
        ScoreTable[PlrTurn-1].Score+=Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.Points;
        FetchaTarget();
      }
      Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
      Chessboard[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;

      return SUCCESS;

}


int Move(){
  int static VoH=1; /* Vertical or horizontal movement */
  int CaughtFlag=0;
  char Moved=0;
  struct timespec toWait;
  toWait.tv_sec=0;
  toWait.tv_nsec=MIN_HOLD_NSEC;
  if(MyTarget[myTurn].Fuel>0 && MyTarget[myTurn].Distance!=MAX_INT && Chessboard[MyTarget[myTurn].DestinationRow*MAX_WIDTH+MyTarget[myTurn].DestinationCol].Symbol=='F'){


    if(VoH){
      if(PawnRow<MyTarget[myTurn].DestinationRow && PawnRow<MAX_HEIGHT)
        if(Where(GOUP)==0) Moved=1;else;
      else if(PawnRow>MyTarget[myTurn].DestinationRow && PawnRow>0)
        if(Where(GODOWN)==0) Moved=1;
      if(!Moved){VoH=0;}
    }else{
      if(PawnCol<MyTarget[myTurn].DestinationCol && Moved==0 && PawnCol<MAX_WIDTH)
        if(Where(GORIGHT)==0) Moved=1;else;
      else if(PawnCol>MyTarget[myTurn].DestinationCol && Moved==0 && PawnCol>0)
        if(Where(GOLEFT)==0) Moved=1;else;
      if(!Moved){VoH=1;}
    }

  }else{return FAIL;}

  nanosleep(&toWait,NULL);
  /*release_Sem(SemID,1);*/
  return SUCCESS;
}

int FetchaPawn(){

}

int FetchaTarget(){ /* The Pawn can decide to go after a closest flag IF and only IF there's no other pawn pursuing it */
    struct Destination closest;
    int i,j,Row,Col,Distance;
    closest.Distance=MAX_INT;
    for(i=0;i<MAX_HEIGHT;i++){
      for(j=0;j<MAX_WIDTH;j++){
        if(Chessboard[i*MAX_WIDTH+j].Symbol=='F'){
          Distance=0;
          Row=PawnRow;
          Col=PawnCol;

          if(Row>i) Distance+=Row-i;
          else if(Row<i) Distance+=i-Row;
          else if(Row==i) Distance+=0;

          if(Col>j) Distance+=Col-j;
          else if(Col<j) Distance+=j-Col;
          else if(Col==j) Distance+=0;

          if(Distance<closest.Distance){
            if(MyTarget[myTurn].Fuel>Distance){ /* Check if the pawn has enough fuel */
              closest.Distance=Distance;
              closest.DestinationRow=i;
              closest.DestinationCol=j;
             }
          }
        }
      }
    }
    if(closest.Distance==MAX_INT){release_Sem(SemID,5); return FAIL;} /* Check if a flag found a pawn */
    for(i=0;i<TOT_PAWNS;i++){
        if(closest.DestinationCol==MyTarget[i].DestinationCol && MyTarget[i].DestinationRow==closest.DestinationRow)
          if(closest.Distance<MyTarget[i].Distance)MyTarget[i].Distance=MAX_INT; else{release_Sem(SemID,5); return FAIL;}
    }

    MyTarget[myTurn].DestinationCol=closest.DestinationCol;
    MyTarget[myTurn].DestinationRow=closest.DestinationRow;
    MyTarget[myTurn].Distance=closest.Distance;
    release_Sem(SemID,5);
    return SUCCESS;
}


void handle_signal(int signal){
  if(signal==SIGINT)
  exit(EXIT_SUCCESS);
	Logn("Signal", signal);
  /*printf("Signal %d\n", signal);*/
}
