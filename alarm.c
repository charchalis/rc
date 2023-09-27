#include <stdio.h>
#include "alarm.h"
#include "sendFrame.h"

int alarmEnabled = 0;
int alarmCount = 0;


void alarmHandler(int signal) {
    
    printf("jfdkslafjdsaçkl\n");

    if(alarmCount < 3){
        sendFrame(fd, SET);
        alarmEnabled = 1;
        printf("Timeout/invalid value: Sent frame again (numretries = %d)\n", alarmCount);
        alarm(3);
        alarmCount++;

    }
    else{
        printf("Number of retries exceeded (numretries = %d)\n", alarmCount);
        //finish = 1;
    }
}
