/**
UNIX Shell Project
https://github.com/GabriCastillo/SHELL.git
Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h" // remember to compile with module job_control.c

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */
#include <string.h>

//timeout

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define errExit(msg)        \
	do                      \
	{                       \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	} while (0)

typedef struct
{
	pid_t pgid;
	// estruturas de datos adicionales para manejar el temporizador
	timer_t timerid;	   // para poder destruirlo cuando no haga falta
	struct itimerspec its; // especificacion de tiempos del temporizador
	struct sigevent sev;   // el evento que envia el temporizador
} elem;					   // o bien, poner el puntero al timer en la lista de procesos

//timeout

//colores

#define ROJO "\x1b[31;1;1m"
#define NEGRO "\x1b[0m"
#define VERDE "\x1b[32;1;1m"
#define AZUL "\x1b[34;1;1m"
#define CIAN "\x1b[36;1;1m"
#define MARRON "\x1b[33;1;1m"
#define PURPURA "\x1b[35;1;1m"

//colores

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------
job *lista; //*tareas

static void handler(int sig, siginfo_t *si, void *uc)
{
	elem *payload;

	payload = si->si_value.sival_ptr;

	killpg(payload->pgid, SIGTERM); // envio TERM al grupo timed-out-ed!
	timer_delete(payload->timerid); // destruyo el temporizador

	free(payload); // si es un elemento de la lista de procesos, no se elimina todavia!!!
}

void mysigchild(int s)
{
	printf("\nSe recibió SIGCHLD\n"); // NO SE ACONSEJA usar printf en una señal

	// MANEJADOR DE SIGCHLD ->
	//   recorrer todos los jobs en bg y suspendidos a ver
	//   qué les ha pasado???:
	//     SI MUERTOS-> quitar de la lista
	//     SI CAMBIAN DE ESTADO -> cambiar el job correspondiente

	int i, status, info, pid_wait;
	enum status status_res; /* status processed by analyze_status() */
	job *jb;

	for (int i = 1; i <= list_size(lista); i++)
	{
		jb = get_item_bypos(lista, i);

		pid_wait = waitpid(jb->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED);

		if (pid_wait == jb->pgid)
		{
			status_res = analyze_status(status, &info);
			// qué puede ocurrir?
			// - EXITED
			// - SIGNALED
			// - SUSPENDED
			// - CONTINUED

			printf("\n[SIGCHLD] Wait realizado para trabajo en background: %s, pid=%i\n", jb->command, pid_wait);
			/* Actuar según los posibles casos reportados por status
                Al menos hay que considerar EXITED, SIGNALED, y SUSPENDED
                En este ejemplo sólo se consideran los dos primeros */

			if ((status_res == SIGNALED) | (status_res == EXITED))
			{
				delete_job(lista, jb);

				// [1]->[2]->[3]....

				i--; /* Ojo! El siguiente ha ocupado la posicion de este en la lista */
			}

			if (status_res == CONTINUED)
			{
				jb->state = BACKGROUND;
			} // sOLO CAMBIAR DE ESTADO

			if (status_res == SUSPENDED)
			{
				jb->state = STOPPED;
			} // SOLO CAMBIAR DE ESTADO

			print_job_list(lista); //debugging
		}
	}

	return;
}

