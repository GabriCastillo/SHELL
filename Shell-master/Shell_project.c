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


/**
 * Nombre: Javier Ortuño Roig
 * DNI: 77186883X
 */

#include "job_control.h"   // remember to compile with module job_control.c 
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h> //Para el isdigit

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job *job_list;

void manejador(int senial) {

	block_SIGCHLD();
	int status, info;

	for (int i = 1; i <= list_size(job_list); i++) {
		job *trabajo = get_item_bypos(job_list, i);
		int correcto;
		correcto = waitpid(- trabajo -> pgid, &status, WNOHANG | WUNTRACED | WCONTINUED); //Hacemos un wait no bloqueante (WNOHANG)

		if (correcto > 0) {
			int estatus_analizado = analyze_status(status, &info);

			if (estatus_analizado == 3 || estatus_analizado == 1) { //Si se recibe la señal SIGCONT
				trabajo -> state = BACKGROUND;
			} else if (estatus_analizado == 0) {
				trabajo -> state = STOPPED;
			} else if (estatus_analizado == 2) { //Si el proceso está exited lo sacamos de la lista de bg.
				int hecho = delete_job(job_list, get_item_bypos(job_list, i));
				if (hecho == 1) {
					i--;
				}
			}
		}

	}
	unblock_SIGCHLD();
}

void comandoCD(char ruta []) {
	char anterior [100], posterior [100];
	getcwd(anterior, sizeof(anterior));
	chdir(ruta);
	getcwd(posterior, sizeof(posterior));

	if (strcmp(anterior, posterior) == 0) {
		printf("\nRuta no válida\n");
	}
}

void comandoJOBS() { //Dado que mi lista de trabajos solo tiene comandos en bg y stopped simplemente tengo q imprimirla
	print_job_list(job_list);
}

void comandoFG(int pos) {

	int status, info;
	job *trabajo = get_item_bypos(job_list, pos);
	trabajo -> state = FOREGROUND;
	set_terminal(trabajo -> pgid);
	waitpid(-trabajo -> pgid, &status, WUNTRACED); //Para tener en cuenta también la suspensión de un hijo  es  necesario  añadir  la  opción  WUNTRACED

	int estatus_analizado = analyze_status(status, &info);
	killpg(trabajo -> pgid, SIGCONT);
	if (estatus_analizado == 0) { //SUSPENDED
		trabajo -> state = STOPPED;
	} else {
		delete_job(job_list, trabajo);
	}
}

void comandoBG(int pos) {
	job *trabajo = get_item_bypos(job_list, pos);
	if (trabajo -> state == STOPPED) {
		trabajo -> state = BACKGROUND;
		killpg(trabajo -> pgid, SIGCONT);
	}
}

int esNumero(char linea[]) {
	int esNumero = 1, i = 0;
	while (esNumero == 1 && i < strlen(linea)) {
		if (isdigit(linea[i]) == 0) {
			esNumero = 0;
		}
		i++;
	}
	return esNumero;
}

