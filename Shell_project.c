/**
UNIX Shell Project

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

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------
job *lista; //*tareas
int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;				/* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE / 2];	/* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;				/* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;
	int fg = 0; /* info processed by analyze_status() */

	new_list("milista");
	job *nuevo;				   //item
	terminal_signals(SIG_IGN); //no muere con ^c ni se suspende con ^z
	signal(SIGCHLD, mySigChild);

	ignore_terminal_signals();
	while (1) /* Program terminates normally inside get_command() after ^D is typed*/
	{
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background); /* get next command */

		if (args[0] == NULL)
			continue; // if empty command

		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/

		if (!strncmp(args[0], "cd", MAX_LINE))
		{
			int res;
			res = chdir(args[1]);
			if (res == -1)
			{
				printf("\n Error, ruta %s no encontrada", args[1];)
			}
			continue;
		}

		if (!strncmp(args[0], "jobs", MAX_LINE))
		{
			block_SIGCHLD();
			print_job_list(lista);
			unblock_SIGCHLD();
			continue;
		}

		if (!strncmp(args[0], "fg", MAX_LINE))
		{
			block_SIGCHLD();
			int pos = 1;
			fg = 1;

			if (args[1] != NULL)
			{
				pos = atoi(args[1]);
			}
			nuevo = get_item_bypos(lista, pos);
			if (nuevo != NULL)
			{
				set_terminal(nuevo->pgid);
				if (nuevo->state == STOPPED)
				{
					killpg(nuevo->pgid, SIGCONT);
				}
				pid_fork = nuevo->pgid;
				delete_job(lista, nuevo);
			}
			unblock_SIGCHLD();
		}

		if (!strncmp(args[0], "bg", MAX_LINE))
		{
			block_SIGCHLD();
			if (atoi(args[1]) < 1 || atoi(args[1]) > list_size(lista))
			{
				printf("\nPosición no valida.\n");
			}
			else
			{
				int pos = 1;
				if (args[1] != NULL)
				{
					pos = atoi(args[1]);
				}
				nuevo = get_item_bypos(lista, pos);
				if ((nuevo != NULL) && (nuevo->state == STOPPED))
				{
					nuevo->state = BACKGROUND;
					killpg(nuevo->pgid, SIGCONT);
				}
			}
			unblock_SIGCHLD(); 
			continue;
		}

		if (!strncmp(args[0], "children", MAX_LINE))
		{
			CHILDREN();
			continue;
		}
		if (!fg)
		{
			pid_fork = fork();
		}

		if (pid_fork > 0)
		{
			if (background == 0) //primer plano
			{
				waitpid(pid_fork, &status, WUNTRACED);
				set_terminal(getpid());

				status_res = analyze_status(status, &info);
				if (status_res == EXITED)
				{
					printf("\n Comando %s ejecutado en primer plano con pid %d. Estado $s. Info: %d\n", args[0], pid_fork, status_strings[status_res], info);
				}
				else if (status_res == SIGNALED)
				{
					printf("\n Comando %s ejecutado en primer plano con pid %d, termino por una señal\n", args[0], pid_fork);
				}
				else if (status_res == SUSPENDED)
				{
					block_SIGCHLD();
					nuevo = new_job(pid_fork, args[0], SUSPENDED)
						add_job(milista, nuevo);
					printf("\n Comando %s ejecutado en primer plano con pid %d. Estado $s. Info: %d\n", args[0], pid_fork, status_strings[status_res], info);
					unblock_SIGCHLD();
				}
				if (info != 255)
				{
					printf("\n Comando %s ejecutado en primer plano con pid %d. Estado finalizado. Info: %d\n", args[0], pid_fork, info);
				}
			}
			fg = 0;
			else //segundo plano
			{
				block_SIGCHLD();
				nuevo = new_job(pid_fork, args[0], BACKGROUND)
					add_job(milista, nuevo);
				printf("\n Comando %s ejecutado en segundo plano con pid %d\n", args[0], pid_fork);
				unblock_SIGCHLD();
			}
		}
		else
		{
			new_process_group(getpid());
			if (background == 0)
			{
				set_terminal(getpid();)
			}
			restore_terminal_signals();
			execvp(args[0], args);
			printf("\n Error. Comando %s no encontrado", args[0])
				exit(-1);
		}

		/*
		// Externos
		pid_t pidShell = getpid;
		pid_t pid_fork = fork();

		if (pid_fork > 0)
		{
			//padre shell
			new_process_group(pid_fork); // hijo en un grupo nuevo independiente
										 // hijo es el lider de su grupo gpid == pid
			if (background)
			{
				//poner hijo bg
				printf("background\n");
				//meter en lista

				//nuevo nodo job -> nuevo job BACKGROUND
				nuevo = new_job(pid_fork, inputBuffer, BACKGROUND)

					// Meterlo en la lista
					// Sección crítica libre de SIGCHLD

					block_SIGCHLD();
				add_job(lista, nuevo);
				unblock_SIGCHLD();
			}
			else
			{

				// hijo  fg
				//ceder el terminal
				set_terminal(pid_fork);
				waitpid(pid_fork, &status, WUNTRACED);
				status_res = analyze_status(status, &info);

				if (status_res == EXITED)
				{
					printf("El hijo en fg acabo normalmente y retorno %d\n", info);
				}
				else if (status_res == SIGNALED)
				{
					printf("El hijo en fg acabo por una seña\n");
				}
				else if (status_res == SUSPENDED)
				{
					printf("El hijo en fg se suspendio\n");
				}

				if (status_res == SUSPENDED)
				{
					nuevo = new_job(pid_fork, inputBuffer, STOPPED);
					block_SIGCHLD();
					add_job(lista, nuevo);
					unblock_SIGCHLD();
				}
				set_terminal(getpid());
			}
		}
		else if (pid_fork == 0)
		{

			//hijo
			new_process_group(getpid());
			if (background == 0)
			{
				set_terminal(pid_fork);
				int info;
				whilepid(fork != waitpid(-pid_fork, &status, WUNTRACED))
				{
					printf("Esperando ");
				}

				if (analyze_status(status, &info) == 0)
				{
					job *trabajo = new_job(pid_fork, args[0], STOPPED);
					add_job(lista, tranajo);
					enum status estado = analyze_status(status, &info);

					printf("Foregroun pid: %d, command: %s, %s, info: %d \n", pid_fork, args[0], status_string[estado], info);
					set_terminal(getpid());
				}
			}
			else
			{

				if (background)
				{
					job *trabajo = new_job(pid_fork, args[0], BACKGROUND);
				}
				else
				{
					job *trabajo = new_job(pid_fork, args[0], FOREGROUND);
				}
				add_job(lista, trabajo);
				printf("Se esta ejecutando en BACGROUND.. pid: %d, command %s", getpid(), args[0]);
				set_terminal(getpid());
			}
			terminal_signals(SIG_DFL);
			execvp(args[0], args);

			perror("Si llega aquí hubo un error en exec");
			exit(EXIT_FAILURE);
		}
		else
		{
			//errror
			perror("Error en fork ...\n");
		}
	}
	*/

		// end while
	}

	void mySigChild(int s)
	{
		block_SIGCHLD();
		/*
		MANEJADOR DE SIGCHILD ->
	recorrer todos los jobs en bg y suspendidos a ver que les ha pasado
	SI MUERTOS-> quitar de la lista
	SI CAMBIAN DE ESTADO -> cambiar el job correspondiente
		*/

		int i, status, info, pid_wait;
		enum status status_res;
		job *jb;

		for (int i = 1; i <= list_sise(lista); i++)
		{
			jb = get_item_bypos(lista, i);

			pid_wait = waitpid(jb->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED);

			if (pid_wait == jb->pgid)
			{
				status_res = analyze_status(status, &info);
				printf("\n [SIGCHILD] Wait realizado para trabajo en background: %s, pid=%i\n", jb->command, pid_wait);

				if ((status_res == SIGNALED) | (status_res == EXITED))
				{
					printf("\n Comando %s ejecutado en segundo plano con pid %d ha concluido", jb->command, jb->pgid);
					delete_job(lista, jb);
					i--;
				}

				if (status_res == CONTINUED)
				{
					printf("\n Comando %s con pid %d se esta ejecutando en segundo plano", jb->command, jb->pgid);
					jb->stare = BACKGROUND;
				}

				if (status_res == SUSPENDED)
				{
					printf("\n Comando %s ejecutado en segundo plano con pid %d ha suspendido su ejecucion", jb->command, jb->pgid);
					jb->state = STOPPED;
				}
			}
		}
		unblock_SIGCHLD();
		return;
	}

	void CHILDREN()
	{
	}
