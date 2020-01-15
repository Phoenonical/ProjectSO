#include "common.h"

int ConfigParser(char* Path, char* toGet){ /* I might've over-complicated it */
  int Row=1; /* I assume there's at least one Row, even if it's empty */
  char ch;
  char *str;
  int index=0;
  FILE * my_f;
  int myVal;
  my_f = fopen(Path,"r");
  if(my_f==NULL){printf("CONFIG ERROR: Error opening file\n"); exit(EXIT_FAILURE);}


  ch = fgetc(my_f);
  /* Read .conf file */
  while(ch!=EOF){ /* For each row */
    str=calloc(20,sizeof(char));
    myVal=0;
    index=0;
    if(ch=='#'){ /* Ignore the comment rows */
      while(ch!=EOF && ch!='\n'){ch=fgetc(my_f);}
    }else if(ch>=32 && ch<=126){ /* A row with at least 1 character */
        while(ch!=EOF && ch!='\n'){ /* Scan row */


          if(ch!='='){ /* The str has a limited amount of size, if the index goes over it, just ignore the rest of the letters */
            if(index<20){
              str[index]=ch;
              index++;
            }
          }else if(ch=='='){ /* The value after "=" is what we memorize */

            #ifdef DEBUG
            printf("%s\n", str);
            #endif

            ch = fgetc(my_f);
            if(ch=='\n' || ch==EOF){ /* If after "=" there's nothing, exit with a failure */
              printf("CONFIG ERROR: Parser error at row %d\n", Row); exit(EXIT_FAILURE);
            }
            while(ch!=EOF && ch!='\n'){ /* Scan number */
              if(ch>=48 && ch<=57){ /* If not a number, error */
                myVal=(myVal*10)+(ch-48);
              }else if(ch!=32){printf("CONFIG ERROR: Value is not a number at row %d\n",Row); exit(EXIT_FAILURE);}
              ch=fgetc(my_f);
            }

            #ifdef DEBUG
            printf("%d\n",myVal); /* Print row value */
            #endif

            if(strcmp(str,toGet)==0){ /* Check if the str in this row is equal toGet */
              fclose(my_f); /* If yes, close file and return the value of this row */
              return myVal;
            }
          }

          if(ch!='\n' && ch!=EOF){
            ch=fgetc(my_f);
            if(ch=='\n' || ch==EOF){ /* If there's a row with letters but no "=", exit with a failure */
              printf("CONFIG ERROR: Parser error at row %d\n",Row); exit(EXIT_FAILURE);
            }
          }


        }
      }else if(ch=='\n'){ /* Blank rows can be ignored */
        ch=fgetc(my_f);
        Row++; /* Increase row counter */
      }else if(ch!=EOF){
        ch=fgetc(my_f);
      }
      free(str);
    }
  fclose(my_f);
  return -1; /* If toGet was never found, return a -1 */
}

char* tostring(int Num){
  char *str;
  str = malloc(sizeof(char)*12);
  int i, rem, len=0, n;
  n = Num;

  while(n!=0){ /* Find number of digits in the number */
    len++;
    n/=10;
  }

  for(i=0;i<len;i++){ /* The Stringinification process */
    rem = Num % 10;
    Num = Num / 10;
    str[len - (i+1)] = rem + '0';
  }

  str[len] = '\0'; /* The \0 indicates the end of a string */

  return str;

}

int SharedMemID(int Key, int size){
  int i;
  int MAX_WIDTH=ConfigParser("./Settings.conf", "MAX_WIDTH");
  int MAX_HEIGHT=ConfigParser("./Settings.conf", "MAX_HEIGHT");
  int shmid = shmget(Key,size,0666|IPC_CREAT);
  void *attach;
  if(shmid == -1){
    printf("ERROR: Something went wrong with the creation of Shared memory\n");
    exit(EXIT_FAILURE);
  }
  return shmid;
}

void* AttachMem(int shmID){
  void *attach;
  attach = shmat(shmID,0,0); /* Attach shared memory segment to buff */
  if(attach == (void*) -1){
    perror("ERROR: Problem with attaching");
    exit(EXIT_FAILURE);
  }
return attach;
}

int MinMax(int Min, int Max){
	time_t t; /* Here's a genius idea, I'll use the time as my srand seed */
	srand((unsigned) time(&t)); /* Intializes random number generator */
	return rand() % (Max + 1 - Min) + Min; /* Returns a number between Min and Max */
}
