/*
 * File: timeout_timer_thread.c
 *
 * Compile: gcc timeout_timer_thread.c -o timeout_timer_thread -lrt
 *
 * Author: Sergio Romero Montiel <sromero@uma.es>
 *
 * Created on May 20th, 2016
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct {
  pid_t   pgid;
	// estruturas de datos adicionales para manejar el temporizador
  timer_t timerid;		// para poder destruirlo cuando no haga falta
  struct itimerspec its;	// especificacion de tiempos del temporizador
  struct sigevent   sev;	// thread que crea el temposizador
} elem;	// o bien, poner el puntero al timer en la lista de procesos


// el manejador, encargado de matar al grupo de procesos indicado
void killer(union sigval arg)
{
  elem *payload = arg.sival_ptr;

  killpg(payload->pgid,SIGTERM);	// envio TERM al grupo timed-out-ed!
  timer_delete(payload->timerid);	// destruyo el temporizador

  free(payload);  // si es un elemento de la lista de procesos, no se elimina todavia!!!
}


int main(int argc, char *argv[])
{
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
    	// theOne->timerid todavia no lo conocemos, lo escribe timer_create()

      // Crear el timer
      theOne->sev.sigev_notify = SIGEV_THREAD;
      theOne->sev.sigev_notify_function = killer;
      theOne->sev.sigev_notify_attributes = NULL;
      theOne->sev.sigev_value.sival_ptr = (void *)theOne; // it points to me!
      if (timer_create(CLOCK_REALTIME, &theOne->sev, &theOne->timerid) == -1)
        errExit("timer_create");

      // Arranca el timer
      theOne->its.it_value.tv_sec = when;
      theOne->its.it_value.tv_nsec = 0;
      theOne->its.it_interval = theOne->its.it_value;
      if (timer_settime(theOne->timerid, 0, &theOne->its, NULL) == -1)
        errExit("timer_settime");
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