void comandoChildren() {

	system("mkdir Procesos");

	char rutaInicial [200];
	char rutaProcesos [200] = "/proc/";
	char rutaProcesosIndividuales [200] = "/Procesos/";
	getcwd(rutaInicial, sizeof(rutaInicial));
	chdir(rutaProcesos);

	printf("PID\t\tCHILDREN\t\tTHREADS"); //cabecera

	char rutaArchivos [200];
	strcpy(rutaArchivos, rutaInicial);
	strcat(rutaArchivos, "/ls.txt");
	char comando [200] = "ls > ";
	strcat(comando, rutaArchivos);
	system(comando);

	int parents [500];
	int parentSize = 0;
	int childrens[500];
	int i = 0;
	while (i < 500) {
		childrens[i] = 0; //Lo inicializamos porque los que valgan 0 no se imprimiran
		i++;
	} 


 	FILE *ls = fopen(rutaArchivos, "rt");

	if (ls == NULL) {
		printf("ERROR al abrir el docuemento");
	} else {
		char lineas [100];
		int i = 0;
		while (!feof(ls)) {
			fscanf(ls, "%s", lineas);
			int Pid;

			if (esNumero(lineas)) {

				char rutaIndividual [200];
				strcpy(rutaIndividual, rutaProcesos);
				strcat(rutaIndividual, lineas);

				//Creacion de status filtrados PPID -------
				char textoPID [200] = "cat ";
				strcat(textoPID, rutaIndividual);
				strcat(textoPID, "/status | grep PPid > ");
				strcat(textoPID, rutaInicial);
				strcat(textoPID, rutaProcesosIndividuales);
				strcat(textoPID, lineas);
				strcat(textoPID, ".txt");

				char rutaArchivos [200];
				strcpy(rutaArchivos, rutaInicial);
				strcat(rutaArchivos, "/esta.txt");

				char comprobacion [100];
				strcpy(comprobacion, "ls ");
				strcat(comprobacion, rutaProcesos);
				strcat(comprobacion, lineas);
				strcat(comprobacion, " > ");
				strcat(comprobacion, rutaArchivos);
				system(comprobacion);

				FILE *status = fopen(rutaArchivos, "rt");
				if (status == NULL) {
					printf("Error al leer archivo");
				} else {
					char elementos[100];
					int existeStatus = 0, z = 0;
					while (!feof(status)) {
						fscanf(status, "%s", elementos);
						if (strcmp(elementos, "status") == 0) {

							system(textoPID);


							//Rellenar array
							char procesoStatus[200];
							strcpy(procesoStatus, rutaInicial);
							strcat(procesoStatus, rutaProcesosIndividuales);
							strcat(procesoStatus, lineas);
							strcat(procesoStatus, ".txt");

							FILE *proc = fopen(procesoStatus, "rt");

							if (proc == NULL) {
								printf("Error");
							} else {
								char dato [100];
								fgets(dato, 100, proc);

								int i = 0;
								char PPid [20] = "";
								while (i < strlen(dato)) {
									if (isdigit(dato[i]) != 0) {
										strcat(PPid, &dato[i]);
									}
									i++;
								}
								int PPIDNumber = atoi(PPid);

								int j = 0, encontrado = 0;
								while (j < parentSize && encontrado == 0) {
									if (parents[j] == PPIDNumber) {
										encontrado = 1;
										childrens[j] = childrens[j] + 1;
									}
									j++;
								}
								if (encontrado == 0) {
									parents[parentSize] = PPIDNumber;
									childrens[parentSize]= 1;
									parentSize++;
								}
							}
							fclose(proc);
							//----------------------


							//Creacion de status filtrados PID-------
							textoPID [200];
							strcpy(textoPID, "cat ");
							strcat(textoPID, rutaIndividual);
							strcat(textoPID, "/status | grep Pid > ");
							strcat(textoPID, rutaInicial);
							strcat(textoPID, rutaProcesosIndividuales);
							strcat(textoPID, lineas);
							strcat(textoPID, ".txt");
							system(textoPID);
							//-------------------

							//Mostrar Columna PID
							strcpy(procesoStatus, rutaInicial);
							strcat(procesoStatus, rutaProcesosIndividuales);
							strcat(procesoStatus, lineas);
							strcat(procesoStatus, ".txt");

							proc = fopen(procesoStatus, "rt");

							if (proc == NULL) {
								printf("Error");
							} else {
								char dato [100];
								printf("\n");
								fgets(dato, 100, proc);

								//fscanf(proc, "%s", dato);
								//fscanf(proc, "%s", dato);
								int i = 0;
								char pidString[20] = "";
								while (i < strlen(dato)) {
									if (isdigit(dato[i]) != 0) {
										//printf("%c", dato[i]);
										strcat(pidString, &dato[i]);
									}
									i++;
								}
								Pid = atoi(pidString);
								printf("%d", Pid);

								int j = 0, encontrado = 0;
								while (j < parentSize && encontrado == 0) {
									if (parents[j] == Pid) {
										encontrado = 1;
										printf("\t\t%d", childrens[j]);
									} 
									j++;
								}
								if (encontrado == 0) {
									printf("\t\t0");
								}
							}
							fclose(proc);
							//------------------- 


							//-------------------

							//Creacion de status filtrados Threads-------
							strcpy(textoPID,"cat ");
							strcat(textoPID, rutaIndividual);
							strcat(textoPID, "/status | grep Threads > ");
							strcat(textoPID, rutaInicial);
							strcat(textoPID, rutaProcesosIndividuales);
							strcat(textoPID, lineas);
							strcat(textoPID, ".txt");
							system(textoPID);
							//-------------------

							//Mostrar Columna Threads
							procesoStatus[200];
							strcpy(procesoStatus, rutaInicial);
							strcat(procesoStatus, rutaProcesosIndividuales);
							strcat(procesoStatus, lineas);
							strcat(procesoStatus, ".txt");

							proc = fopen(procesoStatus, "rt");

							if (proc == NULL) {
								printf("Error");
							} else {
								char dato [100];
								printf("\t\t");
								fscanf(proc, "%s", dato);
								fscanf(proc, "%s", dato);
								int i = 0;
								while (i < strlen(dato)) {
									if (isdigit(dato[i]) != 0) {
										printf("%c", dato[i]);
									}
									i++;
								}
							}
							//------------------- */
						}
					}
				}

				
			}
		}
	}
	char borrar1 [100];
	char borrar2 [100];
	char borrar3 [100];

	strcpy(borrar1, "rm -r ");
	strcat(borrar1, rutaInicial);
	strcat(borrar1, "/Procesos");

	strcpy(borrar2, "rm -r ");
	strcat(borrar2, rutaInicial);
	strcat(borrar2, "/esta.txt");
	
	strcpy(borrar3, "rm -r ");
	strcat(borrar3, rutaInicial);
	strcat(borrar3, "/ls.txt");

	system(borrar1);
	system(borrar2);
	system(borrar3);
	
}


// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */


	job_list = new_list("Job List");

	signal(SIGCONT, manejador); //Sale en el PDF pero no entiendo exactamente que hace
	signal(SIGSTOP, manejador); //Sale en el PDF pero no entiendo exactamente que hace
	signal(SIGCHLD, manejador);
	ignore_terminal_signals();

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{
		// print_job_list(job_list); Se ve con el comando jobs

		printf("\n\nCOMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command


		if (strcmp(args[0], "cd") == 0) { // Comando cd
			comandoCD(args[1]);
			continue;

		} else if (strcmp(args[0], "jobs") == 0) {
			comandoJOBS();
			continue;

		} else if (strcmp(args[0], "fg") == 0) {
			if (args[1] == NULL) {
				comandoFG(1);
			} else if (atoi(args[1]) < 1 || atoi(args[1]) > list_size(job_list)) {
				printf("Posición no valida.\n");
			} else {
				comandoFG(atoi(args[1])); //Atoi pasa un string a un int. https://www.educative.io/edpresso/how-to-convert-a-string-to-an-integer-in-c
			}
			continue;

		} else if (strcmp(args[0], "bg") == 0) {
			if (args[1] == NULL) {
				comandoBG(1);
			} else if (atoi(args[1]) < 1 || atoi(args[1]) > list_size(job_list)) {
				printf("Posición no valida.\n");
			} else {
				comandoBG(atoi(args[1])); //Atoi pasa un string a un int. https://www.educative.io/edpresso/how-to-convert-a-string-to-an-integer-in-c
			}
			continue;			continue;
		} else if (strcmp(args[0], "children") == 0) {
			comandoChildren();
			continue;
		}

		pid_t pidShell = getpid();

		pid_t pid_fork = fork();   // creamos un proceso hijo
		int status;
		

		if (pid_fork == 0) { /* proceso hijo */

			restore_terminal_signals();

			if (-1 == execvp(args[0], args)) {
				printf("Error, command not found: %s ", args[0]);
			} 

			exit(-1);

		} else { //Proceso padre

			new_process_group(pid_fork); //Creamos un nuevo proceso


			if (background == 0) { //Foreground

				set_terminal(pid_fork);
				int info;
				while (pid_fork != waitpid(-pid_fork, &status, WUNTRACED));

				if (analyze_status(status, &info) == 0) { 
					job* trabajo = new_job(pid_fork, args[0], STOPPED);
					add_job(job_list, trabajo);
				}

				enum status estado = analyze_status(status, &info);

				printf("\nForeground pid: %d, command: %s, %s, info: %d \n", pid_fork, args[0], status_strings[estado], info);
				fflush(stdout);
				set_terminal(pidShell);

			} else { //background
				
				job* trabajo = new_job(pid_fork, args[0], background ? BACKGROUND : FOREGROUND);
				add_job(job_list, trabajo);
				printf("Background job running... pid: %d, command: %s", getpid(), args[0]);
			}
		}
		printf("\n--------------------------------------------------------------------\n\n");
	} // end while
}
