#include "linkLayer.h"

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
int timeout = 0;
int retransmitions = 0;
unsigned char tramaTx = 0;
unsigned char tramaRx = 1;

int llopen(LinkLayer link) {


//-----------------------------------CONFIG PORT------------------//


    
    const char *serialPortName = port;

    printf("port: %s\n", serialPortName);

    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
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
    printf("aqui???\n");
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0; 

    // Set input mode (non-canonical, no echo,...)
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


    
//-----------------------------------READ/WRITE + STATE MACHINE------------------//

//...

}

int llread(int fd, unsigned char *packet){
    
    unsigned char byte, cField;
    int i = 0;
    LinkLayerState state = START;

    while (state != STOP) {  
        if (read(fd, &byte, 1) > 0) {
            switch (state) {
                //state machine
            }    
    }
    return -1;
}

int llwrite(int fd, const unsigned char *buf, int bufSize) {

    //TODO: Payload/Packet (beez)
    //TODO: Stuffing
    //TODO: Frame it and send it (F,A,C,BCC,...,F)
    //TODO: Receive RRx


}

int llclose(int fd){

    //TODO: send DISC (Tx)
    //TODO: receive DISC (Rx)
    //TODO: receive DISC (Tx)
    //TODO: send DISC (Rx)
    //TODO: send UA (Tx) 
    //TODO: receive UA (Rx)

}

