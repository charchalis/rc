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


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename){

    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort,serialPort);
    linkLayer.role = strcmp(role, "tx") ? receiver : transmitter;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    int fd = llopen(linkLayer);
    if (fd < 0) {
        printf("Connection failed\n");
        return;
    } 

    switch (linkLayer.role) {

        case transmitter: {
            
            FILE* file = fopen(linkLayer.filename);
            
            if(llwrite(fd, controlPacket, cpSize) == -1) { 
                printf("Exit: error in end packet\n");
                exit(-1);
            }
            llclose(fd);
            break;
        }

        case receiver: {

            FILE* newFile = fopen((char *) name, "wb+");

                while (llread(fd, packet) < 0){
                    
                }
            }

            fclose(newFile);
            break;

        default:
            exit(-1);
            break;
    }
}