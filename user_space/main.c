#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define DEV_NAME "/dev/keyboard_inc"
#define MSG_MAX 32
#define MSG_RESET "0"

int32_t convert_to_int(unsigned char* data) {
    int32_t x = 0;
    
    x  = (int32_t)data[0];
    x |= (int32_t)(data[1]) << 8;
    x |= (int32_t)(data[2]) << 16;
    x |= (int32_t)(data[3]) << 24;
        
    return x;
}

int main(int argc, char *argv[])
{
    int fd;
    ssize_t num_read, num_written;
    unsigned char msg[MSG_MAX];
    int32_t counter = 0;
    
    for (int i = 0; i < MSG_MAX; i++) {
        msg[i] = 0;
    }

    int i = 0;

    while (1) {
        /* Open kernel */
        fd = open(DEV_NAME, O_RDWR);
        if (fd == -1)
        {
            perror("Cannot open file description from kernel\n");
            return -1;
        }
        
        /* Read value data from kernel */
        num_read = read(fd, &msg, MSG_MAX);
        if (num_read == -1)
        {
            perror("Cannot read information from kernel \n");
            return -1;
        }
        if (num_read) {
            counter = convert_to_int(msg);
            printf("returned: %d from the system call, num bytes read: %d (%d)\n", counter, (int)num_read, i);
        }
        
        i++;
        if (i % 100 == 0) {
            printf("Reset counter\n");
            
            num_written = write(fd, MSG_RESET, sizeof(MSG_RESET));
            if (num_written == -1) {
                perror("Cannot send message to kernel module \n");
                return -1;
            }
        }
        
        if (i == 300) {
            break;
        }
        
        close(fd);
        
        usleep(100000);
    }

    return 0;
}