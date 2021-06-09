/*
 * File: timeout_pthread_sleep.c
 *
 * Compile: gcc timeout_pthread_sleep.c -o timeout_pthread_sleep -pthread
 *
 * Author: Sergio Romero Montiel <sromero@uma.es>
 *
 * Created on May 20th, 2016
 */

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct {
  pid_t   pgid;
	// datos adicionales para sleep()
  int time;
} elem;	// o bien, poner el puntero al timer en la lista de procesos


// el manejador, encargado de matar al grupo de procesos indicado
static void* killer(void *arg)
{
  elem *payload = arg;

  sleep(payload->time);
  killpg(payload->pgid,SIGTERM);	// envio TERM al grupo timed-out-ed!
}


int main(int argc, char *argv[])
{
  pthread_t thid;
  elem *theOne;		// usa esto o el elemento de la lista de procesos
  int pgid;
  int when;
  int read;

  while(1)
  {
    printf("Dar proceso y tiempo:\n");
    read = scanf("%d %d",&pgid,&when);

    switch (read)
    {
    case 2:
      // Incluir info del temporizador en la estructura de tipo elem
      theOne = (elem *)malloc(sizeof(elem));
      theOne->pgid = pgid; // a quien vamos a matar
      theOne->time = when; // y cuando lo vamos a hacer

      // abre el thread detached (para no hace join)
      if (0 == pthread_create(&thid,NULL,killer,(void *)theOne))
        pthread_detach(thid);
      else
        fprintf(stderr,"pthread_create: error\n");
      break;
    case -1:
      if (errno==EINTR) perror("scanf");
      else errExit("scanf");
      break;
    default: /* 0 o 1 dato, falta pgid o when */
      scanf("%*[^\n]%*c"); // clear stdin
      fprintf(stderr,"Faltó el numero de proceso o el tiempo de vida\n");
      break;
    }
  }

  exit(EXIT_SUCCESS);
}
