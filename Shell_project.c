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

#include job_control.h // remember to compile with module job_control.c

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------
job *lista;
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

	lista = new_list("milista");
	job *nuevo;
	terminal_signals(SIG_IGN); //no muere con ^c ni se suspende con ^z
	signal(SIGCHLD, mySigChild);

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

		if (!strncmp(args[0], "jobs", MAX_LINE))
		{
			// Hacer algo para el comando jobs
			JOBS();
			continue;
		}

		if (!strncmp(args[0], "cd", MAX_LINE))
		{ 
			CD(args[1]);
			continue;
		}
		else if (!strncmp(args[0], "jobs", MAX_LINE))
		{
			JOBS();
			continue;
		}
		else if (!strncmp(args[0], "fg", MAX_LINE))
		{
			if (args[1] == NULL)
			{
				FOREG(1);
			}
			else if (atoi(args[1]) < 1 || atoi(args[1]) > list_size(job_list))
			{
				printf("Posición no valida.\n");
			}
			else
			{
				FOREG(atoi(args[1]));
			}
			continue;
		}
		else if (!strncmp(args[0], "bg", MAX_LINE))
		{
			if (args[1] == NULL)
			{
				BACKG(1);
			}
			else if (atoi(args[1]) < 1 || atoi(args[1]) > list_size(job_list))
			{
				printf("Posición no valida.\n");
			}
			else
			{
				BACKG(atoi(args[1]));
			}
			continue;
			continue;
		}
		else if (!strncmp(args[0], "children", MAX_LINE))
		{
			CHILDREN();
			continue;
		}

		// comando interno bg:
		// checkear argumentos de bg -> args[1] == NULL sin argumentos
		// atoi(args[1]) -> si tiene argumentos (atoi("5") -> 5)
		// chequear los rangos del argumento   1 < n <numero de jobs
		// job->state = BACKGROUND;
		// si estaba suspendido -> SIGCONT de grupo

		// comando interno fg:
		// chequear argumentos de fg -> igual que en bg
		//
		// job->state = FOREGROUND;
		//
		// sacarlo de la lista
		//
		// ceder el terminal al hijo
		// set_terminal(pgid del job);
		//
		// si estaba suspendido -> SIGCONT de grupo
		//
		// el shell se tiene que quedar esperando:
		// - terminación (exit/signal)
		// - suspensión ^Z
		// wait?? -> bloqueante
		// waitpid(pgid del job , &status, WUNTRACED);
		//
		// meter en lista SI suspendido
		//~ if (status_res == SUSPENDED){
		//~ nuevo = new_job(pid_fork,
		//~ inputBuffer,
		//~ STOPPED);
		//~ block_SIGCHLD();
		//~ add_job(lista, nuevo);
		//~ unblock_SIGCHLD();
		//~ }
		//
		//~ // el shell recupera el terminal
		//~ set_terminal(getpid());

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
			if (background==0)
			{
				set_terminal(pid_fork);
				int info;
				whilepid(fork!=waitpid(-pid_fork,&status,WUNTRACED)){
					printf("Esperando ");
				}

				if(analyze_status(status,&info)==0){
					job* trabajo=new_job(pid_fork,args[0],STOPPED);
					add_job(lista,tranajo);
					enum status estado=analyze_status(status,&info);

					printf("Foregroun pid: %d, command: %s, %s, info: %d \n",pid_fork, args[0],status_string[estado],info);
					set_terminal(getpid());
				}


			}
			else
			{

				if(background){
					job* trabajo=new_job(pid_fork,args[0],BACKGROUND);
				}else{
					job* trabajo=new_job(pid_fork,args[0],FOREGROUND);
				}
				add_job(lista,trabajo);
				printf("Se esta ejecutando en BACGROUND.. pid: %d, command %s", getpid(),args[0]);
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

	} // end while
}

void mySigChild(int s)
{

	/*
		MANEJADOR DE SIGCHILD ->
	recorrer todos los jobs en bg y suspendidos a ver que les ha pasado
	SI MUERTOS-> quitar de la lista
	SI CAMBIAN DE ESTADO -> cambiar el job correspondiente
		*/

	int i, status, info, pid_wait;
	enum status status_res;
	job* jb;

	for (int i = 1; i <= list_sise(lista); i++)
	{
		jb = get_item_bypos(lista, i);

		pid_wait = waitpid(jb->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED);

		if (pid_wait == jb->pgid)
		{
			status_res = analyze_status(status,& info);
			printf("[SIGCHILD] Wait realizado para trabajo en background: %s, pid=%i\n", jb->command, pid_wait);

			if ((status_res == SIGNALED) | (status_res == EXITED))
			{
				delete_job(lista, jb);
				i--;
			}

			if (status_res == CONTINUED)
			{
				jb->stare = BACKGROUND;
			}

			if (status_res == SUSPENDED)
			{
				jb->state = STOPPED;
			}

			print_job_list(lista);
		}
	}
	return;
}

void JOBS(){

}

void CD(){

}

void FROREG(){

}

void BACKG(){

}

void CHILDREN(){
	
}


