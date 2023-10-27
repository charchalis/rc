#include "applicationLayer.h"
#include "linkLayer.h"
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#define PCK_SIZE 512
#define CTRLSTART 2

unsigned char packet[PCK_SIZE+30];


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename){


    printf("application layer\n");
    
    LinkLayer connectionParameters;
    strcpy(connectionParameters.serialPort, serialPort);
        
    if(strcmp(role,"rx") == 0) {
        connectionParameters.role = receiver;
    }
    else if(strcmp(role,"tx") == 0) {
        connectionParameters.role = transmitter;
    }
    else {
        perror(role);
        exit(-1);
    }

    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    printf("\n\t---- LLOPEN ----\n\n");

    int fd = llopen(connectionParameters); 

    if(fd == -1) {
      perror("Error opening a connection using the port parameters");
      exit(-1);
    } 
    


    //---------------------LLWRITE-----------------------//
    

    if(connectionParameters.role == transmitter) {

        printf("\n\t---- TRANSMITTER ROLE ----\n\n");

        FILE* file = fopen(filename, "rb");
        
        if(file == NULL) {
            perror("Error openning the file\n");
            exit(-1);
        } 

        struct stat st;
        int file_size = (stat(filename, &st) == 0) ? st.st_size : 0;

        packet[0] = CTRLSTART;
        packet[1] = 0;
        packet[2] = sizeof(long);

        printf("\n\t---- LLWRITE ----\n\n");

        *((long*)(packet + 3)) = file_size;

        
        int frameSize_llwrite = llwrite(fd, packet, 10); 

        if( frameSize_llwrite == -1){
            perror("llwrite returned -1\n");
            exit(-1);
        }
        
        printf("frameSize_llwrite: %d\n", frameSize_llwrite);
    }

    else if(connectionParameters.role == receiver) {

        //int file_size = 0, size_rx = 0;

        printf("\n\t---- LLREAD ----\n\n");

        int bytes = llread(fd, packet);
        
        printf("bytes: %d\n", bytes);
    }
    

    printf("\n\t---- LLCLOSE ----\n\n");
    
    if(llclose(fd) == -1) {
        printf("llclose failed\n");
        perror("llclose");
        exit(-1);
    }
}