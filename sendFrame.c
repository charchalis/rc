#include <stdio.h>
#include <unistd.h>

void sendFrame(int fd, unsigned char* SET){

    printf("SET = %s\n", SET);

    for (int i = 0; i < 5; i++)
    {
        printf("SET[%d]: 0x%02X\n", i, SET[i]);
    }

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    //buf[5] = '\n';

    int bytes = write(fd, SET, 5);
    printf("%d bytes written\n", bytes);
}