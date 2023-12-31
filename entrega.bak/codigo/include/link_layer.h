// Link layer header.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define _POSIX_SOURCE 1
#define BAUDRATE 38400
#define MAX_PAYLOAD_SIZE 1000

#define BUF_SIZE 256
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define ESC 0x7D
#define A_ER 0x03
#define A_RE 0x01
#define C_SET 0x03
#define C_DISC 0x0B
#define C_UA 0x07
#define C_RR0 0x05
#define C_RR1 0x85
#define C_REJ0 0x01
#define C_REJ1 0x81
#define C_I0 0x00
#define C_I1 0x40

typedef enum
{
   transmitter,
   receiver,
} LinkLayerRole;

typedef enum
{
   START,            //0
   FLAG_RCV,         //1
   A_RCV,            //2
   C_RCV,            //3
   BCC1_OK,          //4
   STOP_STATE,       //5
   DATA_FOUND_ESC,   //6
   DATA,             //7
   DISCONNECTED,     //8
   BCC2_OK           //9
} LinkLayerState;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer link);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(int fd, const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(int fd, unsigned char *packet);

// Close previously opened connection.
// Return "1" on success or "-1" on error.
int llclose(int fd, LinkLayerRole);

// timeout
void alarmHandler(int signal);

unsigned char readControlFrame (int fd);

int sendFrame(int fd, unsigned char A, unsigned char C);

#endif // _LINK_LAYER_H_
