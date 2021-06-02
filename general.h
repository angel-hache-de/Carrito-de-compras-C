#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio_ext.h>
#include <math.h>
#include <sys/msg.h>
#include <ncurses.h>

#define CLIENTE 1
//Constantes de la tuberia
#define LONGITUD (sizeof(mensaje) - sizeof(long))
#define TRUE 1
//Tipos de mensaje a mandar y recibir
#define TYPMSG_CONTROL 2
#define TYPMSG_USUARIO 1
//Tipos de peticiones al servidor
#define REGISTRO 1
#define INI_SESION 2
#define USU_NO_LOG -1

//Semaforos
#define SIN_LLAVE 0
#define ENTERO_LLAVE 7
#define ARCH_SEM_CAR "Carritos"
#define INT_SEM_CAR 7
#define ARCH_SEM_PRO "Productos"
#define INT_SEM_PRO 14

//Estados de la app
// #define OK 0
#define ERROR_ARCH -1
#define CAD_INV -2
#define AP_INV -3
#define ERROR_COLA -4
#define ERROR_EN_MEMORIA -5
#define ERROR -6
#define CANT_NO_DISP -7
#define CLI_NO_ENCONTRADO -8 
#define SNACK_NO_ENCONTRADO -9
#define REGISTRO_NO_ENCONTRADO -10
//Constantes a utilizar
#define ASCII_CERO 48
#define TAM_MAX_NOM_USU 8
#define TAM_MIN_NOM_USU 4
#define TAM_MAX_PASS 8
#define TAM_MIN_PASS 4
#define COMPRADO 1
#define NO_COMPRADO 0
//Separador de datos en los archivos txt
#define C_SEPARADOR ':'
#define S_SEPARADOR ":"
#define NUM_SEP_LIN_CLI 4  //Ejemplo: :8:jazmin:jazmin:
#define NUM_SEP_LIN_PRO 5  //Ejemplo: :1:Cono de Fresa:20:26:
#define NUM_SEP_LIN_CAR 5  //Ejemplo: :1:0:1:2:
//Archivo de clientes
#define ARCH_CLI "clientes.txt"
#define ARCH_PRO "productos.txt"
#define ARCH_CAR "carrito.txt"

typedef struct {
    char tipo_peticion;
    char nombre_usuario[50];
    char password[50];
    key_t llave_carritos;
    key_t llave_productos;
    int estado;
    int id_usuario;
    int num_cliente;
} payload;

typedef struct {
    long tipo;
    payload carga;
} mensaje;


typedef struct
{
    int id;
    int tipo;
    char *nombre_usuario;
} usuario;

typedef struct
{
    int id;
    int existencia;
    int precio;
    char * nombre_snack;
} snack;

//Metodos
char * cadenaError(int error);
int escribirRegistroArchivo(int id, char * cadena, char * archivo);
int mandarPeticion(int * msqid, mensaje * msj );
int leerRespuesta(int * msqid, mensaje * msj, int num_cliente);
int convLineaSnack(char * linea, snack ** producto);
int buscarRegistro( FILE * archivo, char * id );
int mostrarProductos( sem_t *);
int obtUltimoId(const char * nombre_archivo, int separadores_por_linea);
int validarCadena(const char * cadena);
#endif