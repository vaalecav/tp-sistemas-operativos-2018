/*
 ============================================================================
 Name        : instancia.c
 Author      : Los Simuladores
 Version     : Alpha
 Copyright   : Todos los derechos reservados
 Description : Proceso Instancia
 ============================================================================
 */
#include "instancia.h"

void freeEntrada(void* ent) {
	Entrada* entrada = (Entrada*)ent;
	free(entrada->clave);
	free(entrada->valor);
	free(ent);
}

void loguearEntrada(void* entr) {
	Entrada entrada = *(Entrada*)entr;
	log_info(logInstancia, "Clave: %s - Valor: %s - Entrada: %d - Ocupa: %d - Ultima vez usada: %d", entrada.clave, entrada.valor, entrada.primeraEntrada, entrada.cantidadEntradas, entrada.indexUltimaSentencia);
}

void mostrarEntrada(void* entr) {
	Entrada entrada = *(Entrada*)entr;
	printf("CLAVE %s\nEntrada: %d - %d\nValor: %s\n---------------------\n", entrada.clave, entrada.primeraEntrada, entrada.cantidadEntradas, entrada.valor);
}

void avisarAlCoordinador(int idMensaje) {
	enviarHeader(socketCoordinador, "", idMensaje);
}

void loguearInstancia() {
	char* entradasUsadas;

	// Obtengo la cantidad de entradas usadas como string
	entradasUsadas = malloc(estructuraAdministrativa.cantidadEntradas + 1);
	for (int i = 0; i < estructuraAdministrativa.cantidadEntradas; i++) {
		entradasUsadas[i] = '0' + estructuraAdministrativa.entradasUsadas[i];
	}
	entradasUsadas[estructuraAdministrativa.cantidadEntradas] = '\0';

	// Logueo la tabla de entradas
	log_info(logInstancia, "La tabla de entradas quedó: %s", entradasUsadas);
	list_iterate(estructuraAdministrativa.entradas, loguearEntrada);

	free(entradasUsadas);
}

void liberarMemoria(){
	free(info);
	config_destroy(configuracion);
	close(socketCoordinador);
	list_destroy_and_destroy_elements(estructuraAdministrativa.entradas, freeEntrada);
	log_destroy(logInstancia);
}

void cerrarInstancia(int sig) {
    terminar = 1;
	loguearInstancia();
	liberarMemoria();
	exit(1);
}

int recibirInformacionEntradas(int socketEmisor, InformacionEntradas* info) {
	int recibido;

	recibido = recv(socketEmisor, info, sizeof(InformacionEntradas), 0);
	if (recibido < 0) {
		return -1;
	} else if (recibido == 0) {
		close(socketEmisor);
		free(info);
		return 0;
	}

	return 1;
}

int buscarEspacioEnTabla(int entradasNecesarias) {
	int libre = -1;
	int cantidadSeguidos = 0;

	for (int i = 0; i < estructuraAdministrativa.cantidadEntradas; i++) {
		if (!estructuraAdministrativa.entradasUsadas[i]) {
			if (!cantidadSeguidos) {
				libre = i;
			}

			cantidadSeguidos++;

			if (cantidadSeguidos == entradasNecesarias) {
				break;
			}
		} else {
			libre = -1;
			cantidadSeguidos = 0;
		}
	}

	return libre;
}

int entradaEsIgualAClave(void* entrada, void* clave) {
	Entrada *ent = (Entrada*) entrada;
	return strcmp(ent->clave, (char*)clave)	== 0;
}

int cantidadDeEntradasLibres() {
	int entradasLibres = estructuraAdministrativa.cantidadEntradas;

	for (int i = 0; i < estructuraAdministrativa.cantidadEntradas; i++) {
		entradasLibres -= estructuraAdministrativa.entradasUsadas[i];
	}

	return entradasLibres;
}

