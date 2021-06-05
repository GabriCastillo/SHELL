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

void manejador(int s)
{

	/*
		MANEJADOR DE SIGCHILD ->
	recorrer todos los jobs en bg y suspendidos a ver que les ha pasado
	SI MUERTOS-> quitar de la lista
	SI CAMBIAN DE ESTADO -> cambiar el job correspondiente
		*/

	int i, status, info, pid_wait;
	enum status status_res;
	job *jb;

	for (int i = list_size(lista); i >= 1; i--)
	{
		jb = get_item_bypos(lista, i);
		pid_wait = waitpid(jb->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED);

		if (pid_wait == jb->pgid)
		{
			status_res = analyze_status(status, &info);

			printf("[SIGCHLD] Wait realizado para trabajo en background: %s, pid=%i\n", jb->command, pid_wait);

			if ((status_res == SIGNALED) | (status_res == EXITED))
			{
				delete_job(lista, jb);
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
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;				/* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE / 2];	/* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;				/* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */

	terminal_signals(SIG_IGN); //no muere con ^c ni se suspende con ^z
	lista = new_list("milista");
	job *nuevo; //item

	signal(SIGCHLD, manejador);

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
			if (args[1] != NULL)
			{
				if (chdir(args[1]) == -1)
				{
					printf("ERROR. No existe el directorio %s\n", args[1]);
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
			if (list_size(lista) > 0)
			{
				print_job_list(lista);
			}
			else
			{
				printf("La lista esta vacia");
			}
			continue;
		}
		else if (!strncmp(args[0], "fg", MAX_LINE))
		{

			job *jl;

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
				printf("ERROR. No se encuentra en la lista\n");
			}
			else
			{
				if (jl->state != FOREGROUND)
				{							//si no estaba en fg
					jl->state = FOREGROUND; //lo ponemos en fg
					set_terminal(jl->pgid);	//cedemos la terminal

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
						delete_job(lista, j);
					}

					printf("Foreground pid: %d, command: %s, %s, info:%d\n", pid_fork, args[0], status_strings[status_res], info);
				}
				else
				{

					printf("El proceso '%s' (pid: %i) no estaba suspendido ni en background", jl->command, jl->pgid);
				}
			}
			continue;
		}
		else if (!strncmp(args[0], "bg", MAX_LINE))
		{

			job *jl;
			int id;

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
				printf("ERROR. No se encuentra en la lista\n");
			}
			else
			{

				jl->state = BACKGROUND;
				printf("Background pid: %id, command: %s\n", jl->pgid, jl->command);
				killpg(jl->pgid, SIGCONT);
			}
			unblock_SIGCHLD();
			continue;
		}
			pid_fork = fork();

		if (pid_fork > 0)
		{
			new_process_group(pid_fork);

			if (background)
			{ //poner el hijo en bg
				//Nuevo nodo job -> nuevo job BG
				nuevo = new_job(pid_fork, inputBuffer, BACKGROUND);
				// Meterlo en la lista
				// Sección crítica libre de SIGCHLD
				block_SIGCHLD(); // mask -> la señal si entra se queda pendiente
				add_job(lista, nuevo);
				unblock_SIGCHLD(); // unmask -> entran sigchild pendientes
				printf("Background job running, pid: %d, command: %s\n", pid_fork, args[0]);
			}
			else
			{ //poner el hijo en fg

				status_res = analyze_status(status, &info);
				printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, args[0], status_strings[status_res], info);
				fflush(stdout);
				set_terminal(pid_fork); //Ceder el terminal al hijo. OJO! se cede a un grupo
				waitpid(pid_fork, &status, WUNTRACED);

				set_terminal(getpid());
				status_res = analyze_status(status, &info);
				if (status_res == EXITED)
				{
					printf("El hijo en fg acabó normalmente\n");
				}
				else if (status_res == SIGNALED)
				{
					printf("El hijo en fg acabó por una señal\n");
				}
				else
				{ // status_res == SUSPENDED
					printf("El hijo en fg se suspendió\n");
				}

				if (status_res == SUSPENDED)
				{
					nuevo = new_job(pid_fork, inputBuffer, STOPPED);
					block_SIGCHLD();
					add_job(lista, nuevo);
					unblock_SIGCHLD();
				}
			}
		}
		else if (pid_fork == 0)
		{								 //lo ejecuta el hijo
			new_process_group(getpid()); //HIJO -> grupo propio

			if (!background)
			{
				set_terminal(getpid());
			}

			terminal_signals(SIG_DFL); // activamos de nuevo ^C y ^Z
			execvp(args[0], args);
			printf("Error, command not found: %s\n", inputBuffer);
			exit(EXIT_FAILURE);
		}
		else
		{
			perror("Error\n");
		} // end while
	}
}