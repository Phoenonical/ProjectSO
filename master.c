#include "common.h"
#include "semaphoreSO.h"

/* Debugging messages, WARNING: It's a really big mess */
#define DEBUG 0
/* 0 for no border chessboard */
#define BORDERS 0
/* Print semaphore board */
#define DEBUGCHESS 0
/* Debug messaing and debug sleep() */
#define DEBUG 0

int NumFlags;

void handle_signal(int signal);
void BuildPlayingField();
void Color(char Symbol, int PIDPlayer);
void PlaceFlags();
void SetupPlayers(int i);
void Terminate();
void DistributePoints(int *Points, int N);
int ScanforFlags();

int TOT_PLAYERS;
int TOT_PAWNS;
int TOT_POINTS;
int MAX_WIDTH;
int MAX_HEIGHT;
int MIN_FLAGS;
int MAX_FLAGS;
int MIN_HOLD_NSEC;
int MAX_TIME;

int ROUND;

struct Cell *buff;
struct Scoreboard *ScoreTable;
int *PlayerPIDs;
int shmID;
int MessageQueueID;
int ChessboardSemaphoresID;
int SemID;
int ScoreTableID;
int Signaled=0;

int main(){
	int i, j;
	struct sigaction sa; /* Structure for later signal catching */
	sigset_t  my_mask;
	struct timespec toWait;
	ROUND=1;
	toWait.tv_sec=0;
	toWait.tv_sec=50000000;
  ConfigParser("./Settings.conf", "Supercalifragilisticexpialidocious"); /* Check if the .conf is set correctly */
	TOT_PLAYERS=ConfigParser("./Settings.conf", "TOT_PLAYERS");
	TOT_PAWNS=ConfigParser("./Settings.conf", "TOT_PAWNS");
	TOT_POINTS=ConfigParser("./Settings.conf", "TOT_POINTS");
	MAX_WIDTH=ConfigParser("./Settings.conf", "MAX_WIDTH");
	MAX_HEIGHT=ConfigParser("./Settings.conf", "MAX_HEIGHT");
	MAX_FLAGS=ConfigParser("./Settings.conf", "MAX_FLAGS");
	MIN_FLAGS=ConfigParser("./Settings.conf", "MIN_FLAGS");
	MIN_HOLD_NSEC=ConfigParser("./Settings.conf", "MIN_HOLD_NSEC");
	MAX_TIME=ConfigParser("./Settings.conf", "MAX_TIME");

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
	sigaction(SIGCHLD, &sa, NULL);

	sigaction(SIGUSR1, &sa, NULL);

	/*signal(SIGUSR1,handle_signal);*/
	signal(SIGALRM,handle_signal);

	/*for(i = 0; i<NSIG; i++){
		if(sigaction(i, &sa, NULL)==-1)
			Logn("Cannot set a user-defined handler for Signal",i);
	}*/



	shmID = SharedMemID(ftok("./",70),sizeof(struct Cell)*MAX_HEIGHT*MAX_WIDTH);

	buff = AttachMem(shmID);

	ChessboardSemaphoresID = Semaphore(ftok("./master.c",64),MAX_HEIGHT*MAX_WIDTH);

	ScoreTableID = SharedMemID(ftok("./master",1),sizeof(struct Scoreboard)*TOT_PLAYERS);
	ScoreTable = AttachMem(ScoreTableID);
	for(i=0;i<TOT_PLAYERS;i++){ /* Initialize the Leaderboard */
		ScoreTable[i].Score=0;
		ScoreTable[i].Moves=0;
	}

	for(i=0;i<MAX_HEIGHT;i++){
		for(j=0;j<MAX_WIDTH;j++){
			buff[i*MAX_WIDTH+j].Symbol=' '; /* Initialize the playing field */
			buff[i*MAX_WIDTH+j].Att.Points=0;
			init_Sem(ChessboardSemaphoresID,i*MAX_WIDTH+j,1);
		}
	}

	SemID = Semaphore(ftok("./player.c",68), 7);
  init_Sem(SemID, 0, 0); /* Players */
	init_Sem(SemID, 1, 0);
	init_Sem(SemID, 2, 1);
	init_Sem(SemID, 3, 1); /* Game start */
	init_Sem(SemID, 4, 1);
	init_Sem(SemID, 5, 1); /* Mutual access to "FetchaTarget()" in Pawn */
	init_Sem(SemID, 6, 1); /* Mutual access to scores */

	Logn("SemID is",SemID);

	/*lock_Sem(SemID,0); *//* Lock the semaphore until all players have been generated */
	PlayerPIDs = malloc(sizeof(int)*TOT_PLAYERS);
	for(i=0;i<TOT_PLAYERS;i++){SetupPlayers(i); /*printf("PlayerPIDs: %d\n",PlayerPIDs[i]);*/ }/* Create all player processes */
	Wait(1);
	release_Sem(SemID,0); /* Allows the players to place down their pawns */

  for(i=0;i<TOT_PAWNS;i++){
	while (!compare_Sem(SemID,0,TOT_PLAYERS+1)); /* Wait until all players are done */
	/*BuildPlayingField();*/
	/*lock_Sem(SemID,0);*/ /* Pawns may not be placed until semaphore reset has been locked */
	init_Sem(SemID,0,0);
	Log("Semaphore has been reset");
	/*sleep(1);*/
	release_Sem(SemID,0); /* Players may place pawns again */
	}
	Log("Done placing, the game begins");
	/* Re-using the old semaphores */
	init_Sem(SemID, 0, 1); /* Make players take turns */
	init_Sem(SemID, 1, 1);
	init_Sem(SemID, 2, 0);

	toWait.tv_sec=0;
  toWait.tv_nsec=MIN_HOLD_NSEC;
	PlaceFlags(); /* Place down flags on the board */
	sleep(1); /* Make sure everyone is infact done */
	init_Sem(SemID, 4, 0);
	while(!compare_Sem(SemID,2,TOT_PLAYERS));
	BuildPlayingField();
	init_Sem(SemID, 3, 0); /* Releasing the beasts */
	/* So it begins - Thèoden Ednew, King of Rohan */

	alarm(MAX_TIME);

	while(1){
		init_Sem(SemID,1,1);
		/*ScanThrough();*/
		NumFlags=ScanforFlags();

		if(NumFlags==0){
			/*init_Sem(SemID,1,1);*/
			alarm(0);
			init_Sem(SemID,2,0);
			init_Sem(SemID,0,1);

			PlaceFlags();
			NumFlags=ScanforFlags();
			ROUND++;
			for(i=0;i<TOT_PLAYERS;i++) kill(PlayerPIDs[i], SIGUSR1);
			init_Sem(SemID,0,0);
			while(!compare_Sem(SemID,2,TOT_PLAYERS));
			/*init_Sem(SemID,1,0);*/
			alarm(MAX_TIME);
		}

		 BuildPlayingField();
		 init_Sem(SemID,1,0);
		 nanosleep(&toWait,NULL);




	}


	Terminate(); /* Kill off any players and deallocate SHMs and semaphores */

}