int setearValor(char* clave, char* valor, int entradasNecesarias, int posicionParaSetear) {
	void* entradaVoid;
	Entrada *entrada;

	if (posicionParaSetear == -1) {
		posicionParaSetear = buscarEspacioEnTabla(entradasNecesarias);
	}

	// Marco las entradas como ocupadas
	for (int i = posicionParaSetear; i < posicionParaSetear + entradasNecesarias; i++) {
		estructuraAdministrativa.entradasUsadas[i] = 1;
	}

	if ((entradaVoid = list_find_with_param(estructuraAdministrativa.entradas, (void*)clave, entradaEsIgualAClave)) != NULL) {
		entrada = (Entrada*)entradaVoid;
		free(entrada->valor);
		free(entrada->clave);
	} else {
		entrada = malloc(sizeof(Entrada));
	}

	entrada->clave = malloc(strlen(clave) + 1);
	strcpy(entrada->clave, clave);
	entrada->clave[strlen(clave)] = '\0';
	entrada->valor = malloc(strlen(valor) + 1);
	strcpy(entrada->valor, valor);
	entrada->valor[strlen(valor)] = '\0';
	entrada->primeraEntrada = posicionParaSetear;
	entrada->cantidadEntradas = entradasNecesarias;
	entrada->indexUltimaSentencia = cantidadSentencias;
	cantidadSentencias++;

	if (entradaVoid == NULL) {
		list_add(estructuraAdministrativa.entradas, (void*)entrada);
	}

	// Logueo que setteo el valor
	log_trace(logInstancia, "Setteo %s: %s, en la entrada %d con un largo de entradas %d", entrada->clave, entrada->valor, entrada->primeraEntrada, entrada->cantidadEntradas);

	/*
	PARA DEBUGGEAR

	list_iterate(estructuraAdministrativa.entradas, mostrarEntrada);
	puts("");
	for(int i = 0; i < estructuraAdministrativa.cantidadEntradas; i++) {
		printf("Entrada %d: %s\n", i, (estructuraAdministrativa.entradasUsadas[i] ? "usada" : "libre"));
	}
	puts("");
	puts("=====================");
	puts("");
	*/

	return INSTANCIA_SENTENCIA_OK_SET;
}

int cantidadEntradasPosiblesContinuas() {
	int cantidadContinuasMax = 0;
	int cantidadContinuas = 0;
	for (int i = 0; i < estructuraAdministrativa.cantidadEntradas; i++) {
		if (estructuraAdministrativa.entradasUsadas[i]) {
			cantidadContinuasMax = max(cantidadContinuasMax, cantidadContinuas);
			cantidadContinuas = 0;
		} else {
			cantidadContinuas++;
		}
	}

	cantidadContinuasMax = max(cantidadContinuasMax, cantidadContinuas);
	return cantidadContinuasMax;
}

int tieneElIndexYEsAtomico(void* entradaVoid, void* indexVoid) {
	Entrada* entrada = (Entrada*)entradaVoid;
	int index = *(int*)indexVoid;
	
	return entrada->primeraEntrada == index && entrada->cantidadEntradas == 1;
}

bool instanciaLRU(void* entrada1, void* entrada2) {
	// Sobre la que hace más tiempo se hizo una sentencia
	return ((Entrada*)entrada1)->indexUltimaSentencia < ((Entrada*)entrada2)->indexUltimaSentencia;
}

bool instanciaBSU(void* entrada1, void* entrada2) {
	// El que más espacio ocupe
	return strlen(((Entrada*)entrada1)->valor) > strlen(((Entrada*)entrada2)->valor);
}

void ejecutarAlgoritmoDeRemplazo() {
	char* algoritmo;
	void* entradaVoid;
	Entrada* entrada;

	algoritmo = config_get_string_value(configuracion, "ALG_REMP");


	if (strcmp(algoritmo, "CIRC") == 0) {
		// Logueo que ejecuto el algoritmo de remplazo
		log_trace(logInstancia, "Ejecuto algoritmo de remplazo circular");

		// Elimino el item de la lista que tenga el index y sea atómico
		entradaVoid = list_remove_by_condition_with_param(estructuraAdministrativa.entradas, (void*)(&indexCirc), tieneElIndexYEsAtomico);

		// Aumento el index
		indexCirc++;

		// Si el index se pasó de la cantidad de entradas, vuelvo a 0
		if (indexCirc > estructuraAdministrativa.cantidadEntradas) {
			indexCirc = 0;
		}
	} else if (strcmp(algoritmo, "LRU") == 0 || strcmp(algoritmo, "BSU") == 0) {
		// LRU y BSU solo difieren en la forma que se ordena la lista

		if (strcmp(algoritmo, "LRU") == 0) {
			// Logueo que ejecuto el algoritmo de remplazo
			log_trace(logInstancia, "Ejecuto algoritmo de remplazo LRU");

			// Ordeno la lista según LRU
			list_sort(estructuraAdministrativa.entradas, instanciaLRU);
		
		} else if (strcmp(algoritmo, "BSU") == 0) {
			// Logueo que ejecuto el algoritmo de remplazo
			log_trace(logInstancia, "Ejecuto algoritmo de remplazo BSU");
			
			// Ordeno la lista según BSU
			list_sort(estructuraAdministrativa.entradas, instanciaBSU);
		}

		// Verifico que el primero sea atómico
		int indexRemover = 0;
		int terminarRemplazo = 0;
		Entrada* entradaPrueba;

		do {
			entradaPrueba = ((Entrada*)list_get(estructuraAdministrativa.entradas, indexRemover));
			if (entradaPrueba == NULL) {
				entradaVoid = NULL;
				terminarRemplazo = 1;
			} else if (entradaPrueba->cantidadEntradas == 1) {
				entradaVoid = list_remove(estructuraAdministrativa.entradas, indexRemover);
				terminarRemplazo = 1;
			}

			indexRemover++;
		} while (!terminarRemplazo);
	}

	if (entradaVoid != NULL) {
		entrada = (Entrada*)entradaVoid;

		// Logueo la entrada que repmplazo
		log_trace(logInstancia, "Reemplazo la entrada %s", entrada->clave);

		// Pongo en la tabla de entradas que el espacio está libre (solo toma atomicas, asi que no me preocupo por desocupar las demas)
		estructuraAdministrativa.entradasUsadas[entrada->primeraEntrada] = 0;

		// Libero el espacio que la entrada ocupaba
		freeEntrada(entradaVoid);
	}
}

