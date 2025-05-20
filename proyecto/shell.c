/**
* Linux Job Control Shell Project
*
* Operating Systems
* Grados Ing. Informatica & Software
* Dept. de Arquitectura de Computadores - UMA
*
* Some code adapted from "OS Concepts Essentials", Silberschatz et al.
*
* To compile and run the program:
*   $ gcc shell.c job_control.c -o shell
*   $ ./shell
*	(then type ^D to exit program)
**/

#include "job_control.h"   /* Remember to compile with module job_control.c */

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough */

/************************************************************************
						MAIN
************************************************************************/
int main(void)
{
	//BASE
	char inputBuffer[MAX_LINE]; /* Buffer to hold the command entered */
	int background;             /* Equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* Command line (of 256) has max of 128 arguments */
	/* Probably useful variables: */
	int pid_fork, pid_wait; /* PIDs for created and waited processes */
	int status;             /* Status returned by wait */
	enum status status_res; /* Status processed by analyze_status() */
	int info;				/* Info processed by analyze_status() */

	//NUEVO

	ignore_terminal_signals(); //igrnorar las sñales relacionadas con el terminal al principio

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND->"); //pide un comando
		fflush(stdout);
		//fflush = comando que fuerza el vaciado del búfer de salida para evitar posibles errores
		get_command(inputBuffer, MAX_LINE, args, &background);  /* Nuevo comando*/
		/* 
			COMPONENTES: 
			- inputBuffer = almacena los datos introducidos por el usuario
			- MAX_LINE = tamaño máximo que puede tener un comando (no lo usamos)
			- args = array de strings que almacena las partes que componen el comando (0 = comando, 1 = ... resto de partes del comando)
			- &background = variable que representa si el comando es introducido a primer plano o segundo plano. Pasamos por referencia (porque se mofica)	
		*/
		if(args[0]==NULL) continue;  /* Do nothing if empty command */
		//continue = obliga a que se pase a la siguiente iteracioń 

		/* 
		FINAL FASE 1:
		- implementar el comando cd para cambiar el direcotio actual
		- implementar el comando exit para salir del shell sin hacer Ctrl+D 
		*/
		if (strcmp(args[0], "exit") == 0){ //si el comando introducido es "exit"
			exit(EXIT_SUCCESS); //es lo mismo que poner exit(0)
		} else{
			if (strcmp(args[0], "cd") == 0){ //si el comando introducido es "cd"
				chdir(args[1]); //pasamos como argumento el path al que queremos ir, args[1]
				continue; //vuelve a pedir otro comando
			} else{ //cualquier otro comando
				/** The steps are:
				*	 (1) Fork a child process using fork()
				*	 (2) The child process will invoke execvp()
				* 	 (3) If background == 0, the parent will wait, otherwise continue
				*	 (4) Shell shows a status message for processed command
				* 	 (5) Loop returns to get_commnad() function
				**/

				//CREACIÓN DE UN PROCESO HIJO (fork())
				pid_fork = fork(); //Variable que almacena el pid del proceso hijo que vamos a crear
				if (pid_fork > 0){ //Padre
					//Cuando el hijo ejecuta un comando el padre tiene que esperar a que termine el hijo 
					if (background == 0){ //PRIMER PLANO (fg)
						/* 
						Esperar a un proceso 
						Usamos el comando waitpid(pid_proceso, &status, opciones)
						- pid_proceso = pid del proceso al que va a esperar (en nuestro caso al hijo)
						- status = puntero a la variable status que almacenará el estado en el que ha terminado el hijo
						- opciones = 0

						Valor de retorno = pid que ha recogido
						*/
						waitpid(pid_fork, &status, WUNTRACED); 
						/*
						Analizamos el estado en el que ha termina un proceso 
						Usamos la función analizeStatus(status, &info)
						- status = estado en el que ha terminado el proceso -> como es solo lectura no lo pasamos por referencia
						- &info = variable que almacena información sobre el proceso pasado por referencia
						- opciones = añadimos la opción WUNTRACED -> opción para tener en cuenta la suspensión de procesos

						Valor de retorno = enum status	
						- enum status = tipo que tiene tres valores
							- SUSPENDIDO 
							- RENAUDADO 
							- FINALIZADO
						*/
					
						/* 
						Dentro el código del padre tenemos que esperar a que el hijo termine, asignamos el terminal proceso que le toque, recuperamos el acceso al terminal
						*/	
						set_terminal(getpid()); //devuelve el terminal al proceso -> si hacemos getppid() = devuelve el pid del proceso padre 

						status_res = analyze_status(status, &info); //ALMACENA EL ESTADO DEL PROCESO
						/* 
						Imprimimos mensaje ejecución del proceso en primer plano 
						FORMATO: "Foreground pid: pid, command: comando, status: Exited, info: info"
						*/
						/* 
						CONTROL DE ERRORES = los errores siempre tienen como valor en info el numero 255 -> todo el que tenga un número distinto, entonces es válido
						Solo queremos imprimir la información del comando en caso de que sea un comando válido
						*/

						/*
						Analizamos el estado del proceso 
						*/
						if (status_res == SUSPENDED){
							printf("\nForeground pid: %d, command: %s, Suspended, %s: %d\n", pid_fork, args[0], status_strings[status_res], info);
						} else if (status_res == EXITED){
							if (info != 255){
								printf("\nForeground pid: %d, command: %s, status: %s, info: %d\n", pid_fork,args[0], status_strings[status_res], info); 
							}
						}

					} else{ //SEGUNDO PLANO (bg)
						/* 
						Imprimimos mensaje de que el proceso está en segundo plano 
						FORMATO: "Background pid: pid, command: comando, status: Suspended, info: info"
						*/
						printf("\nBackground job running... pid: %d, command: %s\n", pid_fork, args[0]); 
					}

				} else{ //Hijo
					/* 
					Asigna un grupo propio
					*/
					new_process_group(getpid());  //para crear un nuevo grupo usamos el método que ya está creado al que tenemos que pasar como parámetro el pid del proceso
					if (background == 0){
						set_terminal(getpid())
					} 
					/* 
					Lanzar un comando de linux
					Usamos la función execvp (comando, args)
						- comando = args[0]
						- args = argumentos
					*/
					execvp(args[0], args); //ejecuta el comando que se había introducido
					/* 
					Control de errores en el comando: 
					En el caso de que el comando introducido no fuera válido (error) el comando execvp no va a poder sustituir el código por el comando 
					Al no cambiar el código del hijo, el único caso en el que el código se seguiría ejecutando en la siguiente línea a execvp es cundo el comando no 
					ha podido ser identificado y por tanto es un error -> imprimos mensaje de error + exit(-1)

					FORMATO: "Error, command not found: {comando}"
					*/
					printf("\nError, command not found: %s\n", args[0]);
					exit(-1); //fin del programa 
				}
			}
		} /* End while */
	}
}
