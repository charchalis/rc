#include "link_layer.h"

volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
int timeout = 0;
int retransmitions = 0;
unsigned char tramaNr = 0;



int sendFrame(int fd, unsigned char A, unsigned char C){
    unsigned char FRAME[5] = {FLAG, A, C, A ^ C, FLAG};
    
    return write(fd, FRAME, 5);
}

unsigned char readControlFrame(int fd){

    unsigned char byte, cField = 0;
    LinkLayerState state = START;

    //printf("READING CONTROL FRAME\n");
    
    while (state != STOP_STATE && alarmEnabled == FALSE) {  
        if (read(fd, &byte, 1) > 0) {
            //printf("state: %d\n", state);
            //printf("byte: %d\n", byte);
            switch (state) {
                case START:
                    if (byte == FLAG) state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (byte == A_RE) state = A_RCV;
                    else if (byte != FLAG) state = START;
                    break;
                case A_RCV:
                    if (byte == C_RR0 || byte == C_RR1 || byte == C_REJ0 || byte == C_REJ1 || byte == C_DISC){
                        state = C_RCV;
                        cField = byte;   
                    }
                    else if (byte == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case C_RCV:
                    if (byte == (A_RE ^ cField)) state = BCC1_OK;
                    else if (byte == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case BCC1_OK:
                    if (byte == FLAG){
                        state = STOP_STATE;
                    }
                    else state = START;
                    break;
                default: 
                    break;
            }
        } 
    } 
    return cField;
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
    timeout = link.timeout;
    
    // Handle different roles
    switch (link.role) {
        case transmitter: {

            //alarm setup
            (void) signal(SIGALRM, alarmHandler);
            
            printf("retransmissions: %d\n", retransmitions);

            // alarm loop
            while (retransmitions > 0 && state != STOP_STATE) {
                // Send frame (C_SET)
                //printf("sending frame\n");
                sendFrame(fd, A_ER, C_SET);
                alarm(link.timeout);
                alarmEnabled = FALSE;
                
                // Receive and process frames
                while (alarmEnabled == FALSE && state != STOP_STATE) {
                    
                    //reading response frame

                    if (read(fd, &byte, 1) > 0) {
                        //printf("state: %d\n", state);
                        //printf("reading byte: %d\n", byte);
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


            if (state != STOP_STATE) {
                printf("ERROR: did not reach STOP state\n");
                return -1;
            }
            printf("\nllopen: Connection successfull\n");
            break;
        }
        case receiver: {
            // Main loop for the receiver role
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
            
            printf("sending ua frame\n");
            //printf("state: %d\n", state);
            
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

int llread(int fd, unsigned char *buffer){
    
    //printf("LLREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEAD\n");
    
    unsigned char byte, c_type;
    int i = 0;
    LinkLayerState state = START;

    while (state != STOP_STATE) {  
        if (read(fd, &byte, 1) > 0) {
            //printf("state: %d\n", state);
            //printf("byte: %d\n", byte);
            switch (state) {
                case START:
                    if (byte == FLAG) state = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (byte == A_ER) state = A_RCV;
                    else if (byte != FLAG) state = START;
                    break;
                case A_RCV:
                    if (byte == C_I0 || byte == C_I1){
                        state = C_RCV;
                        c_type = byte;   
                    }
                    else if (byte == FLAG) state = FLAG_RCV;
                    else if (byte == C_DISC) {
                        sendFrame(fd, A_RE, C_DISC);
                        return 0;
                    }
                    else state = START;
                    break;
                case C_RCV:
                    if (byte == (A_ER ^ c_type)) state = DATA;
                    else if (byte == FLAG) state = FLAG_RCV;
                    else state = START;
                    break;
                case DATA:
                    if (byte == ESC) state = DATA_FOUND_ESC;
                    else if (byte == FLAG){
                        unsigned char bcc2 = buffer[i-1];
                        i--;
                        buffer[i] = '\0';
                        unsigned char acc = buffer[0];

                        for (unsigned int j = 1; j < i; j++)
                            acc ^= buffer[j];

                        if (bcc2 == acc){
                            //printf("llread: reached stop state\n");
                            //printf("llread: sending control frame\n");
                            state = STOP_STATE;
                            sendFrame(fd, A_RE, tramaNr == 0 ? C_RR1: C_RR0);
                            tramaNr = tramaNr == 0 ? 1 : 0;
                            return i; 
                        }
                        else{
                            printf("Error: retransmition\n");
                            sendFrame(fd, A_RE, tramaNr == 0 ? C_REJ0 : C_REJ1);
                            return -1;
                        };

                    }
                    else{
                        buffer[i++] = byte;
                    }
                    break;
                case DATA_FOUND_ESC:
                    state = DATA;
                    if (byte == ESC || byte == FLAG) buffer[i++] = byte;
                    else{
                        buffer[i++] = ESC;
                        buffer[i++] = byte;
                    }
                    break;
                default: 
                    break;
            }    
        }
    }
    return -1;
}

int llwrite(int fd, const unsigned char *buffer, int bufferLength) {
    
    //--------------------PAYLOAD/PACKET------------------//
    
    int frameSize = 6+bufferLength;  
    unsigned char *frame = (unsigned char *) malloc(frameSize); //frame fica com a length de frameSize
    frame[0] = FLAG;
    frame[1] = A_ER;
    frame[2] = tramaNr == 0 ? C_I0 : C_I1;
    frame[3] = frame[1] ^ frame[2];
    memcpy(frame+4,buffer, bufferLength);
    unsigned char BCC2 = buffer[0];


    for (unsigned int i = 1 ; i < bufferLength ; i++) BCC2 ^= buffer[i];


    //--------------------STUFFING------------------//
    
    int j = 4;
    for (unsigned int i = 0 ; i < bufferLength ; i++) {
        if(buffer[i] == FLAG || buffer[i] == ESC) {
            frame = realloc(frame,++frameSize);
            frame[j++] = ESC;
        }
        frame[j++] = buffer[i];
    }
    frame[j++] = BCC2;
    frame[j++] = FLAG;

    int currentTransmition = 0;
    int rejected = 0, accepted = 0;
    

    //trying to send to rx

    while (currentTransmition < retransmitions) { 
        alarmEnabled = FALSE;
        alarm(timeout);
        rejected = 0;
        accepted = 0;
        while (alarmEnabled == FALSE && !rejected && !accepted) {

            write(fd, frame, j);
            printf("write successful\n");
            unsigned char cField = readControlFrame(fd);
            
            printf("reading Control Frame cField: %d\n", cField);
            
            if(!cField){
                continue;
            }
            else if(cField == C_REJ0 || cField == C_REJ1) {
                rejected = 1;
            }
            else if(cField == C_RR0 || cField == C_RR1) {
                accepted = 1;
                tramaNr = tramaNr == 0 ? 1 : 0;
            }
            else continue;

        }
        if (accepted){
            printf("receiver accepted the frame sent\n");
            break;
        }
        currentTransmition++;
    }
    
    //printf("about to free frame\n");
    
    free(frame); //free memory
    if(accepted) return frameSize;
    else{
        printf("receiver did not accept the frame sent\n");
        //llclose(fd, transmitter);
        return -1;
    }

    return 1;

}

int llclose(int fd, LinkLayerRole role){
    
    LinkLayerState state = START;
    unsigned char byte;
    (void) signal(SIGALRM, alarmHandler);
    

    if(role == receiver){
        printf("llclose receiver\n");
        
        ///printf("llclose: analysing disc\n");
        while (1) {
            int c_type;
            if (read(fd, &byte, 1) > 0) {
                //printf("llclose state: %d\n", state);
                //printf("byte: %d\n", byte);
                switch (state) {
                    case START:
                        if (byte == FLAG) state = FLAG_RCV;
                        break;
                    case FLAG_RCV:
                        if (byte == A_ER) state = A_RCV;
                        else if (byte != FLAG) state = START;
                        break;
                    case A_RCV:
                        if (byte == C_DISC){ state = C_RCV; c_type = C_DISC; }
                        else if (byte == C_UA) { state = C_RCV; c_type = C_UA; }
                        else if (byte == FLAG) state = FLAG_RCV;
                        else state = START;
                        break;
                    case C_RCV:
                        if (byte == (A_ER ^ c_type)) state = BCC1_OK;
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
            if(state == STOP_STATE){
                if(c_type == C_UA){
                    printf("closing llclose\n");
                    return close(fd);
                }
                
                printf("llclose: sending disc response frame\n");
                sendFrame(fd, A_RE, C_DISC);
                state = START;
                //break;
            }
        } 
    }
    else {
        while (retransmitions != 0 && state != STOP_STATE) {
                    
            printf("llclose: sending disc\n");
            sendFrame(fd, A_ER, C_DISC); //send Disc (Tx)
            alarm(timeout);
            alarmEnabled = FALSE;
                    
            printf("llclose: receiving disc\n");
            while (alarmEnabled == FALSE && state != STOP_STATE) {
                if (read(fd, &byte, 1) > 0) {
                    printf("llclose state: %d\n", state);
                    printf("byte: %d\n", byte);
                    switch (state) {
                        case START:
                            if (byte == FLAG) state = FLAG_RCV;
                            break;
                        case FLAG_RCV:
                            if (byte == A_RE) state = A_RCV;
                            else if (byte != FLAG) state = START;
                            break;
                        case A_RCV:
                            if (byte == C_DISC) state = C_RCV;
                            else if (byte == FLAG) state = FLAG_RCV;
                            else state = START;
                            break;
                        case C_RCV:
                            if (byte == (A_RE ^ C_DISC)) state = BCC1_OK;
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
            retransmitions--;
        }
        
        
        if (state != STOP_STATE) return -1;
        
        printf("llclose: valid frame\n");
        
        printf("llclose: sending ua frame\n");

        sendFrame(fd, A_ER, C_UA);
        printf("closing llclose\n");
        return close(fd);
    }
}


