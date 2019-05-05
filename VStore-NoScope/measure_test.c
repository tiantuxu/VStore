#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "measure.h"

int main(int argc, char** argv){
    long it = strtol(argv[1],NULL,10);
    k2_measure("start");
    for (long i = 0; i<it; i++){
        int a = 2;
    }
    k2_measure("mid");
    for (long i = 0; i<it; i++){
        int a = 2;
    }
    k2_measure("end");
    k2_measure_flush();
    return EXIT_SUCCESS;
}
