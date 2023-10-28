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
    


    //---------------------TRANSMITTER-----------------------//
    

    if(connectionParameters.role == transmitter) {

        printf("\n\t---- TRANSMITTER ROLE ----\n\n");

        FILE* file = fopen(filename, "rb"); //open file in binary mode
        printf("filename: %s\n", filename);
        
        if(file == NULL) {
            perror("Error openning the file\n");
            exit(-1);
        } 

        struct stat st; //to retrieve file information
        int file_size = (stat(filename, &st) == 0) ? st.st_size : 0;
        printf("file_size from stat: %d\n", file_size);
        printf("file_permissions stat: %d\n", st.st_mode & 07777);

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


        //--------------------------------CONCETRACAO---------------------------//
        
        int bytes_tx = 0;
        unsigned char i = 0;
        do {
            unsigned long total_bytes;
            if(file_size-bytes_tx < PCK_SIZE) {
                total_bytes = fread(packet + 4, 1, file_size-bytes_tx, file);
            }
            else {
                total_bytes = fread(packet + 4, 1, PCK_SIZE, file);
            }

            packet[0] = CTRLDATA;
            packet[1] = i;
            packet[2] = total_bytes >> 8;  //primeiros 8 bytes
            packet[3] = total_bytes % 256; //ultimos 8 bytes
                                           
            printf("SENDING CONTROL DATA PACKET\n");
            
            if(llwrite(fd, packet, total_bytes + 4) == -1){
                printf("failed llwriting CTRLDATA\n");
                perror("llwrite\n");
                exit(-1);
                break;
            }
            printf("Packet %i sent\n",i);
            bytes_tx += total_bytes;
            i++;
        }while(bytes_tx < file_size);
        
        printf("llwriting CTRLEND packet\n");
        
        packet[0] = CTRLEND;
        if(llwrite(fd, packet,1) == -1){
            printf("failed llwriting CTRLEND\n");
            perror("llwrite\n");
            exit(-1);
        }
        
        
        //--------------------------------FIM CONCETRACAO---------------------------//
        
        
        fclose(file);


        printf("\n\t---- LLCLOSE TRANSMITTER ----\n\n");
        
        if(llclose(fd, transmitter) == -2) {
            printf("llclose failed\n");
            perror("llclose");
            exit(-2);
        }
    }


    //---------------------RECEIVER-----------------------//
    

    else if(connectionParameters.role == receiver) {

        int file_size = 0, size_rx = 0;

        printf("\n\t---- LLREAD ----\n\n");

        int bytes = llread(fd, packet);
        
        printf("bytes: %d\n", bytes);
        
        
        //--------------------------------CONCETRACAO---------------------------//
        
        int type,length,*value;

        if(packet[0] == CTRLSTART) {
            int tlv_size = 1;
            while(tlv_size < bytes) {
                tlv_size += tlv(packet + tlv_size, &type, &length, &value);
                if(type == 0){
                    file_size = *value;
                    printf("File size: %d\n",file_size);
                }
            }

            FILE* file = fopen("penguin.gif", "wb");

            if(file == NULL) {
                perror("Cannot open the file\n");
                exit(-1);
            } 
            else {
                //printf("Control packet received\n");
                printf("ready to write to file\n");
            }

            int final_seq_num = 0;
            

            while(size_rx < file_size) {

                printf("FJDKSLÇAFJLDKSFJÇDSLAJFLKASJFÇLFJDKLSAÇFJÇLKSFJLKDASÇFJDSAKLÇFJDLSKÇAJFKSADL\n");
                printf("size_rx: %d\n", size_rx);

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
                    else if(packet[1] != final_seq_num){
                        perror("final_seq_num failed\n");
                        exit(-1);
                    }
                    else{
                        unsigned long size = packet[3] + packet[2]*256;

                        if(bytes-4 != size) {
                            perror("Header deprecated\n");
                            exit(-1);
                        }

                        fwrite(packet + 4, 1, size, file);
                        size_rx += size;

                        printf("Packet %d received\n", final_seq_num);

                        final_seq_num++;
                    }
                }
            }
            
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
                printf("packet[0]: %d\n", packet[0]);
                printf("packet[1]: %d\n", packet[1]);
            }
            else{
                printf("Received end packet\n");
                printf("Transmission ending...\n");
            }
            fclose(file);


        //--------------------------------FIM CONCETRACAO---------------------------//
        

        }
        else {
            perror("Start packet failed\n");
            exit(-1);
        }



        printf("\n\t---- LLCLOSE RECEIVER ----\n\n");
        
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