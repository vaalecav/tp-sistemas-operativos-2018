/*
 ============================================================================
 Name        : planificador.c
 Author      : Los Simuladores
 Version     : Alpha
 Copyright   : Todos los derechos reservados
 Description : Proceso Planificador
 ============================================================================
 */

#include "planificador.h"
#include <configuracion/configuracion.h>
#include <pthread.h>

void remove_element(int *array, int index, int array_length) {
	int i;
	for (i = index; i < array_length - 1; i++)
		array[i] = array[i + 1];
}

void tratarConexiones() {
	fd_set descriptoresLectura;
	int socketServer;
	int socketCliente[100];
	int numeroClientes = 0;
	int i;
	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int resultado;

	char ip[16];
	int puerto;
	int maxConexiones;

	//Leo puertos e ips de archivo de configuracion
	leerConfiguracion("PUERTO:%d", &puerto);
	leerConfiguracion("IP:%s", &ip);
	leerConfiguracion("MAX_CONEX:%d", &maxConexiones);

	socketServer = socketServidor(puerto, ip, maxConexiones); //Levanto el servidor.
	/*socketCliente[numeroClientes] = servidorConectarComponente(&socketServer, "planificador",
	 "esi"); //Espero al primer ESI.
	 numeroClientes++;
	 printf("El cliente %d acaba de ingresar a nuestro servidor\n", socketCliente[0]);*/

	while (done != 1) {
		FD_ZERO(&descriptoresLectura);
		FD_SET(socketServer, &descriptoresLectura);
		for (i = 0; i < numeroClientes; i++) {
			FD_SET(socketCliente[i], &descriptoresLectura);
		}

		resultado = select(100, &descriptoresLectura, NULL, NULL, &timeout);

		if (resultado != 1) {
			/* Se tratan los clientes */
			for (i = 0; i < numeroClientes; i++) {
				if (FD_ISSET(socketCliente[i], &descriptoresLectura)) {
					printf("El cliente %d se fue de nuestro servidor\n",
							socketCliente[i]);
					close(socketCliente[i]);
					remove_element(socketCliente, i, numeroClientes);
					numeroClientes--;
					printf("CANTIDAD DE CLIENTES: %d\n", numeroClientes);
					/* Hay un error en la lectura. Posiblemente el cliente ha cerrado la conexión. Hacer aquí el tratamiento. En el ejemplo, se cierra el socket y se elimina del array de socketCliente[] */
				}
			}

			/* Se trata el socket servidor */
			if (FD_ISSET(socketServer, &descriptoresLectura)) {
				socketCliente[numeroClientes] = servidorConectarComponente(
						&socketServer, "planificador", "esi");
				printf("El cliente %d acaba de ingresar a nuestro servidor\n",
						socketCliente[numeroClientes]);
				numeroClientes++;
				printf("CANTIDAD DE CLIENTES: %d\n", numeroClientes);
				/* Un nuevo cliente solicita conexión. Aceptarla aquí. En el ejemplo, se acepta la conexión, se mete el descriptor en socketCliente[] y se envía al cliente su posición en el array como número de cliente. */
			}
		}
	}

	while (numeroClientes != 0) {
		printf("El cliente %d fue finalizado por comando (quit).\n",
				socketCliente[numeroClientes - 1]);
		close(socketCliente[numeroClientes - 1]);
		numeroClientes--;
	}

	close(socketServer);
}

int main() {
	puts("Iniciando Planificador.");
	pthread_t hiloConexiones;
	if (pthread_create(&hiloConexiones, NULL, (void *) tratarConexiones,
	NULL)) {

		fprintf(stderr, "Error creating thread\n");
		return 1;
	}
	iniciarConsola();
	if (pthread_join(hiloConexiones, NULL)) {

		fprintf(stderr, "Error joining thread\n");
		return 2;

	}
	puts("El Planificador se ha finalizado correctamente.");
	return 0;
}

//=======================COMANDOS DE CONSOLA====================================

int cmdQuit() {
	done = 1;
	return 0;
}

int cmdHelp() {
	register int i;
	puts("Comando:					Descripcion:");
	for (i = 0; comandos[i].cmd; i++) {
		printf("%s						%s\n", comandos[i].cmd, comandos[i].info);
	}
	return 0;
}

//=====================FUNCIONES DE CONSOLA=====================================

int existeComando(char* comando) {
	register int i;
	for (i = 0; comandos[i].cmd; i++) {
		if (strcmp(comando, comandos[i].cmd) == 0) {
			return i;
		}
	}
	return -1;
}

char *leerComando(char *linea) {
	char *comando;
	int i, j;
	int largocmd = 0;
	for (i = 0; i < strlen(linea); i++) {
		if (linea[i] == ' ')
			break;
		largocmd++;
	}
	comando = malloc(largocmd + 1);
	for (j = 0; j < largocmd; j++) {
		comando[j] = linea[j];
	}
	comando[j++] = '\0';
	return comando;
}

void obtenerParametros(char **parametros, char *linea) {
	int i, j;
	for (i = 0; i < strlen(linea); i++) {
		if (linea[i] == ' ')
			break;
	}
	(*parametros) = malloc(strlen(linea) - i);
	i++;
	for (j = 0; i < strlen(linea); j++) {
		if (linea[i] == '\0')
			break;
		(*parametros)[j] = linea[i];
		i++;
	}
	(*parametros)[j++] = '\0';
}

COMANDO *punteroComando(int posicion) {
	return (&comandos[posicion]);
}

int ejecutarSinParametros(COMANDO *comando) {
	return ((*(comando->funcion))());
}

int ejecutarConParametros(char *parametros, COMANDO *comando) {
	return ((*(comando->funcion))(parametros));
}

int verificarParametros(char *linea, int posicion) {
	int i;
	int espacios = 0;
	char *parametros;
	COMANDO *comando;
	for (i = 0; i < strlen(linea); i++) {
		if (linea[i] == ' ')
			espacios++;
	}
	if (comandos[posicion].parametros == espacios) {
		if (espacios == 0) {
			comando = punteroComando(posicion);
			ejecutarSinParametros(comando);
		} else {
			obtenerParametros(&parametros, linea);
			comando = punteroComando(posicion);
			ejecutarConParametros(parametros, comando);
			free(parametros);
		}
	} else {
		printf("%s: La cantidad de parametros ingresados es incorrecta.\n",
				comandos[posicion].cmd);
	}
	return 0;
}

void ejecutarComando(char *linea) {
	char *comando = leerComando(linea);
	int posicion = existeComando(comando);
	if (posicion == -1) {
		printf("%s: El comando ingresado no existe.\n", comando);
	} else {
		verificarParametros(linea, posicion);
	}
	free(comando);
}

char *recortarLinea(char *string) {
	register char *s, *t;
	for (s = string; whitespace(*s); s++)
		;
	if (*s == 0)
		return s;
	t = s + strlen(s) - 1;
	while (t > s && whitespace(*t))
		t--;
	*++t = '\0';
	return s;
}

void iniciarConsola() {
	char *linea, *aux;
	done = 0;
	while (done == 0) {
		linea = readline("user@planificador: ");
		if (!linea)
			break;
		aux = recortarLinea(linea);
		if (*aux)
			ejecutarComando(aux);
		free(linea);
	}
}