void Terminate(){
	int i;
	int status;
	int winner, winnerScore=0, winnerMoves;
	char ch;
	for(i=0;i<TOT_PLAYERS;i++) kill(PlayerPIDs[i], SIGUSR2);

	for(i=0;i<TOT_PLAYERS;i++){
		if(winnerScore<ScoreTable[i].Score){
			winner=i; winnerScore=ScoreTable[i].Score; winnerMoves=ScoreTable[i].Moves;
		}
	}

	BuildPlayingField();
	printf("\nWinner winner chicken dinner ----- "); Color('Q',PlayerPIDs[winner]);
	printf("Player%d\033[0m ----- Score: %d ----- Moves: %d\n\n",winner+1,winnerScore,winnerMoves);

	printf("Press any key to exit....\n");
	scanf("%c",&ch);


	for(i=0;i<TOT_PLAYERS;i++) kill(PlayerPIDs[i], SIGINT);
	while(wait(&status) != -1);
	shmdt(buff); /* Deattach shared memory segment */
	shmctl(shmID,IPC_RMID,NULL); /* Deallocate a shared memory */
	shmctl(ScoreTableID,IPC_RMID,NULL);
	remove_Sem(SemID);
	remove_Sem(ChessboardSemaphoresID);
	exit(EXIT_SUCCESS);
}