int main(void)
{
	printf(ROJO "Demostración %sde %scolor" NEGRO, VERDE, AZUL);
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;				/* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE / 2];	/* command line (of 256) has max of 128 arguments */

	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;				/* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	lista = new_list("milista");

	int fg = 0;
	int puntero_historial = 0;
	char historial[MAX_LINE][MAX_LINE];
	int time, timeout;
	//////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////

	job *nuevo; // para meter un nuevo trabajo en la lista

	terminal_signals(SIG_IGN); // Ya no se muere con ^C ni se suspende con ^Z

	signal(SIGCHLD, mysigchild);

	while (1) /* Program terminates normally inside get_command() after ^D is typed*/
	{
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background); /* get next command */

		if (args[0] == NULL)
			continue; // if empty command

		strcpy(historial[puntero_historial], args[0]);
		puntero_historial++;

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/
		////////////////////////////////////////////////////////////////
		if (!strncmp(args[0], "cd", MAX_LINE))
		{
			if (args[1] != NULL)
			{
				if (chdir(args[1]) == -1)
				{
					printf("\nERROR. No existe el directorio %s\n", args[1]);
				}
			}
			else
			{
				chdir(getenv("HOME"));
			}
			continue;
		}
		else if (!strncmp(args[0], "jobs", MAX_LINE))
		{
			block_SIGCHLD();
			if (list_size(lista) > 0)
			{
				print_job_list(lista);
			}
			else
			{
				printf("\nLa lista esta vacia\n");
			}
			unblock_SIGCHLD();
			continue;
		}
		else if (!strncmp(args[0], "fg", MAX_LINE))
		{

			block_SIGCHLD();
			job *jl;
			if (empty_list(nuevo))
			{
				printf("No se encuentra ningún proceso en ejecución\n");
			}
			else
			{

				if (args[1] == NULL)
				{
					jl = get_item_bypos(lista, 1);
				}
				else
				{
					jl = get_item_bypos(lista, atoi(args[1]));
				}

				if (jl == NULL)
				{
					printf("\nERROR. No se encuentra en la lista\n");
				}
				else
				{
					if (jl->state != FOREGROUND)
					{							//si no estaba en fg
						jl->state = FOREGROUND; //lo ponemos en fg
						set_terminal(jl->pgid); //cedemos la terminal

						killpg(jl->pgid, SIGCONT); //envia señal al grupo de procesos para que sigan
						waitpid(jl->pgid, &status, WUNTRACED);

						set_terminal(getpid()); //devolvemos terminal
						status_res = analyze_status(status, &info);
						if (status_res == SUSPENDED)
						{
							jl->state = STOPPED;
						}
						else
						{
							delete_job(lista, jl);
						}

						printf("\nForeground pid: %d, command: %s, %s, info:%d\n", pid_fork, args[0], status_strings[status_res], info);
					}
					else
					{

						printf("\nEl proceso '%s' (pid: %i) no estaba suspendido ni en background\n", jl->command, jl->pgid);
					}
				}
			}
			unblock_SIGCHLD();
			continue;
		}
		else if (!strncmp(args[0], "bg", MAX_LINE))
		{

			job *jl;
			int id;
			if (empty_list(nuevo))
			{
				printf("No se encuentra ningún proceso en ejecución\n");
			}
			else
			{
				if (args[1] == NULL)
				{
					id = 1;
				}
				else
				{
					id = atoi(args[1]);
				}

				block_SIGCHLD();
				jl = get_item_bypos(lista, id);

				if (jl == NULL)
				{
					printf("\nERROR. No se encuentra en la lista\n");
				}
				else
				{

					jl->state = BACKGROUND;
					printf("\nBackground pid: %id, command: %s\n", jl->pgid, jl->command);
					killpg(jl->pgid, SIGCONT);
				}
			}
			unblock_SIGCHLD();
			continue;
		}
		else if (!strncmp(args[0], "history", MAX_LINE))
		{

			for (int k = 0; k < puntero_historial; k++)
			{
				printf("\n");
				printf(historial[k], "\n");
			}
			continue;
		}
		else if (!strncmp(args[0], "time-out", MAX_LINE))
		{
			struct sigaction sa; // la accion que realiza el receptor
			elem *theOne;		 // usa esto o el elemento de la lista de procesos
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

			pid_fork = fork();
			new_process_group(pid_fork);
			nuevo = new_job(pid_fork, inputBuffer, BACKGROUND); //Nuevo nodo job

			// Meterlo en la lista
			// Sección crítica libre de SIGCHLD
			block_SIGCHLD(); // mask -> la señal si entra se queda pendiente
			add_job(lista, nuevo);
			unblock_SIGCHLD();

			if (args[1] != NULL && args[2] != NULL)
			{
				pgid = nuevo->pgid;
				when = atoi(args[1]);

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
			}
		}
		/////////////////////////////////////////////////////////////////////

		pid_fork = fork();

		if (pid_fork > 0)
		{
			// PADRE (shell)

			new_process_group(pid_fork); // hijo en un grupo nuevo independiente
										 // hijo es el lider de su grupo gpid == pid

			if (background)
			{
				// poner el hijo en bg
				printf("\nbackground\n");

				// meter en lista

				// Nuevo nodo job -> nuevo job BACKGROUND
				nuevo = new_job(pid_fork, inputBuffer, BACKGROUND); //Nuevo nodo job

				// Meterlo en la lista
				// Sección crítica libre de SIGCHLD
				block_SIGCHLD(); // mask -> la señal si entra se queda pendiente
				add_job(lista, nuevo);
				unblock_SIGCHLD(); // unmask -> entran sigchild pendientes
			}
			else
			{
				// poner el hijo en fg

				// ceder el terminal al hijo
				set_terminal(pid_fork);

				// el shell se tiene que quedar esperando:
				// - terminación (exit/signal)
				// - suspensión ^Z
				// wait?? -> bloqueante
				waitpid(pid_fork, &status, WUNTRACED);

				// cuando se "sale" del wait?
				// EXITED -> el hijo acaba normalmente
				// SIGNALED -> muere por culpa de una señal
				// SUSPENDED -> se suspende (^Z, o kill -STOP ...)
				status_res = analyze_status(status, &info);
				if (status_res == EXITED)
				{
					printf("\nEl hijo en fg acabó normalmente y retornó %d\n", info);
				}
				else if (status_res == SIGNALED)
				{
					printf("\nEl hijo en fg acabó por una señal\n");
				}
				else if (status_res == SUSPENDED)
				{
					printf("\nEl hijo en fg se suspendió\n");
				}

				// meter en lista SI suspendido
				if (status_res == SUSPENDED)
				{
					nuevo = new_job(pid_fork,
									inputBuffer,
									STOPPED);
					block_SIGCHLD();
					add_job(lista, nuevo);
					unblock_SIGCHLD();
				}

				// el shell recupera el terminal
				set_terminal(getpid());
			}
		}
		else if (pid_fork == 0)
		{
			// HIJO

			new_process_group(getpid()); // hijo en un grupo nuevo independiente

			if (background)
			{
				// hijo en bg
			}
			else
			{
				// hijo en fg
				// ceder el terminal al hijo
				set_terminal(getpid());
			}

			terminal_signals(SIG_DFL); // por defecto ^C, ^Z ...

			execvp(args[0], args); // args[0]="xclock";

			// Error en exec?
			perror("\nSi llega aquí hubo un error en exec\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			// Error en fork()
			perror("\nError en fork ....\n");
		} // end while
	}
}