#include "general.h"
//Memoria compartida por todos los actores
sem_t* sem_carritos;
sem_t* sem_productos;
int memoria1, memoria2;
//Id de la cola de mensajes
int msqid;

//variables globales
sem_t mutex_peticiones;
sem_t mutex_archivo_clientes;
//Inicia los semaforos que son memoria compartida entre procesos.
void iniciarSemaforos(){
    key_t llave1, llave2;
    llave1=ftok(ARCH_SEM_CAR,INT_SEM_CAR);
    llave2=ftok(ARCH_SEM_PRO,INT_SEM_PRO);
    memoria1=shmget(llave1,sizeof(sem_t),IPC_CREAT|0600);
    if(memoria1 == -1) return;
    sem_carritos=shmat(memoria1,0,0);
    //Obtiene el semaforo para el archivo carritos de compra
    memoria2=shmget(llave2,sizeof(sem_t),IPC_CREAT|0600);
    if( memoria2 == -1) return;
    sem_productos=shmat(memoria2,0,0);

    sem_init(sem_carritos, 1, 1);
    sem_init(sem_productos, 1, 1);
    puts("Semaforos iniciados");
    return;
}
//Libera la memoria de los semaforos
void cerrarSemaforos(){
    sem_destroy(sem_productos);
    sem_destroy(sem_carritos);
    shmdt((void*)sem_carritos);
    shmctl(memoria1, IPC_RMID, (struct shmid_ds*)NULL);
    shmdt((void*)sem_productos);
    shmctl(memoria2, IPC_RMID, (struct shmid_ds*)NULL);
    puts("Semaforos liberados");
    return;
}
//Registra un cliente
int registroCliente(const char* nombre_usuario, const char* password){
    //Verifica los apuntadores
    if( nombre_usuario == NULL || password == NULL )
        return AP_INV;
    //validacion de las cadenas
    int nombre_long = validarCadena(nombre_usuario);
    int pass_long = validarCadena(password);
    if( nombre_long < TAM_MIN_NOM_USU || nombre_long > TAM_MAX_NOM_USU )
        return CAD_INV;
    if( pass_long < TAM_MIN_PASS || pass_long > TAM_MAX_PASS )
        return CAD_INV;
    
    //Creamos la linea del cliente a escribir en el txt; se suman los separadores
    char* linea_en_archivo = (char*)malloc(sizeof(char)*(nombre_long + pass_long + NUM_SEP_LIN_CLI));
    sprintf(linea_en_archivo, "%s%c%s%c",nombre_usuario,C_SEPARADOR,password,C_SEPARADOR);
    //Activamos el semaforo para tener los id correctos
    sem_wait(&mutex_archivo_clientes);
    int id_nuevo_cliente = obtUltimoId(ARCH_CLI, NUM_SEP_LIN_CLI) + 1;
    
    if(id_nuevo_cliente == ERROR_ARCH){
        sem_post(&mutex_archivo_clientes);
        free(linea_en_archivo);
        return ERROR_ARCH;
    }
    
    if( escribirRegistroArchivo( id_nuevo_cliente, linea_en_archivo, ARCH_CLI ) != OK ){
        sem_post(&mutex_archivo_clientes);
        free(linea_en_archivo);
        return ERROR_ARCH;
    }
    sem_post(&mutex_archivo_clientes);
    
    free(linea_en_archivo);
    return OK;
}
//Busca el usuario y password en el archivo de clientes.
//REcibe Strings y regresa el id del cliente si se encuentra.
int buscarCliente(const char* nombre_usuario, const char* password){
    if( nombre_usuario == NULL || password == NULL ) return AP_INV;
    //Longitud de una linea del archivo clientes. Se sumanlos separadores y otros 10
    //por lo grande que pueda ser el id
    int long_linea = TAM_MAX_NOM_USU + TAM_MAX_PASS + NUM_SEP_LIN_CLI + 10;
    char* linea_leida = (char*)malloc(sizeof(char)*long_linea);
    char* char_id = (char*)malloc(sizeof(char)*10);
    if( linea_leida == NULL || char_id == NULL ) return ERROR_EN_MEMORIA;
    
    //Ayuda a recorrer las cadenas
    char* aux;
    
    int id, long_usuario, long_pass, i;
    long_usuario = strlen(nombre_usuario);
    long_pass = strlen(password);

    //Activamos el semaforo
    sem_wait(&mutex_archivo_clientes);

    //Se busca el nombre y pass recibido dentro del archivo
    FILE* archivo;
    archivo = fopen(ARCH_CLI, "a+");
    if (archivo == NULL){
        sem_post(&mutex_archivo_clientes);
        free(linea_leida);
        free(char_id);
        return ERROR_ARCH;
    } 
    
    while (fgets(linea_leida, long_linea, archivo) != NULL){
        //La linea del archivo tiene el formato :id:nombre:password:
        aux = linea_leida;    
        i = 0;    
        //Recorremos la linea y guardamos los datos.
        aux++;
        //Obtenemos el id.
        while(*aux != C_SEPARADOR ){
            char_id[i] =*aux; 
            aux++;
            i++;
        }
        id = atoi(char_id);
        i = 0;
        //Saltamos el separador
        aux++;
        //Comparamos usuario
        i = 0;
        while(*aux != C_SEPARADOR &&*aux == nombre_usuario[i]){
            aux++;
            i++;  
        }
        //Si el usuario coincide comparamos la pass. Se compara que se haya 
        //leido todo el usuario de la linea y que la longitud de cadenas
        //sea la misma
        if(*aux == C_SEPARADOR && i == long_usuario ){
            //Salta el separador
            aux++;
            i = 0;
            //Recorre la pass del archivo y la compara con la
            //recibida
            while(*aux != C_SEPARADOR &&*aux == password[i]){
                aux++;
                i++;
            }

            //Si coinciden las passwords, regresa el id del usuario.
            //Compara que se haya leido toda la pass y que la longitud de cadenas
            //sea la misma
            if(*aux == C_SEPARADOR && i == long_pass ){
                fclose(archivo);
                sem_post(&mutex_archivo_clientes);
                free(linea_leida);
                free(char_id);
                return id;
            }
        } 
        
    }
    sem_post(&mutex_archivo_clientes);
    fclose(archivo);
    free(linea_leida);
    free(char_id);
    return CLI_NO_ENCONTRADO;
}
//Recibe Strings y el id a donde se guarda el id.
int iniciarSesion(const char* nombre_usuario, const char* password, int* id){
    //Verifica los apuntadores
    if( nombre_usuario == NULL || password == NULL )
        return AP_INV;
    //validacion de las cadenas
    int nombre_long = validarCadena(nombre_usuario);
    int pass_long = validarCadena(password);
    if( nombre_long < TAM_MIN_NOM_USU || nombre_long > TAM_MAX_NOM_USU )
        return CAD_INV;
    if( pass_long < TAM_MIN_PASS || pass_long > TAM_MAX_PASS )
        return CAD_INV;

    int estado = buscarCliente(nombre_usuario, password);
    if(estado == ERROR_ARCH || estado == CLI_NO_ENCONTRADO)
        return estado;
   *id = estado;
    return OK;
}
//Encola mensajes en la cola
int contestarPeticion(mensaje* msj ){
    if( msj == NULL )
        return AP_INV;
    //Manda mensaje
    if(msgsnd(msqid, msj, LONGITUD, IPC_NOWAIT) == -1){
        perror("msgsnd");
        return ERROR_COLA;
    }
    // El tipo del mensaje es el numero unico del cliente (pid)
    printf("peticion contestada al cliente: %ld; estado: %d\n", msj->tipo, msj->carga.estado);

    return OK;
        
}
//Hilo que se encarga de manejar la peticion. Recibe un apuntador de una estructura
//mensaje.
void* Hilo_manejador_peticion(void* args){
    //Se crea una copia del mensaje que se recibe
    sem_wait(&mutex_peticiones);
    mensaje* param = (mensaje*) args;
    mensaje* msj = (mensaje*)malloc(sizeof(mensaje));
 
    if(msj == NULL){
        perror("malloc");
        sem_post(&mutex_peticiones);
        free(msj);
        exit(1);
    } 
    //Mapeamos los datos del parametro al mensaje local
    strcpy(msj->carga.nombre_usuario, param->carga.nombre_usuario);
    strcpy(msj->carga.password, param->carga.password); 
    msj->carga.num_cliente = param->carga.num_cliente;
    msj->carga.tipo_peticion = param->carga.tipo_peticion;
    sem_post(&mutex_peticiones);
    
    mensaje* msj_retorno = (mensaje*)malloc(sizeof(mensaje));
    if(msj_retorno == NULL) { 
        perror("malloc");
        exit(-1);
    }
    //Se prepara la respuesta a la peticion 
    msj_retorno->tipo = msj->carga.num_cliente;
    msj_retorno->carga.llave_carritos = SIN_LLAVE;
    msj_retorno->carga.llave_productos = SIN_LLAVE;
    msj_retorno->carga.id_usuario = USU_NO_LOG;
    
    if( msj->carga.tipo_peticion == REGISTRO ){
        int estado = registroCliente(msj->carga.nombre_usuario, msj->carga.password);
        //Retornamos el mensaje
        msj_retorno->carga.estado = estado;
        if ( (estado = contestarPeticion(msj_retorno)) != OK)
            printf("----%s-----\n", cadenaError(estado));
    }
    else if( msj->carga.tipo_peticion == INI_SESION ){
        int id = 0;
        int estado = iniciarSesion(msj->carga.nombre_usuario, msj->carga.password, &id);
        msj_retorno->carga.estado = estado;
        if( estado == OK ){
            msj_retorno->carga.id_usuario = id;
            //Mandamos las llaves creadas para acceder a los semaforos.
            msj_retorno->carga.llave_carritos = ftok(ARCH_SEM_CAR, INT_SEM_CAR);
            msj_retorno->carga.llave_productos = ftok(ARCH_SEM_PRO, INT_SEM_PRO);
        }                
        if ( ( estado = contestarPeticion(msj_retorno) ) != OK)
           printf("----%s-----\n", cadenaError(estado));
    }
    free(msj);
    free(msj_retorno);
    
    pthread_exit(NULL);
}
//Hilo que lee los mensajes de la cola de mensajes
void* Hilo_lector(void* args){    
    int* bandera = (int*)args;  
    mensaje* msj = (mensaje*)malloc(sizeof(mensaje));
    if(msj == NULL) perror("malloc");
    int estado;
    //Se ejecuta todo el rato esperando peticiones
    while(*bandera != 0 ){
        //lee mensaje
        if(msgrcv(msqid, msj, LONGITUD, TYPMSG_USUARIO, IPC_NOWAIT) != -1){
            //Crea un hilo que se encarga de responder la peticion
            setvbuf (stdout, NULL, _IONBF, 0);
            pthread_t hilo_nuevo;
            pthread_create(&hilo_nuevo,NULL, Hilo_manejador_peticion, (void*)msj);              
        }
        
    }
    free(msj);
    pthread_exit(NULL);
}
//Se encarga de leer la entrada del usuario
void* Hilo_entrada_usuario( void* args ){
    int* bandera = (int*)args;
    do{
        puts("Para cerrar el servidor ingresa el 0.");
        scanf("%d", bandera);
    }while(*bandera != 0);
    pthread_exit(NULL);
}

int main(){
    //Bandera de ejecucion para el hilo lector de la cola
    int flag = TRUE;
    pthread_t hilo_lector, hilo_entrada;
    //Creacion de la cola de mensajes
    key_t llave; 
    llave = ftok("Prueba",'k');
    msqid = msgget(llave, IPC_CREAT | 0666);
    if(msqid == -1){
        perror("msgget");
        exit(-1);
    } 
    //Inicia los semaforos de hilos y archivos
    sem_init(&mutex_peticiones, 0, 1);
    sem_init(&mutex_archivo_clientes, 0, 1);
    iniciarSemaforos();
    pthread_create(&hilo_lector,NULL,Hilo_lector, (void*)&flag);
    pthread_create(&hilo_entrada,NULL,Hilo_entrada_usuario, (void*)&flag);
    pthread_join(hilo_lector,NULL);
    pthread_join(hilo_entrada,NULL);
    //Cierra la cola y los semaforos
    msgctl(msqid, IPC_RMID, (struct msqid_ds*)NULL);
    cerrarSemaforos();
    return 0;
}