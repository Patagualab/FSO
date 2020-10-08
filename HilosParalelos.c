//AGUADO LABRADOR PATRICIA		71950984S
//BERMEJO LANCHAS JOSE MANUEL	

//Para la ejecución del programa se introducirán el nombre de este, el tamaño del buffer 1, el fichero
//de salida en el que se quiere almacenar el resultado y el número de consumidores

#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

//Variables globales
int escribir;
char **buffer_circular1;
int tam_buffer_circular1; //El tamaño del buffer1 se pasará como primer argumento a la funcion principal
char **buffer_circular2;
int tam_buffer_circular2 = 5;
int contAleatorios = 0;

FILE *salidaFichero;
char *ficheroSalida;

//Declaración de los semáforos
sem_t hay_dato_b1;
sem_t hay_espacio_b1;

sem_t hay_dato_b2;
sem_t hay_espacio_b2;

sem_t mutex_leer;
sem_t mutex_escribir;
sem_t mutex_contAleatorios;


int palindromo (char *palabra);


//Hilo productor que genera 75 números aleatorios y los almacena pasados a string en el buffer1
void *productor (void *arg){
	int escribir = 0;
	int numeroAleatorio;
	int contadorDeAleatorios;
	srand(time(NULL));
	char aString [4]; //Espacio para almacenar el número una vez pasado a string
	for(contadorDeAleatorios = 0; contadorDeAleatorios<75; contadorDeAleatorios++){ 
	
		numeroAleatorio = rand () % (999+1);	//Genera un número aleatorio entre 0 y 999 (incluidos)
		sprintf(aString, "%d", numeroAleatorio);	//Transformación del número a string
		sem_wait(&hay_espacio_b1);	//Espera a que haya espacio en el buffer1
		
		strcpy(buffer_circular1[escribir],aString);	//Almacenamos el número en el buffer1
		escribir = (escribir +1) % tam_buffer_circular1;

		sem_post(&hay_dato_b1);	//Indica al semáforo que hay datos en el buffer1	
	}

	pthread_exit(NULL);
}


//Hilo consumidor que comprobará si el número almacenado en el buffer1, es o no capicúa,
//y guardará una cadena con el resultado en el buffer2
void *consumidor(void *arg){
	int leer = 0;
	int escribir = 0;
	char numero [4];
	char cadena [30];
	while (1){

		if(contAleatorios >= 75){
			break;
		}

		sem_wait(&hay_dato_b1);	//Espera a que haya datos en el buffer1
		sem_wait(&mutex_leer);	//Acceso excluyente

		strcpy(numero, buffer_circular1[leer]);	//Leer el número
		leer = (leer +1) % tam_buffer_circular1;

		sem_post(&mutex_leer);	//Libera el semáforo
		sem_post(&hay_espacio_b1);	//Indica al semáforo que hay espacio en el buffer1

		if (palindromo(numero) == 1)	//Se comprueba si es o no capicúa y se almacena en 'cadena' la cadena
			sprintf (cadena, "El numero %s es capicua", numero);
		else
			sprintf (cadena, "El numero %s no es capicua", numero);

		sem_wait(&hay_espacio_b2);	//Espera a que haya espacio en el buffer2
		sem_wait(&mutex_escribir);	//Acceso excluyente

		strcpy(buffer_circular2[escribir], cadena);	//Almacenamos la cadena en el buffer2
		escribir = (escribir +1) % tam_buffer_circular2;

		sem_post(&mutex_escribir);	//Libera el semáforo
		sem_post(&hay_dato_b2);	//Indica al semáforo que hay datos en el buffer2

		sem_wait(&mutex_contAleatorios);	//Acceso excluyente
		contAleatorios = contAleatorios+1;
		sem_post(&mutex_contAleatorios);	//Libera el semáforo
	}
	
	pthread_exit(NULL);
}


//Hilo consumidor final que recoge las cadenas de texto del buffer2 y las almacena 
//en un fichero pasado como parámetro
void *consumidorFinal(void *arg){
	int leer = 0;
	char cadenaFinal [30];
	salidaFichero = fopen(ficheroSalida, "w");	//Creamos el fichero
	fclose(salidaFichero);

	for(int i =0; i<75; i++){
		salidaFichero = fopen(ficheroSalida,"a");

		sem_wait(&hay_dato_b2);	//Espera a que haya datos en el buffer2
		strcpy(cadenaFinal, buffer_circular2[leer]);	//Almacenamos la cadena en 'cadenaFinal'
		leer = (leer+1) % tam_buffer_circular2;
		sem_post(&hay_espacio_b2);	//Indica al semáforo que hay datos en el buffer2

		fprintf(salidaFichero, "%s\n", cadenaFinal);	//Escribimos en el fichero la cadena
		fclose(salidaFichero);
	}
	
	pthread_exit(NULL);
}


