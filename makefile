
CC=gcc
CFLAGS=-std=c89 -pedantic


output: master.c player.c pawn.c common.o semaphoreSO.o
	$(CC) master.c common.o semaphoreSO.o $(CFLAGS) -o master
	$(CC) player.c common.o semaphoreSO.o $(CFLAGS) -o player
	$(CC) pawn.c common.o semaphoreSO.o $(CFLAGS) -o pawn

common.o: common.c common.h
	$(CC) common.c -c

semaphoreSO.o: semaphoreSO.c semaphoreSO.h
	$(CC) semaphoreSO.c -c
