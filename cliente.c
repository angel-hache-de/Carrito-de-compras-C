#include "general.h"

sem_t * sem_productos;
sem_t * sem_carritos;

int iniciar_semaforos(key_t llave_productos, key_t llave_carritos){
    int memoria1, memoria2;
    //Obtiene el semaforo para el archivo productos
    memoria1=shmget(llave_productos,sizeof(sem_t),IPC_CREAT|0600);
    if(memoria1 == -1) return ERROR_ARCH;
    sem_productos=shmat(memoria1,0,0);
    //Obtiene el semaforo para el archivo carritos de compra
    memoria2=shmget(llave_carritos,sizeof(sem_t),IPC_CREAT|0600);
    if( memoria2 == -1) return ERROR_ARCH;
    sem_carritos=shmat(memoria2,0,0);
    // puts("Se han iniciado los sems");
    return OK;
}
//Agrega un producto al carrito de compras.
int agregarACarrito( int id_cliente, char * id_producto, int cantidad){
    if( id_producto == NULL ) return AP_INV;
    //Verificamos que la cantidad del producto sea menor a la disponible
    snack * producto = (snack *)malloc(sizeof(snack));
    if( producto == NULL )
        return ERROR_EN_MEMORIA;
    int long_linea = 300;
    FILE * archivo;
    producto->nombre_snack = (char *)malloc(sizeof(char)*100);
    char * linea_anterior = (char *)malloc(sizeof(char)*long_linea);
    if( producto->nombre_snack == NULL || linea_anterior == NULL)
        return ERROR_EN_MEMORIA;
    int estado;
    //Activamos el semaforo
    sem_wait(sem_productos);
    sem_wait(sem_carritos);
    archivo = fopen(ARCH_PRO, "r+");
    if ( archivo == NULL ) {
        sem_post(sem_productos);
        sem_post(sem_carritos);
        fclose(archivo);
        free(producto->nombre_snack);
        free(producto);
        free(linea_anterior);
        return ERROR_ARCH;
    }
    //Obtenemos el producto
    if ( ( estado = buscarRegistro(archivo, id_producto) ) != OK ){
        sem_post(sem_productos);
        sem_post(sem_carritos);
        fclose(archivo);
        free(linea_anterior);
        free(producto->nombre_snack);
        free(producto);
        return estado;
    }
    if( ( estado = convLineaSnack( fgets(linea_anterior, long_linea, archivo), &producto ) ) != OK ){
        sem_post(sem_productos);
        sem_post(sem_carritos);
        fclose(archivo);
        free(linea_anterior);
        free(producto->nombre_snack);
        free(producto);
        return estado;
    }
    
    //Si hay menos cantidad de la ingresada regresamos el error.
    if( producto->existencia < cantidad ){
        sem_post(sem_productos);
        sem_post(sem_carritos);
        fclose(archivo);
        free(producto->nombre_snack);
        free(producto);
        free(linea_anterior);
        return CANT_NO_DISP;
    }
    //Preparamos la linea a insertar. Formato:
    //                          NO_COMRPADO:nombre_snack:cantidad:precio_total:
    int precio_total = producto->precio * cantidad;
    char * nueva_linea = (char *)malloc(sizeof(char)*long_linea);
    sprintf(nueva_linea, "%d%c%s%c%d%c%d%c", NO_COMPRADO, C_SEPARADOR, producto->nombre_snack, 
                                            C_SEPARADOR, cantidad, C_SEPARADOR,
                                            precio_total, C_SEPARADOR);
    
    if ( escribirRegistroArchivo(id_cliente, nueva_linea, ARCH_CAR) != OK){
        sem_post(sem_productos);
        sem_post(sem_carritos);
        fclose(archivo);
        free(producto->nombre_snack);
        free(producto);
        free(linea_anterior);
        free(nueva_linea);
        free(linea_anterior);
        return ERROR_ARCH;
    }
    //Se prepara la linea que actuliza la info en el archivo. Formato:
    //                             :id:nombre:precio:existencia:
    producto->existencia -= cantidad;
    sprintf(nueva_linea, "%c%d%c%s%c%d%c%d%c", C_SEPARADOR, producto->id, C_SEPARADOR,
                                                producto->nombre_snack, C_SEPARADOR, 
                                                producto->precio, C_SEPARADOR, 
                                                producto->existencia, C_SEPARADOR);
    
    //Regresamos el apuntador al inicio de la linea a sobreescribir
    fseek(archivo, -strlen(linea_anterior), SEEK_CUR);
    //Sobreescribimos la linea. Se compara para evitar tener doble salto de linea
    if( strlen( linea_anterior ) > strlen( nueva_linea ) )
        fprintf(archivo, "%s",nueva_linea);
    else fprintf(archivo, "%s\n",nueva_linea);
    sem_post(sem_productos);
    sem_post(sem_carritos);
    fclose(archivo);
    free(producto->nombre_snack);
    free(producto);    
    free(nueva_linea);    
    free(linea_anterior);
    return OK;
}
//Muestra los productos y da la opcion de agregar alguno al carrito.
void menuAgregarProductos(usuario * cliente){
    //Vertical es la pos en pantalla
    int opcion = 0;
    int estado;
    //Cadenas para tener almacenar el id de producto y la cantidad.
    char * id = (char *)malloc(sizeof(char)*10); 
    int cantidad;
    if( id == NULL ){
        puts("-----Error en memoria------");
        return;            
    }
    do{
        system("clear");
        if (mostrarProductos(sem_productos) != OK) {
            puts("----Algo ha salido mal----");
            return;
        }
        puts("Escoge una de las opciones:");
        puts("1. Agregar al carrito");
        puts("2. Regresar");
        fflush(stdin);
        //Obtenemos el valor ingresado
        scanf("%d", &opcion);
        
        if(opcion == 1){
            puts("Introduce el numero del snack a comprar y da enter.");
            fflush(stdin);
            scanf("%s", id);
            puts("Introduce la cantidad a comprar y da enter.");
            fflush(stdin);
            scanf("%d", &cantidad);
            if( ( estado = agregarACarrito( cliente->id, id, cantidad ) ) != OK)
                printf("---%s---", cadenaError(estado));
            else
                puts("---PRODUCTO AGREGADO---");
        }
        sleep(3);
    }while (opcion != 2);
    free(id);
    return;
}
//Imprime los productos comprados o en el carrito.
//Recibe el id del clinte y el estado.
//El estado es si busca los comprados o lo que esta en el carro.
//Devuelve el costo total o un numero negativo en caso de error.
int mostrarCarritoVentas(int id_cliente, int estado){
    //Preparamos una linea del tipo: :id:estado:
    int long_linea_leida = 150, precio_total = 0, i;
    char * linea_a_buscar = (char *)malloc(sizeof(char)*20);
    char * linea_leida = (char *)malloc(sizeof(char)*long_linea_leida);
    if( linea_a_buscar == NULL || linea_leida == NULL ) return ERROR_EN_MEMORIA;
    
    char * char_precio = (char *)malloc(sizeof(char)*5);
    if( char_precio == NULL ) return ERROR_EN_MEMORIA;
    
    sprintf(linea_a_buscar, "%c%d%c%d", C_SEPARADOR, id_cliente, C_SEPARADOR, estado);
    int long_linea_buscada = strlen(linea_a_buscar);
    int espacio_nombre = 50;
    
    sem_wait(sem_carritos);
    //Obtenemos los primeros "long_linea" caractere de cada linea del archivo
    //y la comparamos con la linea a buscar. Si es igual, se muestra.
    FILE * archivo = fopen(ARCH_CAR, "r");
    if( archivo == NULL ){
        sem_post(sem_carritos);
        return ERROR_ARCH;
    } 
    char * aux;
    int num_separdores, long_nombre_producto, long_cantidad;
    
    //Imprimimos la tabla
    system("clear");
    if( estado == COMPRADO ) printf("\t\t\t\tTUS COMRPAS\n");
    else printf("\t\t\t\tTU CARRITO\n");
    printf("\tProducto");
    for( int j = 8; j < espacio_nombre; j++ ) printf(" ");
    printf(" Cantidad  Precio Total\n");
    while( fgets( linea_leida, long_linea_leida, archivo ) != NULL ){
        if( strncmp( linea_a_buscar, linea_leida, long_linea_buscada ) == 0 ){
            num_separdores = 0;
            i = 0; 
            long_cantidad = 0;
            long_nombre_producto = 0;
            //LLegamos al nombre. Linea: :id:estado:nombre:...
            aux = linea_leida; 
            // puts("Entra");
            while( num_separdores != 3 ){
                if( *aux == C_SEPARADOR )
                    num_separdores++;
                aux++;
            } 
            printf("\t");
            //Imprime el nombre
            while( *aux != C_SEPARADOR ) {
                printf("%c",(*aux));
                aux++; 
                long_nombre_producto++;
            }
            //Imprime los espacios para que se vea como en tabla.
            for(int j = long_nombre_producto; j < espacio_nombre; j++ ) printf(" ");
            printf(" ");
            //Imprime la cantidad
            aux++;
            while( *aux != C_SEPARADOR ) {
                printf("%c",(*aux));
                aux++; 
                long_cantidad++;
            }
            for(int j = long_cantidad; j < 10; j++ ) printf(" ");
            //Imprime el precio total
            aux++;
            printf("$");
            while( *aux != C_SEPARADOR ) {
                printf("%c",(*aux));
                char_precio[i] = *aux;
                aux++; i++;
            }
            char_precio[i] = '\0';
            precio_total += atoi(char_precio);
            printf("\n");
        }
    }
    //Imprime el total de todo
    printf("\tTotal");
    for(int j = 5; j < 61; j++) printf(" ");
    printf("$%d\n",precio_total);

    sem_post(sem_carritos);
    free(linea_a_buscar);
    free(linea_leida);
    free(char_precio);
    fclose(archivo);

    return precio_total;
}
//Finaliza la compra de todos los productos en el carrito.
int finalizarCompra(int id_cliente){
    //Preparamos una linea del tipo: :id:estado:
    int long_linea_leida = 150;
    char * linea_a_buscar = (char *)malloc(sizeof(char)*20);
    char * linea_leida = (char *)malloc(sizeof(char)*long_linea_leida);
    //Solo va almacenar 2 separadores, el id y el estado
    char * linea_nueva = ( char* )malloc(sizeof(char)*13);
    if( linea_a_buscar == NULL || linea_leida == NULL || linea_nueva == NULL ) 
        return ERROR_EN_MEMORIA;
    
    //Preparamos la linea a buscar. Buscamos la que tenga el id del usuario que recibimos
    //Y su estado sea no comprado
    sprintf(linea_a_buscar, "%c%d%c%d", C_SEPARADOR, id_cliente, C_SEPARADOR, NO_COMPRADO);
    int long_linea_buscada = strlen(linea_a_buscar);
    sem_wait(sem_carritos);
    //Obtenemos los primeros "long_linea" caractere de cada linea del archivo
    //y la comparamos con la linea a buscar. Si es igual, se muestra.
    FILE * archivo = fopen(ARCH_CAR, "r+");
    if( archivo == NULL ){
        sem_post(sem_carritos);
        return ERROR_ARCH;
    }     
    //Buscamos las lineas que coincida su inicio con la que formamos
    while( fgets( linea_leida, long_linea_leida, archivo ) != NULL ){
        if( strncmp( linea_a_buscar, linea_leida, long_linea_buscada ) == 0 ){
            //Nos posicionamos al inicio de la linea
            fseek(archivo, -strlen(linea_leida), SEEK_CUR);
            //Modificamos el estado, el resto de la linea se mantiene igual.
            if ( fprintf(archivo, "%c%d%c%d", C_SEPARADOR, id_cliente, C_SEPARADOR, COMPRADO) < 0)
                return ERROR_ARCH;
            
        }
    }
    fclose(archivo);
    sem_post(sem_carritos);
    free(linea_a_buscar);
    free(linea_leida);
    free(linea_nueva);
    return OK;
}