int palindromo (char *palabra){	//Función que devuelve un 1 si la cadena es capicúa y 0 si no lo es
		int esPalindromo = 1;
		int i, j;

		j= strlen (palabra)-1;
		for(i=0; i< strlen (palabra)/2 && esPalindromo; i++, j--){
			if (*(palabra+i) != *(palabra+j)){
				esPalindromo =0;
			}
		}
		return esPalindromo;
	}



void main(int argc, char *argv[]){

	//Si el número de argumentos no es correcto el programa informa de ello y finaliza
	if(argc != 4){
		printf("%s\n","No se han introducido todos los argumentos." );
		return;
	}

	int numeroConsumidores;

	//Validación de entrada
	if(sscanf (argv[1], "%d", &tam_buffer_circular1) != 1){
		printf("%s\n","No se ha introducido un entero como tamaño para buffer 1." );
		return;
	}
	if(&tam_buffer_circular1 <= 0){
		printf("%s\n", "El valor introducido como tamaño para el buffer 1 no puede ser menor o igual que cero.");
		return;
	}

	if(sscanf (argv[3], "%d", &numeroConsumidores) != 1){
		printf("%s\n","No se ha introducido un entero como numero de consumidores." );
		return;
	}

	if(&numeroConsumidores <= 0){
		printf("%s\n", "El valor introducido como numero de consumidores no puede ser menor o igual que cero.");
		return;
	}

	ficheroSalida = argv[2];

	//Reserva de memoria dinámica para el buffer1 ********************************************
	if ((buffer_circular1 = (char**)malloc(tam_buffer_circular1 *  sizeof (char*))) == NULL) {
      perror("Error al reservar espacio en el buffer 1.");
      return;
    }
    
    for (int i=0; i < tam_buffer_circular1; i++) {

      if((buffer_circular1[i] = (char*)malloc(4*sizeof(char)))== NULL) {
        perror("Error al reservar espacio en el buffer 1.");
        return;
      }
    }

    //Reserva de memoria dinámica para el buffer2 ********************************************
    if ((buffer_circular2 = (char**)malloc(tam_buffer_circular2 *  sizeof (char*))) == NULL) {
      perror("Error al reservar espacio en el buffer 2.");
      return;
    }
    
    for (int i=0; i < tam_buffer_circular2; i++) {

      if((buffer_circular2[i] = (char*)malloc(30*sizeof(char)))== NULL) {
        perror("Error al reservar espacio en el buffer 2.");
        return;
      }
    }

	//Inicialización de semáforos
	sem_init(&hay_dato_b1,0,0);
	sem_init(&hay_espacio_b1,0,tam_buffer_circular1);

	sem_init(&hay_dato_b2,0,0);
	sem_init(&hay_espacio_b2,0,tam_buffer_circular2);

	sem_init(&mutex_leer,0,1);
	sem_init(&mutex_escribir,0,1);
	sem_init(&mutex_contAleatorios,0,1);
	
	pthread_t productorHilo;
	pthread_t *HilosConsumidores;
    if((HilosConsumidores = (pthread_t*)malloc(numeroConsumidores * sizeof(pthread_t)))== NULL) {
        perror("Error al reservar espacio para el array de hilos.");
        return;
    }
	pthread_t consFinalHilo;

	//Creación de los hilos
	pthread_create(&productorHilo, NULL, productor, (void*) NULL);
	for(int i = 0;i < numeroConsumidores;i++){
      pthread_create(&HilosConsumidores[i], NULL, consumidor, (void*) NULL);
    }
	pthread_create(&consFinalHilo, NULL, consumidorFinal, (void*) NULL);
	
	pthread_join(productorHilo, NULL);
	pthread_join(consFinalHilo, NULL);
	for(int i = 0;i < numeroConsumidores;i++){
    	pthread_kill(HilosConsumidores[i], NULL);
 	}

 	printf("El programa ha finalizado.\n");
	
	sem_destroy(&hay_dato_b1);
	sem_destroy(&hay_espacio_b1);
	sem_destroy(&hay_dato_b2);
	sem_destroy(&hay_espacio_b2);
	sem_destroy(&mutex_leer);
	sem_destroy(&mutex_escribir);
	sem_destroy(&mutex_contAleatorios);
}