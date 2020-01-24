#ifndef _SEMAPHORESO_H_
#define _SEMAPHORESO_H_

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<sys/ipc.h>


/* The following union must be defined as required by the semctl man page */
union semun {
	int              val;    /* Value for SETVAL */
	struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  /* Array for GETALL, SETALL */
	struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
};

/* Methods for semaphores */
int Semaphore(key_t key, int size); /* Create/Get a semaphore */

int init_Sem(int semID, int semNum, int val); /* Initialize semaphore(s) to a specific value */

/*
 * semID = The ID of the semaphore IPC object
 * semNum = The position of the semaphore in the array
 * flag = The wanted flags to include
 */

int compare_Sem(int semID, int semNum, int valtocompare); /* Compare a value to semaphore value */

int lock_Sem(int semID, int semNum, int flag); /* Lock the semaphore */

int release_Sem(int semID, int semNum); /* Release the semaphore */

int wait_Sem(int semID, int semNum); /* Wait for semaphore */

void remove_Sem(int semID); /* Deallocate the semaphore from memory */

#endif
