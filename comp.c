#include <stdlib.h>
int main(){
    system("gcc -Wall -c general.c ");
    system("gcc -Wall -c cliente.c ");
    system("gcc -Wall -c control.c ");
    system("gcc -Wall -c proveedor.c");
    system("gcc -o cliente general.o cliente.o -lpthread -lm -lncurses");
    system("gcc -o proveedor general.o proveedor.o -lpthread -lm -lncurses");
    system("gcc -o control general.o control.o -lpthread -lm -lncurses");
    return 0;
}
