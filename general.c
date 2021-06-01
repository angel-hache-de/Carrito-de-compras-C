//Funciones que comparten los actores (clientes, control y proveedor)
#include "general.h"
//Devolver la cadena del error
char* cadenaError(int error){
    switch(error){
        case ERROR_ARCH: return "Ha ocurrido un error con un archivo"; 
        case CAD_INV: return "Una de las entradas no es valida";
        case AP_INV: return "Error de un apuntador";
        case ERROR_COLA: return "Hubo un error al contactar con la unidad de control";
        case ERROR_EN_MEMORIA: return "Ha ocurrido un error en la memoria";
        case CANT_NO_DISP: return "Cantidad ingresada no disponible";
        case CLI_NO_ENCONTRADO: return "Cliente no encontrado";
        case SNACK_NO_ENCONTRADO: return "No se ha encontrado el snack";
        default: return "Ha ocurrido un error";
    }
}
// Escribe un registro en el archivo correspondiente
int escribirRegistroArchivo(int id, char* cadena, char* archivo){
    int estado;
    FILE* fp;
    fp = fopen (archivo,"a");
    if(fp == NULL) return ERROR_ARCH;
    //Escribe la linea al final del archivo
    //Forma: :id:cadena
    estado = fprintf (fp, "%c%d%c%s\n",C_SEPARADOR,id,C_SEPARADOR,cadena);
    /* close the file*/  
    fclose (fp);
    return estado < 0 ? ERROR_ARCH : OK;
}
//Escribe un mensaje en la cola de mensajes
int mandarPeticion(int* msqid, mensaje* msj ){
    if( msqid == NULL || msj == NULL )
        return AP_INV;
    //Manda mensaje
    if(msgsnd(*msqid, msj, LONGITUD, IPC_NOWAIT) == -1){
        perror("msgsnd");
        return ERROR_COLA;
    }
    return OK;
}
//Recibe un mensaje de la cola de mensajes
int leerRespuesta(int* msqid, mensaje* msj, int num_cliente){
    if( msqid == NULL || msj == NULL )
        return AP_INV;
    //Manda mensaje
    if(msgrcv(*msqid, msj, LONGITUD, num_cliente, 0) == -1){
        perror("msgrcv");
        exit(1);
    }
    return OK;       
}
//Convierte una linea del txt es una estructura snack
int convLineaSnack(char* linea, snack** producto){
    if( linea == NULL || producto == NULL ||*producto == NULL ) return AP_INV;
    char* aux = linea;
    //Obtenemos el id;
    char* char_id = (char*)malloc(sizeof(char)*10);
    char* char_existencia = (char*)malloc(sizeof(char)*10);
    char* char_precio = (char*)malloc(sizeof(char)*10);
    if( char_id == NULL || char_existencia == NULL || char_precio == NULL) 
        return ERROR_EN_MEMORIA;
    //Formato de la linea. : es SEPARADOR
    //:id:nombre:precio:existencia:\n
    
    //Saltamos el primer separador y obtenemos el id
    aux++;
    int i = 0;
    while(*aux != C_SEPARADOR ){
        char_id[i] =*aux;
        i++;
        aux++;
    }
    (*producto)->id = atoi(char_id);
    //Saltamos el separador
    aux++;
    i = 0;
    //Obtenemos el nombre del snack
    while(*aux != C_SEPARADOR ){
        (*producto)->nombre_snack[i] =*aux;
        aux++;
        i++;
    }
    (*producto)->nombre_snack[i] = '\0';
    //Saltamos el separador
    aux++;
    //Obtenemos el precio
    i = 0;
    while(*aux != C_SEPARADOR ){
        char_precio[i] =*aux;
        i++;
        aux++;
    }
    (*producto)->precio = atoi(char_precio);
    //Saltamos el separador
    aux++;    
    //Obtenemos la cantidad disponible del producto
    i = 0;
    while(*aux != C_SEPARADOR ){
        char_existencia[i] =*aux;
        i++;
        aux++;
    }
    (*producto)->existencia = atoi(char_existencia);
    free(char_existencia);
    free(char_precio);
    free(char_id);
    return OK; 
}
//Busca la linea con el id recibido y regresa el apuntador al inicio de la linea
//Pensado para updates.
int buscarRegistro( FILE* archivo, char* id ){
    if( archivo == NULL || id == NULL ) return AP_INV;
    int long_linea = 150;
    char* linea_leida = (char*)malloc(sizeof(char)*long_linea);
    char* aux;
    int flag = 0, i = 0;
    //Buscamos la linea que empiece con el id
    //Formato de la linea. ':' es SEPARADOR. :id:...
    while (  flag != TRUE && fgets(linea_leida, long_linea, archivo) != NULL ){
        i = 0;
        aux = linea_leida;
        //Saltamos el primer separador
        aux++;
        //Comparamos el id.
        while(*aux == id[i] ){ aux++; i++; }
        //Si son iguales el id leido y el id recibido, regresamos el apuntador
        //al inicio de la linea
        if(*aux ==  C_SEPARADOR && id[i] == '\0' ){
            flag = TRUE;
            fseek(archivo, -strlen(linea_leida), SEEK_CUR);
        }
    }    
    free(linea_leida);
    return flag == TRUE ? OK : REGISTRO_NO_ENCONTRADO;
}
//Imprime los productos. Recibe el semaforo del archivo productos
int mostrarProductos(sem_t* sem_productos){
    if(sem_productos == NULL) return AP_INV;
    //Long de la linea en el archivo. 
    int long_linea = 120;
    char* linea_leida = (char*)malloc(sizeof(char)*long_linea);
    if( linea_leida == NULL ) return ERROR_EN_MEMORIA;
    //Activamos el semaforo y leemos el archivo;
    sem_wait(sem_productos);
    FILE* archivo;
    archivo = fopen(ARCH_PRO, "a+");
    if (archivo == NULL){
        sem_post(sem_productos);
        free(linea_leida);
        puts("Algo ha salido mal");
        return ERROR_ARCH;
    } 
    
    snack* producto = (snack*)malloc(sizeof(snack));
    if( producto == NULL ) return ERROR_EN_MEMORIA;
    producto->nombre_snack = (char*)malloc(sizeof(char)*100);
    char* char_id = (char*)malloc(sizeof(char)*5);
    char* char_precio = (char*)malloc(sizeof(char)*5);
    char* char_existencia = (char*)malloc(sizeof(char)*5);
    if( producto->nombre_snack == NULL || char_id == NULL || char_precio == NULL
        || char_existencia == NULL) {
        sem_post(sem_productos);
        free(linea_leida);
        fclose(archivo);
        return ERROR_EN_MEMORIA;
    }
    
    
    int estado, espacio_nombre = 50, i;
    //Imprimimos la tabla
    system("clear");
    printf("\t\t\t\tSNACKS CONGELADOS\n");
    printf("\tNumero  Nombre");
    //Imprime los espacios para darle lugar al nombre. 6 por la palabra nombre
    for( i = 6; i < espacio_nombre; i++) printf(" ");
    printf(" Precio\t Existencia\n");

    while (fgets(linea_leida, long_linea, archivo) != NULL){
        if ( ( estado = convLineaSnack(linea_leida, &producto) ) != OK ){
            sem_post(sem_productos);
            fclose(archivo);
            free(producto->nombre_snack);
            free(producto);
            free(linea_leida);
            free(char_id);
            free(char_existencia);
            free(char_precio);
            return estado;
        }
        sprintf(char_id, "%d", producto->id);
        sprintf(char_precio, "%d", producto->precio);
        sprintf(char_existencia, "%d", producto->existencia);
        //Le damos formato de tabla
        printf("\t%s",char_id);
        for(  i = strlen( char_id ); i < 8; i++) printf(" ");
        printf("%s", producto->nombre_snack);
        for(  i = strlen( producto->nombre_snack ); i < espacio_nombre; i++) printf(" ");
        printf(" $%s", char_precio);
        for(  i = strlen( char_precio ); i < 13; i++) printf(" ");
        printf("%s\n", char_existencia);
    }
    sem_post(sem_productos);
    fclose(archivo);
    free(producto->nombre_snack);
    free(producto);
    free(linea_leida);
    free(char_id);
    free(char_existencia);
    free(char_precio);
    return OK;
}
//Obtiene el ultimo id de un archivo. Recibe String con el nombre del txt y el numero de separadores
//Que tienen las lineas en el archivo.
int obtUltimoId(const char* nombre_archivo, int separadores_por_linea){
    if(nombre_archivo == NULL) return AP_INV;
    FILE* archivo;
    archivo = fopen(nombre_archivo,"a+");
    
    //Si no se encuentra el archivo, se crea.
    if(archivo == NULL)
        return ERROR_ARCH;
    
    //Verificamos que no este vacio el archivo. Si esta vacio regresamos
    //el id 0.
    char* aux = (char*)malloc(sizeof(char)*20);
    if(aux == NULL) {
        fclose(archivo);
        return ERROR_EN_MEMORIA;
    }
     
    strcpy(aux, "");
    fgets(aux, 20, archivo);
    if( strcmp( aux, "" ) == 0 ){
        free(aux);
        fclose(archivo);
        return 0;
    }
    free(aux);

    //Nos ponemos al final del archivo
    fseek( archivo, 0, SEEK_END );
    //Recorremos hasta llegar al principio del id.
    char c;
    int contador_separadores = 0;
    //Queremos llegar al inicio del id. //Ejemplo :id:...:...: el ap queda en la d.
    while( contador_separadores != separadores_por_linea - 1 ){
        c = fgetc(archivo);
        if( c == C_SEPARADOR )
            contador_separadores++;
        //Regresa 2 posiciones.
        fseek(archivo, -2, SEEK_CUR);
    }
    
    //Recorremos el id de derecha a izquierda. 
    int id = 0, exp = 0; 
    while( (c = fgetc(archivo)) != C_SEPARADOR ){
        //Convertimos el numero a unidad o decena, etc...
        //Ejemplo: 12 => 2*10^0 + 1*10^1 y tenemos el id en int.
        id += ( c - ASCII_CERO )* pow(10, exp);
        fseek(archivo, -2, SEEK_CUR);
        exp++;
    }
    fclose(archivo);
    return id;
}
//Valida que en la cadena no se incluya el caracter separador o espacios
//Valida nombres (de usuario, productos) y passwords
//Regresa la longitud de la cadena en caso de ser aceptada
int validarCadena(const char* cadena){
    if(cadena == NULL) return AP_INV;
    int cadena_long = 0;
    while(*cadena){
        if(*cadena == C_SEPARADOR ||*cadena == ' ')
            return CAD_INV;
        cadena++;
        cadena_long++;
    }
    return cadena_long;
}