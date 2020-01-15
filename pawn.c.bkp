#include "common.h"
#include "semaphoreSO.h"

int MAX_WIDTH;
int MAX_HEIGHT;
int MAX_MOVES;
struct Cell *buff;
struct Message *MessageBuffer;
struct Scoreboard *ScoreTable;
int waiting=1;
int PawnCol;
int PawnRow;
int ParentPID;
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
    /*printf("Pawn waiting: %d\n", PawnPID);
    printf("Sending type %d\n", MessageBuffer->mtype);*/
    /*printf("Semaphore in pawn is %d\n", semctl(MessageSemaphoreID, myTurn, GETVAL));*/
    init_Sem(MessageSemaphoreID,myTurn,0);
    /*printf("Later Semaphore in pawn is %d\n", semctl(MessageSemaphoreID, myTurn, GETVAL));*/
  }

}


int Move(int Direction){
  if(MAX_MOVES>0){
    switch (Direction) {
      case 1: if(lock_Sem(ChessboardSemaphoresID,(PawnRow+1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)==-1 && buff[(PawnRow+1)*MAX_WIDTH+PawnCol].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[(PawnRow+1)*MAX_WIDTH+PawnCol].Symbol=='F'){kill(MasterPID,SIGUSR1); ScoreTable[PlrTurn-1].Score+=buff[(PawnRow+1)*MAX_WIDTH+PawnCol].Att.Points;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnRow++;
              }break;
      case 2: if(lock_Sem(ChessboardSemaphoresID,(PawnRow-1)*MAX_WIDTH+PawnCol,IPC_NOWAIT)==-1 && buff[(PawnRow-1)*MAX_WIDTH+PawnCol].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[(PawnRow-1)*MAX_WIDTH+PawnCol].Symbol=='F'){kill(MasterPID,SIGUSR1); ScoreTable[PlrTurn-1].Score+=buff[(PawnRow-1)*MAX_WIDTH+PawnCol].Att.Points;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnRow--;
              }break;
      case 3: if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol+1),IPC_NOWAIT)==-1 && buff[PawnRow*MAX_WIDTH+(PawnCol+1)].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[PawnRow*MAX_WIDTH+(PawnCol+1)].Symbol=='F'){kill(MasterPID,SIGUSR1); ScoreTable[PlrTurn-1].Score+=buff[PawnRow*MAX_WIDTH+(PawnCol+1)].Att.Points;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnCol++;
              }break;
      case 4: if(lock_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+(PawnCol-1),IPC_NOWAIT)==-1 && buff[PawnRow*MAX_WIDTH+(PawnCol-1)].Symbol!='F') return FAIL; /* If occupied, return "FAIL" */
              else{
                if(buff[PawnRow*MAX_WIDTH+(PawnCol-1)].Symbol=='F'){kill(MasterPID,SIGUSR1); ScoreTable[PlrTurn-1].Score+=buff[PawnRow*MAX_WIDTH+(PawnCol-1)].Att.Points;}
                buff[PawnRow*MAX_WIDTH+PawnCol].Symbol=' ';
                buff[PawnRow*MAX_WIDTH+PawnCol].Att.Points=0;
                release_Sem(ChessboardSemaphoresID,PawnRow*MAX_WIDTH+PawnCol); /* Release the occupied cell for other pawn/flag processes */
                PawnCol--;
              }break;
      default: /*printf("INVALID DIRECTION\n");*/ return FAIL;break;
    }
  }else return FAIL;
  buff[PawnRow*MAX_WIDTH+PawnCol].Symbol='P';
  buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDParent=ParentPID;
  buff[PawnRow*MAX_WIDTH+PawnCol].Att.pawn.PIDPawn=PawnPID;
  MAX_MOVES--;
  ScoreTable[PlrTurn-1].Moves++;
  return SUCCESS;
}



struct Distance ShortestDistance(){
  /* Strategy 1, first move vertically to the same row, then move horizontally to the same column */
  /* Strategy 2, first move horizontally to the same column, then move vertically to the same row */
  /* Strategy 3, Move once vertically, then horizontally, then vertically (Basically, move diagonally) */
  int i,j;
  char Impossible=0; /* Necessary to know if it's possible to reach the flag */
  /*int I,J;*/
  struct Distance VtoH;
  /*struct Distance HtoV;
  struct Distance Diag;*/
  struct Distance CurShortest;
  /*char Visited[MAX_HEIGHT][MAX_WIDTH]; *//* 0 for unvisited, 1 for visited */
  CurShortest.Distance=MAX_INT;
  /*for(i=0;i<MAX_HEIGHT;i++)
    for(j=0;j<MAX_WIDTH;j++)
      Visited[i][j]=0;*/