//Muestra el menu para ver productos o ver carrito
void menuProductos(int * msqid, usuario * cliente){
    if( msqid == NULL || cliente == NULL )
        puts("-----Algo ha ido mal-----");
    system("clear");
    printf("Bienvenido %s con id: %d y num. de cliente: %d\n",
            cliente->nombre_usuario,cliente->id, getpid());
    int opcion, opcion2, estado, costo_total;
    do{
        puts("Elige lo que quieras hacer");
        puts("1. Ver y agregar productos al carrito");
        puts("2. Ver carrito");
        puts("3. Ver compras");
        puts("4. Cerrar sesion y salir");
        fflush( stdin );
        scanf("%d",&opcion);
        if( opcion == 1 )
            menuAgregarProductos(cliente); 
        else if( opcion == 2 ){
            if ( ( costo_total = mostrarCarritoVentas( cliente->id, NO_COMPRADO )) < 0)
                puts(cadenaError(costo_total));
            else{
                puts("Elige lo que quieras hacer");
                puts("1. Finalizar compra");
                puts("2. Regresar");
                fflush(stdin);
                scanf("%d",&opcion2);
                if( opcion2 == 1 ){
                    if( ( estado = finalizarCompra(cliente->id) ) < 0)
                        puts(cadenaError(estado));
                    else   
                        printf("Has comprado %d productos por $%d\n", estado, costo_total);
                }
            }
        }
        else if( opcion == 3 ){
            if ( ( costo_total = mostrarCarritoVentas( cliente->id, COMPRADO )) < 0)
                puts(cadenaError(costo_total));
        }
            
    }while( opcion != 4 );
    
    return;
}
//Menu de inicio y registro
void menuInicioRegistro(int * msqid, int num_cliente){
    //Mientras 
    int opcion = 2;
    mensaje * peticion = (mensaje* )malloc(sizeof(mensaje));
    mensaje * respuesta = (mensaje* )malloc(sizeof(mensaje));
    if(peticion == NULL) {
        printf("Memoria llena");
        return;
    }
    peticion->carga.num_cliente = num_cliente;
    peticion->tipo = TYPMSG_USUARIO;
    
    while(opcion == 2){
        puts("Elige una opcion");
        puts("1. Iniciar sesion");
        puts("2. Registrar");
        puts("3. Salir");
        fflush( stdin );
        scanf("%d",&opcion);
        if(opcion != 3){
            //Obtencion de datos
            puts("No se permiten espacios entre caracteres ni el caracter ':'");
            puts("Ingresa el nombre de usuario 4-8 caracteres:");
            fflush( stdin );
            scanf("%s",peticion->carga.nombre_usuario);
            puts("Ingresa la password 4-8 caracteres:");
            fflush( stdin );
            scanf("%s",peticion->carga.password);
            
            if(opcion == 1)
                peticion->carga.tipo_peticion = INI_SESION;    
            
            else if(opcion == 2)
                peticion->carga.tipo_peticion = REGISTRO;
            
            if (mandarPeticion(msqid, peticion) == OK)
                if ( leerRespuesta(msqid, respuesta, num_cliente) == OK ){
                    opcion = 2;
                    if( respuesta->carga.estado == OK && peticion->carga.tipo_peticion == INI_SESION )
                        opcion = 1;
                    else if( respuesta->carga.estado == OK && peticion->carga.tipo_peticion == REGISTRO)
                        puts("---Usuario creado---");
                    else
                        printf("------%s------\n",cadenaError(respuesta->carga.estado));
                }
                else puts(" Algo ha salido mal al contactar con el server. ");
            else
                puts(" Algo ha salido mal al mandar al server");
        }
    }
    //Si se ha iniciado sesion
    if( opcion == 1 && respuesta->carga.id_usuario != USU_NO_LOG){
         //inician los semaforos
        if ( iniciar_semaforos( respuesta->carga.llave_productos, respuesta->carga.llave_carritos ) != OK){
            free(respuesta);
            free(peticion);
            puts("Error con los semaforos");
            return;
        }
        //Creamos el objeto del cliente iniciado
        usuario * cliente = (usuario *)malloc(sizeof( usuario ));
        cliente->nombre_usuario = (char *)malloc(sizeof(char)*strlen(peticion->carga.nombre_usuario));
        strcpy(cliente->nombre_usuario, peticion->carga.nombre_usuario);
        cliente->tipo = CLIENTE;
        cliente->id = respuesta->carga.id_usuario;
        
        //Se libera la memoria de las peticiones.
        free(respuesta);
        free(peticion);
        //Se pasa a la parte del usuario autenticado
        menuProductos(msqid, cliente);
        //Cierre de los semaforos
        sem_close(sem_carritos);
        sem_close(sem_productos);
        shmdt((void*)sem_carritos);
        shmdt((void*)sem_productos);
    }
    else{
        free(respuesta);
        free(peticion);
    }
}

int main() {
    //Se accede a la cola de mensajes
    int msqid;
    key_t llave; 
    llave = ftok("Prueba",'k');
    msqid = msgget(llave, IPC_CREAT | 0666);
    if(msqid == -1){
        perror("msgget");
        exit(-1);
    } 
    //Se necesita un numero de cliente unico para conectar a la unidad de control
    int num_cliente = getpid();
    menuInicioRegistro(&msqid, num_cliente);
    //La cola solo la destruye el servidor
    return 0;
}
