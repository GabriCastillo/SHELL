/*
 * File: timeout_timer_signal.c
 *
 * Compile: gcc timeout_timer_signal.c -o timeout_timer_signal -lrt
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
  struct sigevent   sev;	// el evento que envia el temporizador
} elem;	// o bien, poner el puntero al timer en la lista de procesos


// el manejador, encargado de matar al grupo de procesos indicado
static void handler(int sig, siginfo_t *si, void *uc)
{
  elem *payload;

  payload = si->si_value.sival_ptr;

  killpg(payload->pgid,SIGTERM);	// envio TERM al grupo timed-out-ed!
  timer_delete(payload->timerid);	// destruyo el temporizador

  free(payload);  // si es un elemento de la lista de procesos, no se elimina todavia!!!
}


int main(int argc, char *argv[])
{
  struct sigaction sa;	// la accion que realiza el receptor
  elem *theOne;		// usa esto o el elemento de la lista de procesos
  int pgid;
  int when;
  int read;

  // Establecer el manejador de SIGNAL SIGRTMIN
  printf("Estableciendo el handler de SIGNAL %d\n", SIGRTMIN);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGRTMIN, &sa, NULL) == -1)
    errExit("sigaction");

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
      theOne->sev.sigev_notify = SIGEV_SIGNAL;
      theOne->sev.sigev_signo = SIGRTMIN;
      theOne->sev.sigev_value.sival_ptr = (void *)theOne; // it points to me!
      if (timer_create(CLOCK_REALTIME, &theOne->sev, &theOne->timerid) == -1)
        errExit("timer_create");

      // Arranca el timer
      theOne->its.it_value.tv_sec = when;
      theOne->its.it_value.tv_nsec = 0;
      theOne->its.it_interval.tv_sec = theOne->its.it_value.tv_sec;
      theOne->its.it_interval.tv_nsec = theOne->its.it_value.tv_nsec;
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