  /* Scan the entire board for flags */
  for(i=0;i<MAX_HEIGHT;i++){
    for(j=0;j<MAX_WIDTH;j++){
      if(buff[i*MAX_WIDTH+j].Symbol=='F'){

        /*for(I=0;I<MAX_HEIGHT;I++)
          for(J=0;J<MAX_WIDTH;J++)
            Visited[I][J]=0;*/ /* Initialize Visited matrix, obviously the pawn hasn't yet started visiting cells */

        int Row=PawnRow, Col=PawnCol;
        /*Visited[Row][Col]=1;*/
          /* Strategy 1 */

          VtoH.Distance=0;
          VtoH.DestinationRow=i;
          VtoH.DestinationCol=j;
          while((i!=Row || j!=Col) && Impossible==0){
          while(i!=Row && Impossible==0){ /* Loop until Pawn is on the same Row */
            if(i>Row){
              if(buff[(Row+1)*MAX_WIDTH+Col].Symbol==' ' || Row+1==i) Row++;
              else if(Col!=MAX_WIDTH-1)
                  if(buff[Row*MAX_WIDTH+(Col+1)].Symbol==' ') Col++; else;
              else if(Col!=0)
                  if(buff[Row*MAX_WIDTH+(Col-1)].Symbol==' ') Col--; else;
              else if(Row!=0)
                  if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' '){ char Moved = 0; /* Moved becomes 1 it has moved horizontally */
                while(Moved==0 && Impossible==0){
                  if(Row!=0){
                    Row--;
                    VtoH.Distance++;
                    if(Col!=MAX_WIDTH-1)
                      if(buff[Row*MAX_WIDTH+(Col+1)].Symbol==' '){Col++; Moved=1;}else;
                    else if(Col!=0)
                      if(buff[Row*MAX_WIDTH+(Col-1)].Symbol==' '){Col--; Moved=1;}
                    }else Impossible=1;
                } /* while(moved==0) */
              } /* if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' ') */
              else Impossible=1;
            }else if(i<Row){
              if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' ' || Row-1==i) Row--;
              else if(Col!=MAX_WIDTH-1)
                  if(buff[Row*MAX_WIDTH+(Col+1)].Symbol==' ') Col++; else;
              else if(Col!=0)
                  if(buff[Row*MAX_WIDTH+(Col-1)].Symbol==' ') Col--; else;
              else if(Row!=MAX_HEIGHT-1)
                  if(buff[(Row+1)*MAX_WIDTH+Col].Symbol==' '){ char Moved = 0; /* Moved becomes 1 it has moved horizontally */
                while(Moved==0 && Impossible==0){
                  if(Row!=MAX_HEIGHT-1){
                    Row++;
                    VtoH.Distance++;
                    if(Col!=MAX_WIDTH-1)
                      if(buff[Row*MAX_WIDTH+(Col+1)].Symbol==' '){Col++; Moved=1;}else;
                    else if(Col!=0)
                      if(buff[Row*MAX_WIDTH+(Col-1)].Symbol==' '){Col--; Moved=1;}
                    }else Impossible=1;
                } /* while(moved==0) */
              }else; /* if(buff[(Row+1)*MAX_WIDTH+Col].Symbol==' ') */
              else Impossible=1;
            }
          /*  Wait(1);*/
            /*Visited[Row][Col]=1;*/ /* over complicating stuff */
            VtoH.Distance=VtoH.Distance+1;
            if(VtoH.Distance>MAX_MOVES)
            Impossible=1;
            Logn("VtoH Distance",VtoH.Distance);
          } /* while(i!=Row) */

          while(j!=Col && Impossible==0){ /* Loop until Pawn is on the same Col */
            if(j>Col){
              if(buff[Row*MAX_WIDTH+(Col+1)].Symbol==' '|| Col+1==j) Col++;
              else if(Row!=MAX_HEIGHT-1)
                if(buff[(Row+1)*MAX_WIDTH+Col].Symbol==' ') Row++; else;
              else if(Row!=0)
                if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' ') Row--; else;
              else if(Col!=0)
                if(buff[Row*MAX_WIDTH+(Col-1)].Symbol==' '){ char Moved = 0; /* Moved becomes 1 it has moved vertically */
                while(Moved==0 && Impossible==0){
                    if(Col!=0){
                    Col--;
                    if(Row!=MAX_HEIGHT-1)
                      if(buff[(Row+1)*MAX_WIDTH+Col].Symbol==' '){Row++; Moved=1;}else;
                    else if(Row!=0)
                      if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' '){Row--; Moved=1;}
                    VtoH.Distance++;
                    }else Impossible=1;
                } /* while(moved==0) */
              }else; /* if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' ') */
              else Impossible=1;
            }
            else if(j<Col){
              if(buff[Row*MAX_WIDTH+(Col-1)].Symbol==' '|| Col-1==j) Col--;
              else if(Row!=MAX_HEIGHT-1)
                if(buff[(Row+1)*MAX_WIDTH+Col].Symbol==' ') Row++; else;
              else if(Row!=0)
                if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' ') Row--; else;
              else if(Col!=0)
                if(buff[Row*MAX_WIDTH+(Col+1)].Symbol==' '){ char Moved = 0; /* Moved becomes 1 it has moved vertically */
                while(Moved==0 && Impossible==0){
                    if(Col!=MAX_WIDTH-1){
                    Col++;
                    if(Row!=MAX_HEIGHT-1)
                      if(buff[(Row+1)*MAX_WIDTH+Col].Symbol==' '){Row++; Moved=1;}else;
                    else if(Row!=0)
                      if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' '){Row--; Moved=1;}
                    VtoH.Distance++;
                    }else Impossible=1;
                }VtoH.Distance--; /* while(moved==0) */
              }else; /* if(buff[(Row-1)*MAX_WIDTH+Col].Symbol==' ') */
              else Impossible=1;
            }
            /*Wait(1);*/
            VtoH.Distance=VtoH.Distance+1;
            if(VtoH.Distance>MAX_MOVES)
            Impossible=1;
            Logn("VtoH Distance",VtoH.Distance);
          } /* while(i!=Col) */
          }
          /*Logn("VtoH Distance",VtoH.Distance);
          Logn("VtoH Row",VtoH.Row);
          Logn("VtoH Col",VtoH.Col);*/
          if(CurShortest.Distance>VtoH.Distance && Impossible==0){
            CurShortest.Distance=VtoH.Distance;
            CurShortest.DestinationRow=VtoH.DestinationRow;
            CurShortest.DestinationCol=VtoH.DestinationCol;
            CurShortest.SourceRow=PawnRow;
            CurShortest.SourceCol=PawnCol;
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
/*  printf("Pawn not waiting: %d\n", PawnPID);
  printf("Command is: %d\n", MessageBuffer->message.command);*/
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