int setearClave(char* clave, char* valor) {
	Entrada* entrada;
	void* entradaVoid;
	int entradasNecesarias;
	int entradaGuardar = -1;
	ContentHeader *headerCompactar;

	// Verifico si alcanzan las entradas
	entradasNecesarias = divCeil(strlen(valor), estructuraAdministrativa.tamanioEntrada);

	if (entradasNecesarias > estructuraAdministrativa.cantidadEntradas) {
		// Logueo el error
		log_error(logInstancia, "El valor no entra en la tabla de entradas");
		return INSTANCIA_ERROR;
	}
	
	entradaVoid = list_find_with_param(estructuraAdministrativa.entradas, (void*)clave, entradaEsIgualAClave);

	// Verifico si hay que compactar
	if (entradaVoid == NULL && cantidadEntradasPosiblesContinuas() < entradasNecesarias && cantidadDeEntradasLibres() >= entradasNecesarias) {

		// Le aviso al coordinador que hay que compactar
		avisarAlCoordinador(COMPACTAR);

		// Espero a que el coordinador me diga que compacte
		headerCompactar = recibirHeader(socketCoordinador);

		// Si el coordinador me dice que compacte, lo hago
		if (headerCompactar->id == COMPACTAR) {
			compactar();
		}
	} else {
		// Le aviso al coordinador que no hace falta compactar
		avisarAlCoordinador(0);
	}

	// Verifico si ya está seteada la clave
	if (entradaVoid != NULL) {
		entrada = (Entrada*)entradaVoid;

		// Logueo que la entrada ya existe
		log_trace(logInstancia, "La clave ya está seteada en la instancia");

		// Verifico que la cantidad de entradas necesarias no sea mayor que la actual
		if (entradasNecesarias > entrada->cantidadEntradas) {
			return INSTANCIA_ERROR;

		}

		// Marco la entrada en la que se tiene que guardar y libero todas
		entradaGuardar = entrada->primeraEntrada;
		for (int i = entrada->primeraEntrada; i < entrada->primeraEntrada + entrada->cantidadEntradas; i++) {
			estructuraAdministrativa.entradasUsadas[i] = 0;
		}
	} else {
	
		// Si no está seteada, verifico si hay espacio continuo disponible
		while (cantidadEntradasPosiblesContinuas() < entradasNecesarias) {
			ejecutarAlgoritmoDeRemplazo();
		}
	}

	// Setteo porque hay lugar
	return setearValor(clave, valor, entradasNecesarias, entradaGuardar);
}

