#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
    long frames = strtol(argv[3],NULL,10);
    fprintf(stdout,"%ld frames\n",frames);
    FILE* inputFile = fopen(argv[1],"rb");
    if (inputFile == NULL){
     
        return EXIT_FAILURE;
    }
   
    FILE* outputFile = fopen(argv[2],"wb");
    if (outputFile == NULL){
        fclose(inputFile);
        return EXIT_FAILURE;
    }
    long curFrame = 0;
    double d1,d2,d3,d4,d5=0.0;
    fprintf(outputFile,"frame,object_name,confidence,xmin,ymin,xmax,ymax\n");
    fscanf(inputFile,"frame,object_name,confidence,xmin,ymin,xmax,ymax\n");
    do{
        fscanf(inputFile,"%ld,person,%lf,%lf,%lf,%lf,%lf\n",&curFrame,&d1,&d2,&d3,&d4,&d5);
        if(curFrame<frames){
            fprintf(outputFile,"%ld,person,%.6f,%.6f,%.6f,%.6f,%.6f\n",curFrame,d1,d2,d3,d4,d5);
        }
    }while(curFrame < frames);

    fclose(outputFile);
    fclose(inputFile);
    return EXIT_SUCCESS;
}
