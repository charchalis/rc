#include <stdio.h>
#include "alarm.h"

int alarmEnabled = 0;
int alarmCount = 0;

int fd_alarm;
unsigned char* SET_alarm;


void defineFdAndSet(int fd, unsigned char* SET){
    fd_alarm = fd;
    SET_alarm = SET;
}

void sendFrame(){

    printf("SET = %s\n", SET_alarm);

    for (int i = 0; i < 5; i++)
    {
        printf("SET[%d]: 0x%02X\n", i, SET_alarm[i]);
    }

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    //buf[5] = '\n';

    int bytes = write(fd_alarm, SET_alarm, 5);
    printf("%d bytes written\n", bytes);

}


void alarmHandler(int signal) {
    
    printf("jfdkslafjdsaçkl\n");

    if(alarmCount < 100){
        sendFrame();
        alarmEnabled = 1;
        printf("Timeout/invalid value: Sent frame again (numretries = %d)\n", alarmCount);
        alarm(3);
        printf("another 3 seconds");
        alarmCount++;

    }
    else{
        printf("Number of retries exceeded (numretries = %d)\n", alarmCount);
        //finish = 1;
    }
}