int storeClave(char* clave) {
	void* entradaVoid;
	Entrada* entrada;
	char* pto_montaje;
	char* nombreArchivo;
	FILE* archivo;

	// Valido si existe la entrada
	if ((entradaVoid = list_find_with_param(estructuraAdministrativa.entradas, (void*)clave, entradaEsIgualAClave)) == NULL) {
		log_error(logInstancia, "No existe la entrada setteada en la instancia");
		return INSTANCIA_CLAVE_NO_IDENTIFICADA;
	}

	// Obtengo la dirección donde se va a guardar
	pto_montaje = config_get_string_value(configuracion, "PUNTO_MONTAJE");

	// Si no existe la creo (si ya existe, no pasa nada)
	mkdir(pto_montaje, S_IRWXU);

	// Genero la dirección del archivo
	nombreArchivo = malloc(strlen(pto_montaje) + strlen(clave) + 1);
	strcpy(nombreArchivo, pto_montaje);
	strcat(nombreArchivo, clave);
	nombreArchivo[strlen(pto_montaje) + strlen(clave)] = '\0';

	// Imprimo el valor en el archivo
	if ((archivo = fopen(nombreArchivo, "w"))) {
		entrada = (Entrada*)entradaVoid;
		entrada->indexUltimaSentencia = cantidadSentencias;
		cantidadSentencias++;
		fprintf(archivo, "%s", entrada->valor);
		fclose(archivo);

		// Logueo que settee
		log_trace(logInstancia, "Hago store de la clave %s del valor %s en %s", entrada->clave, entrada->valor, nombreArchivo);
	} else {
		log_error(logInstancia, "No se puede acceder al archivo de la clave para hacer store del valor");
	}

	free(nombreArchivo);
	return INSTANCIA_SENTENCIA_OK_STORE;
}

void recibirSentencia() {
	char* mensaje;
	char** mensajeSplitted;
	ContentHeader *header;
	int respuesta;
	char* nombreClave;
	void* entradaVoid;
	Entrada* entrada;

	// Recibo el mensaje del coordinador
	header = recibirHeader(socketCoordinador);

	// Cuando me llego una sentencia no dejo que se hagan DUMPs
	pthread_mutex_lock(&mutexDump);

	if (header->id == COORDINADOR) {
		mensaje = malloc(header->largo + 1);
		recibirMensaje(socketCoordinador, header->largo, &mensaje);

		// Logueo que se recibio una sentencia
		log_trace(logInstancia, "Se recibió una sentencia: %s", mensaje);

		mensajeSplitted = string_split(mensaje, " ");

		if (strcmp(mensajeSplitted[0], "SET") == 0) {
			respuesta = setearClave(mensajeSplitted[1], mensajeSplitted[2]);

			// Si no salió todo OK, le tengo que avisar al coordinador que no compacte
			if (respuesta == INSTANCIA_SENTENCIA_OK_SET) {
				loguearInstancia();
			} else {
				avisarAlCoordinador(0);
			}
		} else if (strcmp(mensajeSplitted[0], "STORE") == 0) {
			respuesta = storeClave(mensajeSplitted[1]);
		} else {
			respuesta = INSTANCIA_ERROR;
			log_error(logInstancia, "Error en la sentencia");
		}

		avisarAlCoordinador(respuesta);

		if (respuesta == INSTANCIA_SENTENCIA_OK_SET) {
			// Le aviso al coordinador la cantidad de entradas libres
			avisarAlCoordinador(cantidadDeEntradasLibres());
		}

		free(mensaje);
		free(mensajeSplitted[0]);
		free(mensajeSplitted[1]);
		free(mensajeSplitted[2]);
		free(mensajeSplitted);

	} else if (header->id == COMANDO_STATUS){
		nombreClave = malloc(header->largo + 1);
		recibirMensaje(socketCoordinador, header->largo, &nombreClave); // Recibo nombre de la clave
		nombreClave[header->largo] = '\0';
		// Valido si existe la entrada con la clave
		if ((entradaVoid = list_find_with_param(estructuraAdministrativa.entradas, (void*)nombreClave, entradaEsIgualAClave)) == NULL) {
			// No existe entrada entonces la clave no posee un valor, se lo comunico al coordinador
			enviarHeader(socketCoordinador, "", COMANDO_STATUS_VALOR_CLAVE_NULL);
		} else {
			// Existe entrada entonces le paso el valor de la clave al coordinador
			entrada = (Entrada*)entradaVoid;
			enviarHeader(socketCoordinador, entrada->valor, COMANDO_STATUS_VALOR_CLAVE_OK);
			enviarMensaje(socketCoordinador, entrada->valor);
		}
		free(nombreClave);
	} else if (header->id == COMPACTAR) {
		compactar();
	}

	free(header);

	// Al terminar, permito que se sigan haciendo DUMPs
	pthread_mutex_unlock(&mutexDump);
}

