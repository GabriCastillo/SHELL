/**
UNIX Shell Project
https://github.com/GabriCastillo/SHELL.git
Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

 Autor: Gabriel Enrique Castillo Salazar

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
//                            MANEJADOR
// -----------------------------------------------------------------------
job *lista; //*tareas
job *hist; //puntero para el historial

void mysigchild(int s) {
    printf("\nSe recibió SIGCHLD\n"); // NO SE ACONSEJA usar printf en una señal

    // MANEJADOR DE SIGCHLD ->
    //   recorrer todos los jobs en bg y suspendidos a ver
    //   qué les ha pasado???:
    //     SI MUERTOS-> quitar de la lista
    //     SI CAMBIAN DE ESTADO -> cambiar el job correspondiente

    int i, status, info, pid_wait;
    enum status status_res; /* status processed by analyze_status() */
    job *jb;

    for (int i = 1; i <= list_size(lista); i++) {
        jb = get_item_bypos(lista, i);

        pid_wait = waitpid(jb->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED);

        if (pid_wait == jb->pgid) {
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

            if ((status_res == SIGNALED) | (status_res == EXITED)) {
                delete_job(lista, jb);

                // [1]->[2]->[3]....

                i--; /* Ojo! El siguiente ha ocupado la posicion de este en la lista */
            }

            if (status_res == CONTINUED) {
                jb->state = BACKGROUND;
            } // sOLO CAMBIAR DE ESTADO

            if (status_res == SUSPENDED) {
                jb->state = STOPPED;
            } // SOLO CAMBIAR DE ESTADO

            print_job_list(lista); //debugging
        }
    }

    return;
}

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------
int main(void) {
    printf(ROJO "Demostración %sde %scolor\n" NEGRO, VERDE, AZUL);
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;                /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2];    /* command line (of 256) has max of 128 arguments */

    // probably useful variables:
    int pid_fork, pid_wait; /* pid for created and waited process */
    int status;                /* status returned by wait */
    enum status status_res; /* status processed by analyze_status() */
    int info;                /* info processed by analyze_status() */
    lista = new_list("milista");


    //---Para el timeout---//
    int time, timeout;
    int pid_timeout;
    //-------------------//

    //--------Para el historial--------//
    char *comando[MAX_LINE / 2];
    char *comando2[MAX_LINE / 2];
    hist = new_list("historial");
    //---------------------------------//

    job *nuevo; // para meter un nuevo trabajo en la lista
    terminal_signals(SIG_IGN); // Ya no se muere con ^C ni se suspende con ^Z
    signal(SIGCHLD, mysigchild);

    fflush(stdout);

    while (1) /* Program terminates normally inside get_command() after ^D is typed*/
    {
        timeout = 0;//booleano del timeout
        int pipes = 0;//booleano del pipe
        int cont = 0;//contador del pipe
        comando[0] = NULL;//primer comando del pipe
        comando2[0] = NULL;//segundo comando del pipe
        printf("COMMAND->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); /* get next command */

        /* the steps are:
               (1) fork a child process using fork()
               (2) the child process will invoke execvp()
               (3) if background == 0, the parent will wait, otherwise continue
               (4) Shell shows a status message for processed command
               (5) loop returns to get_commnad() function
          */

        if (args[0] == NULL)
            continue; // if empty command

        for (int i = 0; args[i] != NULL; ++i) {//bucle buscador de pipes
            if (!strcmp(args[i], "|")) {//si encuentra un pipe se cambia el booleano
                pipes = 1;
                comando[i] = NULL;

            } else if (!pipes) {//guarda el primer comando
                comando[i] = strdup(args[i]);
            } else {
                comando2[cont] = strdup(args[i]);//guarda el segundo comando
                if (args[i + 1] == NULL) comando2[cont + 1] = NULL;
                ++cont;
            }
        }
        //---------------------------FUNCIONES---------------------------//
        if (pipes) {//si hay pipes se ejecuta
            // ---------------------------PIPES---------------------------//
            int pid_continue = fork();

            if (!pid_continue) {
                if (comando[0] != NULL && comando2[0] != NULL) {//verificamos que los comandos no son nulos
                    int desc[2];
                    int fno;
                    pipe(desc);
                    if (fork()) {// proceso padre
                        fno = fileno(stdout);
                        dup2(desc[1], fno);//ejecutar primer comando
                        close(desc[0]);
                        execvp(comando[0], comando);
                    } else { // proceso hijo
                        fno = fileno(stdin);
                        dup2(desc[0], fno);//ejecutar primer comando
                        close(desc[1]);
                        execvp(comando2[0], comando2);
                    }
                } else {
                    puts("error: wrong commands");
                    continue;
                }
            }
            sleep(100);
            continue;

        } else if (!strncmp(args[0], "cd", MAX_LINE)) {
            //---------------------------CD---------------------------//
            if (args[1] != NULL) {
                add_job(hist, new_job(getpid(), args[0], FOREGROUND));//añadir al historial
                if (chdir(args[1]) == -1) {//nos lleva al dir espescificado (si existe)
                    printf("\nERROR. No existe el directorio %s\n", args[1]);
                }
            } else {
                chdir(getenv("HOME"));//nos lleva al home
                add_job(hist, new_job(getpid(), args[0], FOREGROUND));//añadir al historial
            }
            continue;

        } else if (!strncmp(args[0], "jobs", MAX_LINE)) {
            //---------------------------JOBS---------------------------//
            block_SIGCHLD();//bloqueamos proximos comandos
            add_job(hist, new_job(getpid(), args[0], FOREGROUND));//añadir al historial
            if (list_size(lista) > 0) {
                print_job_list(lista);
            } else {
                printf("\nLa lista esta vacia\n");
            }
            unblock_SIGCHLD();//desbloqueamos
            continue;

        } else if (!strncmp(args[0], "fg", MAX_LINE)) {
            //---------------------------FG---------------------------//
            block_SIGCHLD();
            job *jl;
            if (empty_list(nuevo)) {//verificamos si hay algun job
                printf("\nNo se encuentra ningún proceso en ejecución\n");
            } else {

                if (args[1] == NULL) {
                    jl = get_item_bypos(lista, 1);
                } else {
                    jl = get_item_bypos(lista, atoi(args[1]));
                }
                //añadir al historial
                if (jl == NULL) {
                    printf("\nERROR. No se encuentra en la lista\n");
                } else {
                    if (jl->state != FOREGROUND) {                            //si no estaba en fg
                        jl->state = FOREGROUND; //lo ponemos en fg
                        set_terminal(jl->pgid); //cedemos la terminal

                        killpg(jl->pgid, SIGCONT); //envia señal al grupo de procesos para que sigan
                        waitpid(jl->pgid, &status, WUNTRACED);

                        set_terminal(getpid()); //devolvemos terminal
                        status_res = analyze_status(status, &info);
                        if (status_res == SUSPENDED) {
                            jl->state = STOPPED;
                        } else {
                            delete_job(lista, jl);
                        }

                        printf("\nForeground pid: %d, command: %s, %s, info:%d\n", pid_fork, args[0],status_strings[status_res], info);
                    } else {

                        printf("\nEl proceso '%s' (pid: %i) no estaba suspendido ni en background\n",jl->command,jl->pgid);
                    }
                }
            }
            unblock_SIGCHLD();
            continue;

        } else if (!strncmp(args[0], "bg", MAX_LINE)) {
            //---------------------------BG---------------------------//
            job *jl;
            int id;
            if (empty_list(nuevo)) {//verificamos si hay algun job
                printf("\nNo se encuentra ningún proceso en ejecución\n");
            } else {
                if (args[1] == NULL) {
                    id = 1;
                } else {
                    id = atoi(args[1]);
                }

                block_SIGCHLD();
                jl = get_item_bypos(lista, id);

                if (jl == NULL) {//comprobamos que este en la lista
                    printf("\nERROR. No se encuentra en la lista\n");
                } else {

                    jl->state = BACKGROUND;
                    printf("\nBackground pid: %id, command: %s\n", jl->pgid, jl->command);
                    killpg(jl->pgid, SIGCONT);
                }
            }
            unblock_SIGCHLD();
            continue;

        } else if (strncmp(args[0], "historial", MAX_LINE) == 0) { //si es historial imprimo la lista del historial
            //---------------------------HISTORIAL---------------------------//
            int n = 0;//indice del historial
            job *aux = hist;//auxiliar para recorrer el historial


            if (args[1] != NULL) {//vemos si pide un numero especifico
                int pos = atoi(args[1]);//numero del item en el historial
                job *njob = get_item_bypos(hist, pos);//job numero pos
                job *copy = new_job(getpid(), njob->command, FOREGROUND);
                if (njob == NULL) {
                    printf(ROJO"Error, no existe el job\n");
                } else {
                    strcpy(args[0], copy->command);
                    args[1] = NULL;
                    printf(":[%d] %s\n",pos,copy->command);
                }
            }else{
                while (aux->next != NULL) {//bucle para recorrer el historial
                    printf(" [%d] ", n);    //numero de la lista del historial
                    printf("%s\n", aux->command);//comando almacenado de la lista
                    n++;
                    aux = aux->next;//siguiente item del historial
                }
            }
            continue;

        } else if (!strncmp(args[0], "time-out", MAX_LINE)) {
            //---------------------------TIMEOUT---------------------------//
            if (args[1] != NULL && args[2] != NULL) {//verifica que haya un tiempo y un comando
                time = atoi(args[1]);//pasa el numero escrito a int
                if (time <= 0) {//verificamos que sea un tiempo valido
                    printf("\nEl tiempo debe ser natural\n");
                    continue;
                }

                timeout = 1;//actualizacion booleano del timeout
                int p = 0;//posicion del args
                while (args[p] != NULL) {//time-out 5 xclock -update 1 -> xclock -update 1
                    args[p] = args[p + 2];//este bucle reemplaza en el args las primeras 2 posiciones con las siguientes
                    p++;
                }
                args[p - 2] = NULL;
                pid_fork = fork();

            } else {
                printf("\nfaltan argumentos\n");
                continue;
            }
        } else {
            pid_fork = fork();

        }
        /////////////////////////////////////////////////////////////////////




        //---------------------------EXTERNO---------------------------//
        if (pid_fork != 0) {
            // PADRE (shell)
            //funcion para el timeout
            if (timeout) {//time-out 5 xclock -update 1
                pid_timeout = fork();
                if (pid_timeout == 0) {

                    sleep(time);
                    killpg(pid_fork, SIGKILL);
                    exit(0);

                }
            }


            new_process_group(pid_fork); // hijo en un grupo nuevo independiente
            // hijo es el lider de su grupo gpid == pid

            if (background) {

                // poner el hijo en bg
                printf("\nbackground\n");

                // meter en lista

                // Nuevo nodo job -> nuevo job BACKGROUND
                nuevo = new_job(pid_fork, inputBuffer, BACKGROUND); //Nuevo nodo job
                add_job(hist, nuevo);
                // Meterlo en la lista
                // Sección crítica libre de SIGCHLD
                block_SIGCHLD(); // mask -> la señal si entra se queda pendiente
                add_job(lista, nuevo);

                unblock_SIGCHLD(); // unmask -> entran sigchild pendientes
             //   printf("\nBackground job running, pid: %d, command: %s\n", pid_fork, args[0]);


            } else {
                // poner el hijo en fg
                add_job(hist, new_job(getpid(), args[0], FOREGROUND));

              //  printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_fork, args[0],status_strings[status_res], info);
             //   fflush(stdout);
                set_terminal(pid_fork);// ceder el terminal al hijo

                // el shell se tiene que quedar esperando:s
                // - terminación (exit/signal)
                // - suspensión ^Z
                // wait?? -> bloqueante
                pid_wait = waitpid(pid_fork, &status, WUNTRACED);
//aaa
                // cuando se "sale" del wait?
                // EXITED -> el hijo acaba normalmente
                // SIGNALED -> muere por culpa de una señal
                // SUSPENDED -> se suspende (^Z, o kill -STOP ...)
               // set_terminal(getpid());
                status_res = analyze_status(status, &info);
                if (status_res == EXITED) {
                    printf("\nEl hijo en fg acabó normalmente y retornó %d\n", info);
                } else if (status_res == SIGNALED) {
                    printf("\nEl hijo en fg acabó por una señal\n");
                } else if (status_res == SUSPENDED) {
                    printf("\nEl hijo en fg se suspendió\n");
                }

                // meter en lista SI suspendido
                if (status_res == SUSPENDED) {
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
        } else if (pid_fork == 0) {
            // HIJO
            new_process_group(getpid()); // hijo en un grupo nuevo independiente
            add_job(hist, new_job(getpid(), args[0], FOREGROUND));
            if (background) {
                // hijo en bg
            } else {
                // hijo en fg
                // ceder el terminal al hijo
                set_terminal(getpid());
            }


            restore_terminal_signals(); // por defecto ^C, ^Z ...
            execvp(args[0], args); // args[0]="xclock";
            // Error en exec?
            perror("\nSi llega aquí hubo un error en exec\n");
            exit(EXIT_FAILURE);
        } else {
            // Error en fork()
            perror("Error en fork ....");
        }
        /*
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
