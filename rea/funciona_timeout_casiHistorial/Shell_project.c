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
	printf(ROJO "Demostración %sde %scolor\n" NEGRO, VERDE, AZUL);
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
	int pid_timeout;
	//////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////

	job *nuevo; // para meter un nuevo trabajo en la lista

	terminal_signals(SIG_IGN); // Ya no se muere con ^C ni se suspende con ^Z

	signal(SIGCHLD, mysigchild);

	while (1) /* Program terminates normally inside get_command() after ^D is typed*/
	{
	timeout=0;
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
			if(args[1]!=NULL&&args[2]!=NULL){
				time=atoi(args[1]);
				if(time<=0){
					printf("El tiempo debe ser natural\n");
					continue;
				}
				
				timeout=1;
				int p=0;
				while(args[p]!=NULL){//time-out 5 xclock -update 1
					args[p]=args[p+2];
					p++;
				}
				args[p-2]=NULL;
				pid_fork=fork();
				
			}else{
				printf("faltan argumentos");
				continue;
			}
		}else{
		pid_fork=fork();
		
		}
		/////////////////////////////////////////////////////////////////////
		
		
		//pid_fork=fork();


		if (pid_fork != 0)
		{
		// PADRE (shell)
		if(timeout){//time-out 5 xclock -update 1
		pid_timeout=fork();
				if(pid_timeout==0){

					sleep(time);
					killpg(pid_fork,SIGKILL);
exit(0);
					
				}
				}
			

			//new_process_group(pid_fork); // hijo en un grupo nuevo independiente
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
				pid_wait=waitpid(pid_fork, &status, WUNTRACED);

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
		if (pid_fork == 0)
		{
			// HIJO

			new_process_group(getpid()); // hijo en un grupo nuevo independiente

			

			restore_terminal_signals(); // por defecto ^C, ^Z ...

			execvp(args[0], args); // args[0]="xclock";

			// Error en exec?
			perror("\nSi llega aquí hubo un error en exec\n");
			exit(EXIT_FAILURE);
		}/*
		else
		
		{
		
		if(timeout){//time-out 5 xclock -update 1
				if(pid_timeout==0){
					sleep(time);
					killpg(pid_fork,SIGKILL);

					continue;
				}
				}
				
		if (background==0)
			{
				set_terminal(pid_fork);
				pid_wait=waitpid(pid_fork,&status,WUNTRACED);
				set_terminal(getpid());
				status_res=analyze_status(status,&info);
				
				if(status_res==SUSPENDED){
				
				block_SIGCHLD();
				add_job(lista,new_job(pid_fork,args[0],STOPPED));
				unblock_SIGCHLD();
				
				}
				
				printf("algo");
				
			}
			else
			{
			block_SIGCHLD();
			add_job(lista,new_job(pid_fork,args[0],BACKGROUND));
			unblock_SIGCHLD();
			
			printf("otracosa");
			
				// hijo en fg
				// ceder el terminal al hijo
				set_terminal(getpid());
				
				if(timeout){//time-out 5 xclock -update 1
				if(pid_timeout==0){
					sleep(time);
					killpg(pid_fork,SIGKILL);

					continue;
					}
				}
				
			
					
			}
			// Error en fork()
			perror("\nError en fork ....\n");
		}*/ 
	}// end while
}