void compactar() {
	void *entradaVoid;
	Entrada *entrada;

	// Logueo que voy a compactar
	log_trace(logInstancia, "Voy a compactar");

	// Para compactar voy a necesitar recorrer toda la tabla de entradas
	for (int iCompactacion = 0; iCompactacion < estructuraAdministrativa.cantidadEntradas; iCompactacion++) {

		// Verifico si la entrada esta libre
		if (estructuraAdministrativa.entradasUsadas[iCompactacion] == 0) {

			// Si la entrada está libre, busco la primera que no lo esté
			for (int iEntradaOcupada = iCompactacion; iEntradaOcupada < estructuraAdministrativa.cantidadEntradas; iEntradaOcupada++) {
				if (estructuraAdministrativa.entradasUsadas[iEntradaOcupada] == 1) {

					// Cuando encuentro una libre, obtengo la entrada que ocupa ese index
					entradaVoid = list_find_with_param(estructuraAdministrativa.entradas, (void*)&iEntradaOcupada, entradaOcupaEspacio);

					// Este if debería dar siempre true, excepto que haya sucedido alguna inconsistencia
					if (entradaVoid != NULL) {
						entrada = (Entrada*)entradaVoid;

						// Marco todos los espacios que ocupa la entrada como disponible
						for (int iLiberandoEntradas = iEntradaOcupada; iLiberandoEntradas < iEntradaOcupada + entrada->cantidadEntradas; iLiberandoEntradas++) {
							estructuraAdministrativa.entradasUsadas[iLiberandoEntradas] = 0;
						}

						// Marco los espacios que ocupará ahora como ocupados
						for (int iOcupandoEntradas = iCompactacion; iOcupandoEntradas < iCompactacion + entrada->cantidadEntradas; iOcupandoEntradas++) {
							estructuraAdministrativa.entradasUsadas[iOcupandoEntradas] = 1;
						}

						// Le actualizo donde está guardada la entrada para que sea consistente
						entrada->primeraEntrada = iCompactacion;

						// Corto el for que está buscando la entrada ocupada
						break;
					} else {
						log_warning(logInstancia, "********** ERROR DE INCONSISTENCIA AL COMPACTAR **********");
					}
				}
			}
		}
	}

	// Muestro como quedó la instancia después de la compactación
	loguearInstancia();
}

int entradaOcupaEspacio(void* entrada, void* espacio) {
	return ((Entrada*)entrada)->primeraEntrada == *(int*)espacio;
}

char* obtenerValorDelArchivo(const char* path_archivo) {
    char *valor;
    long tamanio;
    FILE *archivo;

    // Abro el archivo
    archivo = fopen(path_archivo, "r");
    if (archivo == NULL) return NULL;

    // Obtengo el tamanio del archivo para hacer el malloc
    fseek(archivo, 0, SEEK_END);
    tamanio = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);
    valor = malloc(tamanio + 1);

    // Leo el valor
	fread(valor, 1, tamanio, archivo);
	valor[tamanio] = '\0';

	// Cierro el archivo
    fclose(archivo);

    return valor;
}

void reincorporarInstancia() {
	DIR *dir;
	struct dirent *ent;
	int entradasNecesarias;
	char* valor;
	char* path_absoluto;
	char* punto_montaje = config_get_string_value(configuracion, "PUNTO_MONTAJE");
	int reincorporeInstancia = 0;

	// Trato de abrir el directorio
	if ((dir = opendir(punto_montaje)) == NULL) {
		log_trace(logInstancia, "No hay nada que reincorporar");
		return;
	}

	// Logueo que el punto de montaje existe
	log_trace(logInstancia, "El punto de montaje <%s> existe.", punto_montaje);

	// Recorro todos los archivos
	while ((ent = readdir(dir)) != NULL) {
		if (ent->d_type == DT_REG) {
			// Creo el path absoluto del archivo
			path_absoluto = malloc(strlen(punto_montaje) + strlen(ent->d_name) + 1);
			sprintf(path_absoluto, "%s%s", punto_montaje, ent->d_name);
			path_absoluto[strlen(punto_montaje) + strlen(ent->d_name)] = '\0';

			// Obtengo el valor del archivo
			valor = obtenerValorDelArchivo(path_absoluto);

			if (valor != NULL) {
				// Obtengo la cantidad de entradas necesarias
				entradasNecesarias = divCeil(strlen(valor), estructuraAdministrativa.tamanioEntrada);

				// Seteo el valor
				setearValor(ent->d_name, valor, entradasNecesarias, -1);

				// Marco que la instancia se reincorporó
				reincorporeInstancia = 1;

				// Libero memoria
				free(valor);
			}

			// Libero memoria
			free(path_absoluto);
		}
	}

	// Cierro el directorio
	closedir(dir);

	// Logueo la reincorporación
	if (reincorporeInstancia) {
		log_trace(logInstancia, "Reincorporé la instancia");
		loguearInstancia();
	}
}