void SetupPlayers(int i){
	char Num[10];
	char *(args)[3];
	sprintf(Num,"%d",i+1);
	/*char *args[] = {"./player", Num, NULL};*/
	args[0]="./player";
	args[1]=Num;
	args[2]=NULL;
	switch(PlayerPIDs[i]=fork()){
	case -1: printf("ERROR: Error creating fork\n"); exit(EXIT_FAILURE);break;
	case 0:  execve("./player", args, NULL); exit(EXIT_FAILURE);break;
	/*case 0: execvp(args[0],args); exit(EXIT_FAILURE);*/
	default: Wait(1); break;
	}
}

int ScanforFlags(){
	int counter;
	int i,j;
	counter=0;
	for(i=0;i<MAX_HEIGHT;i++){
		for(j=0;j<MAX_WIDTH;j++){
			if(buff[i*MAX_WIDTH+j].Symbol=='F')
			counter++;
		}
	}
	return counter;
}

void BuildPlayingField(){
	int i, j;

	printf("-----ROUND %d-----\n\n", ROUND);
	for(i=0;i<TOT_PLAYERS;i++){
		Color('Q',PlayerPIDs[i]);
		printf("Player%d ",i+1); printf("\033[0m");
		printf("----- Score: %d ----- Used Moves: %d\n", ScoreTable[i].Score,ScoreTable[i].Moves);
	}
	printf("Remaining ");
	Color('E',0);
	printf("Flags");
	printf("\033[0m");
	printf(": %d\n\n", NumFlags);

	for(i=0;i<MAX_HEIGHT;i++){
		for(j=0;j<MAX_WIDTH;j++){
			Color(buff[i*MAX_WIDTH+j].Symbol,buff[i*MAX_WIDTH+j].Att.pawn.PIDParent);
		 }
		printf("\n");
	}
	printf("\n");


	 #if DEBUGCHESS
		for(i=0;i<MAX_HEIGHT;i++){
			for(j=0;j<MAX_WIDTH;j++){
				printf("%d",semctl(ChessboardSemaphoresID,i*MAX_WIDTH+j,GETVAL));
			 }
			printf("\n");
		}
		printf("\n");
	#endif
}

void Color(char Symbol, int PIDPlayer){
	int i, ColorNumber, LetterAdd=0;
	char Letter = 'A';
	if(Symbol==' '){printf(".");}
	else if(Symbol=='F'){ printf("\033[1;31mF\033[0m");} /* Flag is bold Red */
	else if(Symbol=='E'){ printf("\033[1;31m");} /* Flag text is bold Red */
	else if(Symbol=='P'){ /* Pawn color and symbol */
		for(i=0;i<TOT_PLAYERS;i++){
			if(PIDPlayer == PlayerPIDs[i]){ ColorNumber=i%10; Letter=Letter+LetterAdd;}
			if((i+1)%10==0)LetterAdd++;
			if(LetterAdd==5)LetterAdd++; /* Skip the letter F */
		}
		switch(ColorNumber){
			case 0: printf("\033[0;32m%c",Letter);break;
			case 1: printf("\033[0;33m%c",Letter);break;
			case 2: printf("\033[0;34m%c",Letter);break;
			case 3: printf("\033[0;35m%c",Letter);break;
			case 4: printf("\033[0;36m%c",Letter);break;
			case 5: printf("\033[1;32m%c",Letter);break;
			case 6: printf("\033[1;33m%c",Letter);break;
			case 7: printf("\033[1;34m%c",Letter);break;
			case 8: printf("\033[1;35m%c",Letter);break;
			case 9: printf("\033[1;36m%c",Letter);break;
			default: /* No color */ break;
		}
		printf("\033[0m");
		/*printf("%c",Symbol); printf("\033[0m");*/
	}else if(Symbol=='Q'){ /* Player Text color */
		for(i=0;i<TOT_PLAYERS;i++){
			if(PIDPlayer == PlayerPIDs[i]) ColorNumber=i%10;
		}
		switch(ColorNumber){
			case 0: printf("\033[0;32m");break;
			case 1: printf("\033[0;33m");break;
			case 2: printf("\033[0;34m");break;
			case 3: printf("\033[0;35m");break;
			case 4: printf("\033[0;36m");break;
			case 5: printf("\033[1;32m");break;
			case 6: printf("\033[1;33m");break;
			case 7: printf("\033[1;34m");break;
			case 8: printf("\033[1;35m");break;
			case 9: printf("\033[1;36m");break;
			default: /* No color */ break;
		}
		/*printf("%c",Symbol); printf("\033[0m");*/
	}
}


