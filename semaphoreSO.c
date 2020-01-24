#include"semaphoreSO.h"

int Semaphore(key_t key, int size){
  int semid = semget(key, size, 0666|IPC_CREAT);
/*  int semid = semget(key, size, 0600|IPC_CREAT|IPC_EXCL);*/
  if(semid == -1){
    printf("ERROR: Error creating semaphore\n");
    exit(EXIT_FAILURE);
  }
  return semid;
}

int init_Sem(int semID, int semNum, int val){
    union semun semVal; /* semaphore value, for semctl(). */
    semVal.val = val;
    return semctl(semID, semNum, SETVAL, semVal);
}

int compare_Sem(int semID, int semNum, int valtocheck){
  int val = semctl(semID, semNum, GETVAL); /* semctl returns the value of semaphore */
  /*printf("val: %d\n", val);*/
  if(val == -1){printf("ERROR: Error checking value of semaphore\n"); exit(EXIT_FAILURE);}
  return (val == valtocheck); /* If false return 0, if true return 1 */
}

int lock_Sem(int semID, int semNum, int flag){ /* Processes may only continue if semaphore's value is greater than 0 */
  struct sembuf sops; /* A structure the semop(...) HAS to have.*/
  sops.sem_num = semNum; /* The position of our semaphore in the array */
  sops.sem_op = -1; /* Decrements value by 1 */
  sops.sem_flg = flag;
  return semop(semID, &sops, 1); /* Processes pause here and may only continue if semaphore's value is greater than 0 */
}

int release_Sem(int semID, int semNum){
  struct sembuf sops; /* A structure the semop(...) HAS to have. */
  sops.sem_num = semNum; /* The position of our semaphore in the array */
  sops.sem_op = 1; /* Increments value by 1 */
  sops.sem_flg = 0;
  return semop(semID, &sops, 1);
}

int wait_Sem(int semID, int semNum){
  struct sembuf sops; /* A structure the semop(...) HAS to have. */
  sops.sem_num = semNum; /* The position of our semaphore in the array */
  sops.sem_op = 0; /* Wait for value to equal 0 */
  sops.sem_flg = 0;
  return semop(semID, &sops, 1);
}

void remove_Sem(int semID){semctl(semID,0,IPC_RMID);}
