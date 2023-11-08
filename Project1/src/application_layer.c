#include "application_layer.h"
#include "link_layer.h"
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



unsigned char packet[PCK_SIZE];


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename){


    printf("defining roles\n");
    
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




    printf("\n----------------LLOPEN------------------\n");

    int fd = llopen(connectionParameters); 

    if(fd == -1) {
      perror("Error: connection not successfull\n");
      exit(-1);
    }else{
        printf("Connection successfull\n");
    }

    //temporario
    //exit(-1);
    


    //---------------------TRANSMITTER-----------------------//
    

    if(connectionParameters.role == transmitter) {

        printf("\n------------------TRANSMITTER-------------------\n");

        FILE* file = fopen(filename, "rb"); //open file in binary mode: r == read, b == binary
        printf("reading file: %s\n", filename);
        
        if(file == NULL) {
            perror("Error openning the file\n");
            exit(-1);
        } 

        struct stat st; //to retrieve file information
        int file_size = (stat(filename, &st) == 0) ? st.st_size : 0;
        printf("file_size from stat: %d\n", file_size);

        printf("file_permissions stat: %d\n", st.st_mode & 07777);  //wrong permissions for some reason

        packet[0] = CTRLSTART;
        packet[1] = 0; //control flag
        packet[2] = sizeof(long);           //L2 guide page 25
        *((long*)(packet + 3)) = file_size; //L1 guide page 25


        //printf("\n------------------LLWRITE-------------------\n");
        
        printf("SENDING CONTROL START PACKET\n");
        
        int frameSize_llwrite = llwrite(fd, packet, 10); 

        if( frameSize_llwrite == -1){
            perror("llwrite returned -1\n");
            exit(-1);
        }
        
        //printf("frameSize_llwrite: %d\n", frameSize_llwrite);


        //--------------------------------FROM FILE TO BYTES---------------------------//
        
        int bytes_tx = 0;
        unsigned char i = 0;
        do {
            unsigned long total_bytes;
            if(file_size - bytes_tx < PCK_SIZE) {
                total_bytes = fread(packet + 4, 1, file_size - bytes_tx, file);
            }
            else {
                total_bytes = fread(packet + 4, 1, PCK_SIZE, file); 
            }

            packet[0] = CTRLDATA;
            packet[1] = i;
            packet[2] = total_bytes >> 8;  //first 8 bytes
            packet[3] = total_bytes % 256; //last 8 bytes
                                           
            printf("SENDING CONTROL DATA PACKET\n");
            
            if(llwrite(fd, packet, total_bytes + 4) == -1){
                printf("failed llwriting CTRLDATA\n");
                exit(-1);
                break;
            }
            printf("\nPacket %i sent\n\n",i);
            bytes_tx += total_bytes;
            i++;
        }while(bytes_tx < file_size);
        
        printf("SENDING CONTROL END PACKET\n");
        
        packet[0] = CTRLEND;
        if(llwrite(fd, packet,1) == -1){
            printf("failed llwriting CTRLEND\n");
            perror("llwrite\n");
            exit(-1);
        }
        
        
        fclose(file);


        printf("\n------------------LLCLOSE------------------\n");
        
        if(llclose(fd, transmitter) == -2) {
            printf("llclose failed\n");
            perror("llclose");
            exit(-2);
        }
    }


    //---------------------RECEIVER-----------------------//
    

    else if(connectionParameters.role == receiver) {

        int file_size = 0, bytes_rx = 0;


        printf("\nREADING CONTROL START PACKET\n");

        int bytes = llread(fd, packet);
        
        //printf("bytes: %d\n", bytes);
        

        
        
        
        int type,length,*value;

        if(packet[0] != CTRLSTART){
            perror("Failed reading CTRLSTART packet\n");
            exit(-1);
        }

        //get file size (tlv)

        int tlv_size = 1; //tracks the position within the packet
        while(tlv_size < bytes) {
            tlv_size += tlv(packet + tlv_size, &type, &length, &value);
            if(type == 0){
                file_size = *value;
                printf("File size: %d\n",file_size);
            }
        }

        FILE* file = fopen(filename, "wb"); //w = write, b = binary

        if(file == NULL) {
            perror("Cannot open the file\n");
            exit(-1);
        } 
        else {
            printf("Control packet received\n");
            printf("ready to write to file\n");
        }

        

        //--------------------------------FROM BYTES TO FILE---------------------------//
        
        printf("\nREADING CONTROL DATA PACKET\n");

        int packetNumber = 0;
        
        while(bytes_rx < file_size) {

            printf("bytes_rx: %d\n", bytes_rx);

            int bytes;
            if((bytes = llread(fd, packet)) == -1){
                perror("llread\n");
                exit(-1);
            }

            if(packet[0] == CTRLEND){
                perror("CTRL_END failed\n");
                exit(-1);
            }

            if(packet[0] == CTRLDATA){
                if(bytes < 5) {
                    perror("CTRLDATA failed\n");
                    exit(-1);
                }
                else if(packet[1] != packetNumber){  //so it doesnt skip packets
                    perror("packetNumber failed\n");
                    exit(-1);
                }
                else{
                    unsigned long size = packet[3] + packet[2]*256; //guide slide 25

                    if(bytes != size + 4) { //4 = header
                        perror("Wrong header\n");
                        exit(-1);
                    }

                    fwrite(packet + 4, 1, size, file);
                    bytes_rx += size;

                    printf("Packet %d received\n", packetNumber);

                    packetNumber++;
                }
            }
        }
        

        printf("\nREADING CONTROL END PACKET\n");
        
        int bytes_read = llread(fd, packet);

        if(bytes_read == -1) {
            perror("llread\n");
            exit(-1);
        }
        else if(bytes_read < 1) {
            perror("Short packet\n");
            exit(-1);
        }
        
        if(packet[0] != CTRLEND){
            printf("CTRLEND failed\n");
        }
        else{
            printf("Received end packet\n");
        }
        fclose(file);


        printf("\nCLOSING CONNECTION\n\n");
        
        if(llclose(fd, receiver) == -2) {
            printf("llclose failed\n");
            perror("llclose");
            exit(-2);
        }
    }
    

}


//--------------------AUXILIARY FUNCTIONS-----------------------//


int tlv(unsigned char *address, int* type, int* length, int** value){
    
    *type = address[0];
    *length = address[1];
    *value = (int*)(address + 2);

    return 2 + *length;
}