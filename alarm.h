#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>


#ifndef ALARM_H
#define ALARM_H

extern int alarmEnabled;
extern int alarmCount;

int fd_alarm;
unsigned char* SET_alarm;

void defineFdAndSet(int fd, unsigned char* SET);

int sendFrame();

void alarmHandler(int signal);

#endif /* ALARM_H */
