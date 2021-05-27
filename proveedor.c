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
  //Iniciamos el semaforo
  if(iniciar_semaforo()==ERROR_ARCH)
  {
    free(articulo->nombre_snack);
    free(articulo);
    free(nueva_linea);
    return ERROR_ARCH;
  }
  //Activamos el semaforo
  sem_wait(sem_productos);
  archivo = fopen(ARCH_PRO, "r+");
  if (archivo == NULL)
  {
      sem_post(sem_productos);
      fclose(archivo);
      free(articulo->nombre_snack);
      free(articulo);
      free(nueva_linea);
      return ERROR_ARCH;
  }
  puts("Ingrese el nombre del snack (Confirme con Enter): ");
  scanf("%s",articulo->nombre_snack);
  fflush(stdin);
  puts("Ingrese el precio del producto (Confirme con Enter): ");
  scanf("%i",&articulo->precio);
  fflush(stdin);
  puts("Ingrese el número de unidades (Confirme con Enter): ");
  scanf("%i",&articulo->existencia);
  fflush(stdin);


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
  int posicion = OK;
  snack* producto =(snack *)malloc(sizeof(snack)); 
  char *buscar=(char *)malloc(sizeof(char));
  if( producto == NULL ) return ERROR_EN_MEMORIA;
  FILE* archivo; 
  if(iniciar_semaforo()==ERROR_ARCH)
  {
    free(producto);
    free(buscar);
    return ERROR_ARCH;
  }
  sem_wait(sem_productos);
  archivo = fopen(ARCH_PRO, "r+");
  if (archivo == NULL)
  {
      sem_post(sem_productos);
      fclose(archivo);
      free(producto);
      free(buscar);
      return ERROR_ARCH;
  }
  puts("Ingrese el ID del producto a buscar (Confirme con enter): ");
  scanf("%s",buscar);
  fflush(stdin);
  while(fread(&producto,sizeof(snack),1,archivo)>0)
  {
    if(strcmp(buscar,producto->nombre_snack)==0)
    {
      int posicion=ftell(archivo)/sizeof(snack);
    }
  }
  sem_post(sem_productos);
  fclose(archivo);
  free(producto);
  free(buscar);
  return posicion;
}
//Agregar más unidades a algún snack del catalogo
int agregarExistencia()
{
  /*char *id;
  puts("Ingrese el ID del producto (Confirme con Enter): ");
  scanf("%s",id);
  FILE *archivo;
  char modifica;
  snack producto;
  if(iniciar_semaforo()==ERROR_ARCH)
  {
    free();
  }
  sem_wait(sem_productos);
  //Abre el archivo para lectura/escritura
  archivo = fopen(ARCH_PRO, "rb+");

  if (archivo == NULL)
  {
    sem_post(sem_productos);
    fclose(archivo);
    free(producto->nombre_snack);
    free(producto);
    free(linea_anterior);
    return ERROR_ARCH;
  }
  fread(&producto2, sizeof(producto2), 1, archivo);
  while (!feof(archivo)) {
    if (producto2.codigo == producto.codigo) {
      fseek(archivo, ftell(archivo) - sizeof(producto), SEEK_SET);
      fwrite(&producto, sizeof(producto), 1, archivo);
      break;
    }
    fread(&producto2, sizeof(producto2), 1, archivo);
  }
  //Cierra el archivo
  fclose(archivo);*/
  return OK;
}

int menuProveedor()
{
  int opc,estado;
  while(opc!=4)
  {
    puts("\n--Menú de Proveedores--\n");
    puts("1. Añadir artículo\n");
    puts("2. Buscar artículo\n");
    puts("3. Añadir existencia\n");
    puts("4. Salir\n");
    puts("Ingrese una opción: ");
    scanf("%i",&opc);
    fflush(stdin);
    if(opc<1 || opc>4) puts("Opción Invalida");
    switch(opc){
      case(1):
      if( ( estado = agregarArticulo() ) != OK)
          printf("---%s---", cadenaError(estado));
      else
          puts("---PRODUCTO AGREGADO---");
      break;
      case(2):
        printf("%i",buscarArticulo());
        break;
      case(3):
        agregarExistencia();
        break;
      case(4):
        puts("Adiós!!");
        return 0;
    }
  }
  return 0;
}


int main(int argc, char const *argv[])
{
 menuProveedor();
 return 0;
}
