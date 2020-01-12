#include "common.h"
#include "semaphoreSO.h"

int MAX_WIDTH;
int MAX_HEIGHT;
int MAX_MOVES;
int TOT_PLAYERS;
struct Cell *buff;
struct Message *MessageBuffer;
struct Scoreboard *ScoreTable;
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
struct sigaction sa; /* Structure for later signal catching */

int WaitforMSG();
struct Distance ShortestDistance();
void handle_signal(int signal);
int Move(int Direction);

int main(int argc, char* argv[]){
  int shmID;
  int MessageQueueID;

  /*bzero(&sa, sizeof(sa));
  sa.sa_handler = handle_signal;

  sigaction(SIGINT, &sa, NULL);*/

  signal(SIGUSR2,handle_signal);

  MAX_WIDTH=ConfigParser("./Settings.conf", "MAX_WIDTH");
  MAX_HEIGHT=ConfigParser("./Settings.conf", "MAX_HEIGHT");
  MAX_MOVES=ConfigParser("./Settings.conf", "MAX_MOVES");
  TOT_PLAYERS=ConfigParser("./Settings.conf", "TOT_PLAYERS");

  if(argc<5){printf("ERROR: Not enough arguments for PAWN\n"); exit(EXIT_FAILURE);}
  PawnCol = atoi(argv[0]);
  PawnRow = atoi(argv[1]);
  MasterPID = atoi(argv[2]);
  myTurn = atoi(argv[3]);
  PlrTurn = atoi(argv[4]);
  PawnPID = getpid();
  ParentPID=getppid();

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


  MessageQueueID = SharedMemID(ftok("./pawn",65), sizeof(struct Message));
  MessageBuffer = AttachMem(MessageQueueID);
  MessageSemaphoreID = Semaphore(ftok("./pawn",PlrTurn), 0);
  ChessboardSemaphoresID = Semaphore(ftok("./master.c",64),MAX_HEIGHT*MAX_WIDTH);

  while(1){
    switch(WaitforMSG()){
      case 0: Log("Pawn Case 0");
              MessageBuffer->mtype=2;
              MessageBuffer->message.Loc=ShortestDistance();
              break;
      case 1: Log("Pawn case 1"); MessageBuffer->mtype=Move(GOUP);break;
      case 2: Log("Pawn case 2"); MessageBuffer->mtype=Move(GODOWN);break;
      case 3: Log("Pawn case 3"); MessageBuffer->mtype=Move(GORIGHT);break;
      case 4: Log("Pawn case 4"); MessageBuffer->mtype=Move(GOLEFT);break;
      default: Logn("Pawn Case default",MessageBuffer->mtype);break;
    }
    waiting=1;
    /*printf("Pawn waiting: %d\n", PawnPID);*/
    /*printf("Sending type %d\n", MessageBuffer->mtype);*/
    /*printf("Semaphore in pawn is %d\n", semctl(MessageSemaphoreID, myTurn, GETVAL));*/
    init_Sem(MessageSemaphoreID,myTurn,0);
    /*printf("Later Semaphore in pawn is %d\n", semctl(MessageSemaphoreID, myTurn, GETVAL));*/
  }

}


int Move(int Direction){
  int CaughtFlag=0;
  if(MAX_MOVES>0){
    switch (Direction) {
      case 1: if(lock_Sem(ChessboardSemaphoresID,(PawnRow+1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)==-1 && buff[(PawnRow+1)*MAX_WIDTH+PawnCol].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[(PawnRow+1)*MAX_WIDTH+PawnCol].Symbol=='F'){CaughtFlag=1;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnRow++;
              }break;
      case 2: if(lock_Sem(ChessboardSemaphoresID,(PawnRow-1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)==-1 && buff[(PawnRow-1)*MAX_WIDTH+PawnCol].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[(PawnRow-1)*MAX_WIDTH+PawnCol].Symbol=='F'){CaughtFlag=1;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnRow--;
              }break;
      case 3: if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol+1),IPC_NOWAIT)==-1 && buff[PawnRow*MAX_WIDTH+(PawnCol+1)].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[PawnRow*MAX_WIDTH+(PawnCol+1)].Symbol=='F'){CaughtFlag=1;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnCol++;
              }break;
      case 4: if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol-1),IPC_NOWAIT)==-1 && buff[PawnRow*MAX_WIDTH+(PawnCol-1)].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[PawnRow*MAX_WIDTH+(PawnCol-1)].Symbol=='F'){CaughtFlag=1;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnCol--;
              }break;
      default: /*printf("INVALID DIRECTION\n");*/ return FAIL;break;
    }
  }else return FAIL;
  buff[PawnRow*MAX_WIDTH+PawnCol].Symbol='P';
  MAX_MOVES--;
  ScoreTable[PlrTurn-1].Moves++;
  if(CaughtFlag){
    init_Sem(SemID,2,TOT_PLAYERS+1);
    kill(MasterPID,SIGUSR1);
    ScoreTable[PlrTurn-1].Score+=buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points;
    buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
    buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;
    wait_Sem(SemID,2);
  }else{
    buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
    buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;
  }
  return SUCCESS;
}



struct Distance ShortestDistance(){
  /* Strategy 1, first move vertically to the same row, then move horizontally to the same column */
  /* Strategy 2, first move horizontally to the same column, then move vertically to the same row */
  /* Strategy 3, Move once vertically, then horizontally, then vertically (Basically, move diagonally) */

  int i,j;
  int Row, Col;
  int distance;
  struct Distance CurShortest;

  CurShortest.Distance=MAX_INT;


  /* Scan the entire board for flags */
  for(i=0;i<MAX_HEIGHT;i++){
    for(j=0;j<MAX_WIDTH;j++){
      if(buff[i*MAX_WIDTH+j].Symbol=='F'){
        distance=0;

        Row=PawnRow;
        Col=PawnCol;

        if(Row>i) distance+=Row-i;
        else if(Row<i) distance+=i-Row;
        else if(Row==i) distance+=0;

        if(Col>j) distance+=Col-j;
        else if(Col<j) distance+=j-Col;
        else if(Col==j) distance+=0;



          if(CurShortest.Distance>distance){
            CurShortest.Distance=distance;
            CurShortest.DestinationRow=i;
            CurShortest.DestinationCol=j;
            CurShortest.SourceRow=Row;
            CurShortest.SourceCol=Col;
          }

      } /* if(buff[i*MAX_WIDTH+j].Symbol=='F') */
    } /*for(j=0;j<MAX_WIDTH;j++)*/
  }/*for(i=0;i<MAX_HEIGHT;i++)*/

  #ifdef DEBUG
  printf("Pawn at R %d C %d is closest to Flag R %d C %d \n",PawnRow,PawnCol,CurShortest.DestinationRow,CurShortest.DestinationCol);
  #endif
  return CurShortest;
}

int WaitforMSG(){
  /*lock_Sem(MessageSemaphoreID,myTurn);*/ /* Pawn may read and write to buffer */

  while(waiting);
  /*printf("Pawn not waiting: %d\n", PawnPID);*/
  /*printf("Command is: %d\n", MessageBuffer->message.command);*/
  /*wait_Sem(MessageSemaphoreID,myTurn);*/
  Logn("Father is",getppid());
  Logn("Pawn turn",myTurn);
  return MessageBuffer->message.command;
}

void handle_signal(int signal){
	Logn("Signal", signal);
  /*printf("Signal caught: %d\n", signal);*/
  if(signal==SIGUSR2) waiting=0;
}