void PlaceFlags(){ /* Randomly place flags in our field */
	int i, j;
	int *Points;
	int randRow;
	int randCol;
	int TOTAL;
	time_t t; /* Here's a genius idea, I'll use the time as my srand seed */
	srand((unsigned) time(&t)); /* Intializes random number generator */

	NumFlags=MinMax(MIN_FLAGS,MAX_FLAGS);
	Points=malloc(sizeof(int)*NumFlags);
	Logn("Number of flags", NumFlags);

	/* Distrubute points randomly among flags, Assuming that TOT_POINTS>=NumFlags is always true */
	TOTAL=TOT_POINTS;
	for(i=0;i<NumFlags;i++){Points[i]=1;} /* Each flag has a minimal value of 1 */
	TOTAL=TOTAL-NumFlags;

	while(TOTAL>0){
		i=rand() % NumFlags;
		Points[i]++;
		TOTAL--;
	}

	/*DistributePoints(Points, NumFlags);*/

	for(i=0;i<MAX_HEIGHT;i++)
		for(j=0;j<MAX_WIDTH;j++)
			if(buff[i*MAX_WIDTH+j].Symbol=='F'){
				buff[i*MAX_WIDTH+j].Symbol=' ';
				buff[i*MAX_WIDTH+j].Att.Points=0;
			 } /* Remove all old flags */

	for(i=0;i<NumFlags;i++){
		Logn("Check for flag",i);
		do{
			randRow = (rand() % MAX_HEIGHT); /* Random number from 0 to MAX_HEIGHT - 1 */
			randCol = (rand() % MAX_WIDTH); /* Random number from 0 to MAX_WIDTH - 1 */
			Logn("Found",buff[randRow*MAX_WIDTH+randCol].Symbol);
		}while(buff[randRow*MAX_WIDTH+randCol].Symbol!=' ');
		/*}while(buff[randRow*MAX_WIDTH+randCol].Symbol!=' ');*/ /* if the cell is ALREADY occupied, we re-randomize our row and column numbers */

		buff[randRow*MAX_WIDTH+randCol].Symbol = 'F';
		buff[randRow*MAX_WIDTH+randCol].Att.Points = Points[i];

		Logn("Placed flag",i);

	}
	free(Points);
}


void handle_signal(int signal){
	int status;
	int i;
	Logn("Signal", signal);
	/*printf("Caught signal in master %d\n", signal);*/
	if(signal==SIGINT || signal==SIGCHLD || signal==SIGSEGV || signal==SIGILL || signal==SIGFPE || signal==SIGKILL){
		if(signal==SIGINT) printf("\nInterrupting game.....\n");
		for(i=0;i<TOT_PLAYERS;i++) kill(PlayerPIDs[i], 2);
		i=0;
		while(PlayerPIDs[i] = wait(&status) != -1){i++;}
		shmctl(shmID,IPC_RMID,NULL);
		shmctl(ScoreTableID,IPC_RMID,NULL);
		remove_Sem(SemID);
		remove_Sem(ChessboardSemaphoresID);
		exit(EXIT_SUCCESS);
	}



	if(signal==SIGALRM){
		if(NumFlags>0){
		Terminate();
		}
	}

}
