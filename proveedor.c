#include "general.h"
sem_t* sem_productos;

int iniciar_semaforo(){
  key_t llave_pro;
  llave_pro=ftok(ARCH_SEM_PRO,INT_SEM_PRO);
  int memoria;
  //Obtiene el semaforo para el archivo productos
  memoria=shmget(llave_pro,sizeof(sem_t),IPC_CREAT|0600);
  if(memoria == -1) return ERROR_ARCH;
  sem_productos=shmat(memoria,0,0);
  return OK;
 }

//Agregar un artículo al catalogo
int agregarArticulo(){
  snack* articulo = (snack *)malloc(sizeof(snack));
  if( articulo == NULL ) return ERROR_EN_MEMORIA;
  int long_linea = 300;
  articulo->nombre_snack = (char*)malloc(sizeof(char)*100);
  char* nueva_linea = (char*)malloc(sizeof(char)*long_linea);
  if( articulo->nombre_snack == NULL || nueva_linea == NULL) return ERROR_EN_MEMORIA;
  FILE* archivo;
  
  //Activamos el semaforo
  sem_wait(sem_productos);
  archivo = fopen(ARCH_PRO, "r+");
  if (archivo == NULL)
  {
      sem_post(sem_productos);
      free(articulo->nombre_snack);
      free(articulo);
      free(nueva_linea);
      return ERROR_ARCH;
  }
  puts("El nombre no debe tener el caracter \":\"");
  puts("Ingrese el nombre del snack (Confirme con Enter): ");
  __fpurge(stdin);
  fgets(articulo->nombre_snack, 100, stdin);
  //Quita el salto de linea que lee la cadena
  articulo->nombre_snack[ strlen(articulo->nombre_snack) - 1 ] = '\0';
  puts("Ingrese el precio del producto (Confirme con Enter): ");
  scanf("%i",&articulo->precio);
  __fpurge(stdin);
  puts("Ingrese el número de unidades (Confirme con Enter): ");
  scanf("%i",&articulo->existencia);
  __fpurge(stdin);


  //Creamos la linea del snack a escribir en el txt; se suman los separadores
  sprintf(nueva_linea,"%s%c%d%c%d%c",articulo->nombre_snack,C_SEPARADOR,
            articulo->precio,C_SEPARADOR,articulo->existencia,C_SEPARADOR);
  //Obtenemos los id correctos
  int id_nuevo_articulo = obtUltimoId(ARCH_PRO, NUM_SEP_LIN_PRO) + 1;

  if(id_nuevo_articulo == ERROR_ARCH){
      sem_post(sem_productos);
      fclose(archivo);
      free(articulo->nombre_snack);
      free(articulo);
      free(nueva_linea);
      return ERROR_ARCH;
  }

  if ( escribirRegistroArchivo(id_nuevo_articulo, nueva_linea, ARCH_PRO) != OK){
      sem_post(sem_productos);
      fclose(archivo);
      free(articulo->nombre_snack);
      free(articulo);
      free(nueva_linea);
      return ERROR_ARCH;
  }

  sem_post(sem_productos);
  fclose(archivo);
  free(articulo->nombre_snack);
  free(articulo);
  free(nueva_linea);
  return OK;
}
//Buscar un artículo en el catalogo, NO SIRVE, EN PROCESO XD
int buscarArticulo()
{
  int i=0, flag=0, estado;
  int long_linea = 300;
  char* linea_leida = (char*)malloc(sizeof(char)*long_linea);
  snack* producto =(snack*)malloc(sizeof(snack)); 
  char* buscar=(char*)malloc(sizeof(char)*100);
  char* aux;
  if( producto == NULL || buscar == NULL ) return ERROR_EN_MEMORIA;
  producto->nombre_snack = (char*)malloc(sizeof(char)*100);
  char* linea_anterior = (char*)malloc(sizeof(char)*long_linea);
  if( producto->nombre_snack == NULL || linea_anterior == NULL)
    return ERROR_EN_MEMORIA;
  FILE* archivo; 
  
  sem_wait(sem_productos);
  archivo = fopen(ARCH_PRO, "r+");
  if (archivo == NULL)
  {
      sem_post(sem_productos);
      fclose(archivo);
      free(producto);
      free(buscar);
      free(linea_anterior);
      free(producto->nombre_snack);
      free(linea_leida);
      return ERROR_ARCH;
  }
  puts("\nLos nombres no contienen espacio ni el caracter \":\"");
  puts("Ingrese el nombre del snack a buscar (Confirme con enter): ");
  __fpurge(stdin);
  fgets(buscar, 200, stdin);  
  //Quita el salto de linea que ha leido la cadena
  buscar[ strlen(buscar) - 1 ] = '\0';
  //Formato de la linea. ':' es SEPARADOR. :id:nombre:...
  while (  flag != TRUE && fgets(linea_leida, long_linea, archivo) != NULL ){
    puts(linea_leida);
    aux = linea_leida;
    //Saltamos el primer y segundo separador
    while(i<2)
    { 
      if(*aux==C_SEPARADOR) i++;
      aux++; 
    }
    //Comparamos el nombre.
    i = 0;
    while(*aux == buscar[i] ){ aux++; i++; }
    //Si son iguales el nombre leido y el nombre recibido, regresamos el apuntador
    //al inicio de la linea :v
    if(*aux ==  C_SEPARADOR && buscar[i] == '\0' ){
      flag = TRUE;
      puts("\n---PRODUCTO ENCONTRADO---");
      fseek(archivo, -strlen(linea_leida), SEEK_CUR);
      }
  }    
  if(flag != TRUE)
  {
    sem_post(sem_productos);
    fclose(archivo);
    free(producto);
    free(buscar);
    free(linea_leida);
    free(producto->nombre_snack);
    free(linea_leida);
    return SNACK_NO_ENCONTRADO;
  }
  if( ( estado = convLineaSnack( fgets(linea_anterior, long_linea, archivo), &producto ) ) != OK )
  {
    sem_post(sem_productos);
    fclose(archivo);
    free(linea_leida);
    free(producto->nombre_snack);
    free(buscar);
    free(linea_anterior);
    free(producto);
    return estado;
  }
  puts("Artículo encontrado\nID\tNombre\t\tPrecio($)\tExistencia");
  printf("%d\t%s\t\t%d\t\t%d\n",producto->id,producto->nombre_snack,producto->precio,producto->existencia);
  
  sem_post(sem_productos);
  fclose(archivo);
  free(producto->nombre_snack);
  free(producto);
  free(buscar);
  free(linea_anterior);
  free(linea_leida);
  return OK;
}
//Agregar más unidades a algún snack del catalogo
int agregarExistencia()
{
  FILE* archivo;
  int cant,estado;
  int long_linea = 300;
  char* id = (char*)malloc(sizeof(char)*10); 
  char* linea_leida = (char*)malloc(sizeof(char)*long_linea);
  snack* producto =(snack*)malloc(sizeof(snack)); 
  producto->nombre_snack = (char*)malloc(sizeof(char)*100);
  
  if(producto == NULL || producto->nombre_snack == NULL || linea_leida == NULL || id == NULL)
    return ERROR_EN_MEMORIA;
  puts("\nIngrese el ID del producto (Confirme con Enter): ");
  __fpurge(stdin);
  scanf("%s",id);
  
  sem_wait(sem_productos);
  //Abre el archivo para lectura/escritura
  archivo = fopen(ARCH_PRO, "r+");

  if (archivo == NULL)
  {
    sem_post(sem_productos);
    fclose(archivo);
    free(producto->nombre_snack);
    free(producto);
    free(linea_leida);
    free(id);
    return ERROR_ARCH;
  }
  //Busca el registro con el id que mandamos. 
  if( (estado = buscarRegistro(archivo, id)) != OK )
  {
    sem_post(sem_productos);
    fclose(archivo);
    free(producto->nombre_snack);
    free(producto);
    free(linea_leida);
    free(id);
    return estado;
  }
  //Convierte la linea de texto en una estructura snack
  if( ( estado = convLineaSnack( fgets(linea_leida, long_linea, archivo), &producto ) ) != OK )
  {
    sem_post(sem_productos);
    fclose(archivo);
    free(linea_leida);
    free(producto->nombre_snack);
    free(id);
    free(producto);
    return estado;
  }
  //Imprime en forma de tabla
  puts("Artículo encontrado\nID\tNombre\t\tPrecio($)\tExistencia");
  printf("%d\t%s\t\t%d\t\t%d\n",producto->id,producto->nombre_snack,producto->precio,producto->existencia); 
  //Ungreso de la nueva cantidad
  puts("Ingrese la nueva cantidad (Confirme con Enter): ");
  scanf("%d",&cant);
  __fpurge(stdin);
  producto->existencia=cant;
  //Regresa el apuntador del archivo al inicio de la linea a sobreescribir
  fseek(archivo, -strlen(linea_leida), SEEK_CUR);
  //Sobreescribimos la línea
  if ( fprintf(archivo,"%c%d%c%s%c%d%c%d%c     \n",C_SEPARADOR,producto->id,C_SEPARADOR,producto->nombre_snack,C_SEPARADOR,producto->precio,C_SEPARADOR,producto->existencia,C_SEPARADOR) < 0)
                return ERROR_ARCH;
  puts("Existencia actualizada\nID\tNombre\t\tPrecio($)\tExistencia");
  printf("%d\t%s\t\t%d\t\t%d\n",producto->id,producto->nombre_snack,producto->precio,producto->existencia); 
  //Cierra el archivo y libera los semaforos junto con la memoria.
  fclose(archivo);
  sem_post(sem_productos);
  free(producto->nombre_snack);
  free(producto);
  free(linea_leida);
  free(id);
  return OK;
}