void dumpEntradas(void* entradaVoid){
	Entrada* entrada = (Entrada*) entradaVoid;
	// No checkeo valor que devuelve ya que no aplica en este caso
	storeClave(entrada->clave);
}

void realizarDump(int intervaloDump){
	while(!terminar) {
		// Hago sleep del tiempo indicado por configuración
		sleep(intervaloDump);
		
		// Bloqeo mutex y cuando termino desbloqueo (no puede hacer otras cosas mientras hace un DUMP)
		pthread_mutex_lock(&mutexDump);
		
		// Itera por todas las entradas guardando los valores
		list_iterate(estructuraAdministrativa.entradas, dumpEntradas);
		pthread_mutex_unlock(&mutexDump);

		log_trace(logInstancia, "Hice un DUMP");
	}
}

int correrEnHilo(int intervaloDump){
	pthread_t idHilo;

	if (pthread_create(&idHilo, NULL, (void*) realizarDump, (void*) intervaloDump)) {
		log_error(logInstancia, "No se pudo crear el hilo para realizar el Dump");
		return 0;
	}
	log_trace(logInstancia, "Hilo asignado para realizar el Dump");
	return 1;
}

int main() {
	char* ipCoordinador;
	int puertoCoordinador;
	char* nombre;
	int intervaloDump; //en segundos

	// Inicio el log
		logInstancia = log_create(ARCHIVO_LOG, "Instancia", LOG_PRINT, LOG_LEVEL_TRACE);


	// Instancio las cosas necesarias para saber cuando se cierra la entrada
		terminar = 0;
		signal(SIGTSTP, &cerrarInstancia);

	// Archivo de configuracion
		configuracion = config_create(ARCHIVO_CONFIGURACION);

		puertoCoordinador = config_get_int_value(configuracion, "PUERTO_COORDINADOR");
		ipCoordinador = config_get_string_value(configuracion, "IP_COORDINADOR");
		nombre = config_get_string_value(configuracion, "NOMBRE");
		intervaloDump = config_get_int_value(configuracion, "INTERVALO_DUMP");

	// Instancio las cosas necesarias para los algoritmos de remplazo
		indexCirc = 0;
		cantidadSentencias = 0;

	// Conexion con el coordinador
		log_trace(logInstancia, "Envio al coordinador el nombre de mi instancia: %s", nombre);

		socketCoordinador = clienteConectarComponente("instancia", "coordinador", puertoCoordinador, ipCoordinador);
		enviarHeader(socketCoordinador, nombre, INSTANCIA);
		int largo = strlen(nombre);
		enviarInformacion(socketCoordinador, nombre, &largo);

	// Instancio todas las estructuras administrativas
		// Espera hasta que recive la informacion de lo que será la tabla de entradas
		info = (InformacionEntradas*) malloc(sizeof(InformacionEntradas));

		recibirInformacionEntradas(socketCoordinador, info);

		estructuraAdministrativa.cantidadEntradas = info->cantidad;
		estructuraAdministrativa.tamanioEntrada = info->tamanio;

		// A todas las entradas las inicio como vacías (0)
		estructuraAdministrativa.entradasUsadas = malloc(sizeof(int) * info->cantidad);
		for (int i = 0; i < info->cantidad; i++) {
			estructuraAdministrativa.entradasUsadas[i] = 0;
		}

		// Creo la lista de claves por primera vez
		estructuraAdministrativa.entradas = list_create();

		// Logueo la operacion de la estructura administrativa
		log_trace(logInstancia, "Se conectó el coordinador e instanció la estructura administrativa: Cant. entradas=%d; Tam. entrada=%d;", estructuraAdministrativa.cantidadEntradas, estructuraAdministrativa.tamanioEntrada);

		// Si la tabla de entradas está creada, la reincorporo
		reincorporarInstancia();

		// Le aviso al coordinador la cantidad de entradas libres
		enviarHeader(socketCoordinador, "", cantidadDeEntradasLibres());

	// Antes de empezar a recibir sentencias, inicio el DUMP en un thread
		if (!correrEnHilo(intervaloDump)) {
			liberarMemoria();
			return 0;
		}

	// Espero las sentencias
		log_trace(logInstancia, "Espero sentencias");
		while(!terminar) {
			recibirSentencia();
		}

	return 0;
}
