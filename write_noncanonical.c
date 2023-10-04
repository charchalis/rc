// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include "alarm.h"

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

#define FLAG 0x7E
#define RECEIVER_A 0x01
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07




unsigned char SET[5] = {FLAG, A, C_SET, FLAG, FLAG};


int fd;


volatile int STOP = FALSE;



int receiveFrame(int fd){

    unsigned char UA[5];

    // Returns after 5 chars have been input
    int bytes = read(fd, UA, 5);
    printf("read value");
    alarm(0);
    UA[bytes] = '\0'; // Set end of string to '\0', so we can printf

    printf("bytes: %d\n", bytes); 
    printf("UA[0]: %d\n", UA[0]);  
    printf("UA[1]: %d\n", UA[1]);  
    printf("UA[2]: %d\n", UA[2]);  
    printf("UA[3]: %d\n", UA[3]);  
    printf("UA[4]: %d\n", UA[4]);  


}

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, nosen echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    (void)signal(SIGALRM, alarmHandler);


    unsigned char BCC = A ^ C_SET;
    SET[3] = BCC;
    
    printf("FLAG = 0x%02X\n", FLAG);
    printf("A = 0x%02X\n", A);
    printf("C = 0x%02X\n", C_SET);
    printf("BCC = 0x%02X\n", BCC);


        printf("AlarmCount: %d\n", alarmCount);
        defineFdAndSet(fd,SET);
        sendFrame();
        printf("sent frame\n");
        alarm(3);
        printf("alarm in 3 seconds\n");
        //printf("popo: %d", receiveFrame(fd));
        receiveFrame(fd);
    
    

    


    // Wait until all bytes have been written to the serial port
    sleep(1);

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