int menuProveedor()
{
  int opc,estado;
  //Inicializa el semaforo
  if(iniciar_semaforo()==ERROR_ARCH)
    return ERROR;
  
  while(opc!=5)
  {
    puts("----Menú de Proveedores----");
    puts("1. Añadir artículo");
    puts("2. Buscar artículo por nombre");
    puts("3. Modificar unidades de un snack");
    puts("4. Listar productos");
    puts("5. Salir");
    puts("Ingrese una opción: ");
    scanf("%i",&opc);
    __fpurge(stdin);
    if(opc<1 || opc>5) puts("Opción Invalida");
    switch(opc){
      case(1):
        if( ( estado = agregarArticulo() ) != OK)
          printf("---%s---\n", cadenaError(estado));
        else
          puts("---PRODUCTO AGREGADO---\n");
        break;
      case(2):
        if( ( estado = buscarArticulo() ) != OK)
          printf("---%s---\n", cadenaError(estado));
        else
          puts("---TAMOS BIEN---\n");
        break;
      case(3):
        if( ( estado = agregarExistencia() ) != OK)
          printf("---%s---\n", cadenaError(estado));
        else
          puts("---COMPLETADO SATISFACTORIAMENTE---\n");
        break;
      case(4):
        if( ( estado = mostrarProductos(sem_productos) ) != OK)
          printf("---%s---\n", cadenaError(estado));
        // else
          // puts("---COMPLETADO SATISFACTORIAMENTE---\n");
        // break;
        break;
      case(5):
      puts("Adiós!!");
    }
  }
  //Cierre de los semaforos
  sem_close(sem_productos);
  shmdt((void*)sem_productos);
  return 0;
}


int main(int argc, char const *argv[])
{
 menuProveedor();
 return 0;
}
