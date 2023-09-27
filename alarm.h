#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>


#ifndef ALARM_H
#define ALARM_H

extern int alarmEnabled;
extern int alarmCount;

void alarmHandler(int signal);

#endif /* ALARM_H */
