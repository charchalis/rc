#include "linkLayer.h"

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
int retransmitions = 0;
unsigned char tramaTx = 0;
unsigned char tramaRx = 1;


int sendFrame(int fd, unsigned char A, unsigned char C){
    unsigned char FRAME[5] = {FLAG, A, C, A ^ C, FLAG};
    return write(fd, FRAME, 5);
}

void alarmHandler(int signal) {
    alarmEnabled = TRUE;
    alarmCount++;
}

int llopen(LinkLayer link) {


//-----------------------------------CONFIG PORT------------------//

    const char *serialPortName = link.serialPort;

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


    
//-----------------------------------ROLES------------------//


    LinkLayerState state = START;
    unsigned char byte;
    retransmitions = link.nRetransmissions;
    
    // Handle different roles
    switch (link.role) {
        case transmitter: {

            printf("transmitter llopen\n");                  
            
            (void) signal(SIGALRM, alarmHandler);
            
            printf("retransmissions: %d\n", retransmitions);

            // Main loop for the transmitter role
            while (retransmitions > 0 && state != STOP_STATE) {
                // Send frame (C_SET)
                printf("sending frame\n");
                sendFrame(fd, A_ER, C_SET);
                alarm(link.timeout);
                alarmEnabled = FALSE;

                // Receive and process frames
                while (alarmEnabled == FALSE && state != STOP_STATE) {
                    if (read(fd, &byte, 1) > 0) {
                        printf("state: %d\n", state);
                        printf("reading byte: %d\n", byte);
                        switch (state) {
                            case START:
                                if (byte == FLAG) state = FLAG_RCV;
                                break;
                            case FLAG_RCV:
                                if (byte == A_RE) state = A_RCV;
                                else if (byte != FLAG) state = START;
                                break;
                            case A_RCV:
                                if (byte == C_UA) state = C_RCV;
                                else if (byte == FLAG) state = FLAG_RCV;
                                else state = START;
                                break;
                            case C_RCV:
                                if (byte == (A_RE ^ C_UA)) state = BCC1_OK;
                                else if (byte == FLAG) state = FLAG_RCV;
                                else state = START;
                                break;
                            case BCC1_OK:
                                printf("Fdsafjkdslaçfdsa");
                                if (byte == FLAG) state = STOP_STATE;
                                else state = START;
                                printf("state: %d", STOP_STATE);
                                break;
                            default: 
                                break;
                        }
                    }
                } 
                retransmitions--;
            }

            // Handle completion conditions
            if (state != STOP_STATE) {
                printf("ERROR: did not reach STOP state\n");
                // Handle failure
                return -1;
            }
            printf("Connection successfull\n");
            break;
        }
        case receiver: {
            // Main loop for the receiver role
            printf("role: receiver\n");
            while (state != STOP_STATE) {
                if (read(fd, &byte, 1) > 0) {
                    printf("reading byte: %d\n", byte);
                    switch (state) {
                        case START:
                            if (byte == FLAG) state = FLAG_RCV;
                            break;
                        case FLAG_RCV:
                            if (byte == A_ER) state = A_RCV;
                            else if (byte != FLAG) state = START;
                            break;
                        case A_RCV:
                            if (byte == C_SET) state = C_RCV;
                            else if (byte == FLAG) state = FLAG_RCV;
                            else state = START;
                            break;
                        case C_RCV:
                            if (byte == (A_ER ^ C_SET)) state = BCC1_OK;
                            else if (byte == FLAG) state = FLAG_RCV;
                            else state = START;
                            break;
                        case BCC1_OK:
                            if (byte == FLAG) state = STOP_STATE;
                            else state = START;
                            break;
                        default: 
                            break;
                    }
                }
            }  
            
            printf("response\n");
            
            // Send response frame (C_UA) 
            sendFrame(fd, A_RE, C_UA);
            break;
        }
        default:
            return -1; // Unsupported role
    }
    
    // Close the serial port and return it
    // close(fd);  
    return fd;

}

int llread(int fd, unsigned char *packet){
    
    unsigned char byte, cField;
    int i = 0;
    LinkLayerState state = START;

    while (state != STOP) {  
        if (read(fd, &byte, 1) > 0) {
            switch (state) {
                default: printf("on llread state machine"); break;
            }    
        }
    }
    return -1;
}

int llwrite(int fd, const unsigned char *buf, int bufSize) {

    //TODO: Payload/Packet (beez)
    //TODO: Stuffing
    //TODO: Frame it and send it (F,A,C,BCC,...,F)
    //TODO: Receive RRx

    int frameSize = 6+bufSize;
    unsigned char *frame = (unsigned char *) malloc(frameSize);
    frame[0] = FLAG;
    frame[1] = A_ER;
    frame[2] = C_N(tramaTx);
    frame[3] = frame[1] ^frame[2];
    memcpy(frame+4,buf, bufSize);
    unsigned char BCC2 = buf[0];
    for (unsigned int i = 1 ; i < bufSize ; i++) BCC2 ^= buf[i];

    int j = 4;
    for (unsigned int i = 0 ; i < bufSize ; i++) {
        if(buf[i] == FLAG || buf[i] == ESC) {
            frame = realloc(frame,++frameSize);
            frame[j++] = ESC;
        }
        frame[j++] = buf[i];
    }
    frame[j++] = BCC2;
    frame[j++] = FLAG;

    int currentTransmition = 0;
    int rejected = 0, accepted = 0;

    while (currentTransmition < retransmitions) { 
        alarmEnabled = FALSE;
        alarm(timeout);
        rejected = 0;
        accepted = 0;
        while (alarmEnabled == FALSE && !rejected && !accepted) {

            write(fd, frame, j);
            unsigned char result = readControlFrame(fd);
            
            if(!result){
                continue;
            }
            else if(result == C_REJ(0) || result == C_REJ(1)) {
                rejected = 1;
            }
            else if(result == C_RR(0) || result == C_RR(1)) {
                accepted = 1;
                tramaTx = (tramaTx+1) % 2;
            }
            else continue;

        }
        if (accepted) break;
        currentTransmition++;
    }
    
    free(frame);
    if(accepted) return frameSize;
    else{
        llclose(fd);
        return -1;
    }

    return 1;

}

int llclose(int fd){

    //TODO: send DISC (Tx)
    //TODO: receive DISC (Rx)
    //TODO: receive DISC (Tx)
    //TODO: send DISC (Rx)
    //TODO: send UA (Tx) 
    //TODO: receive UA (Rx)
    return 1;
}

