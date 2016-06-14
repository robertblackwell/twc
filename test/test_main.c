#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>
#include <assert.h>
#include "pipe_utes.h"

//#define TWC_MULTI_PROCESS
#ifdef ORIGINAL
int main(int argc, char **argv) {
    int fd[2];
    pipe(fd);
    int pid = fork();
    if (pid != 0)
    {
        close(fd[0]);
        pthread_t th_write;
		start_source_thread(&th_write, fd[1]);
		pthread_join(th_write, NULL);
		close(fd[1]);
    }
    else if( pid == 0)
    {   // child: reading only, so close the write-descriptor
        printf("child process %d \n", getpid());
        close(fd[1]);
        pthread_t th_read;
		start_dest_thread(&th_read, fd[0]);
		start_dest_thread(&th_read, PIPE_READ(decor_1));
        pthread_join(th_read, NULL);
        close(fd[0]);
    }else{
        
    }
    
    return 0;
}
#else



#ifdef TWC_MULTI_PROCESS
int main(int argc, char **argv) {
    int fdA[2];
    int fdB[2];
    pipe(fdA);
    pipe(fdB);
    int pid = fork();
    if (pid != 0)
    {
        close(PIPE_READ(fdA));
        close(PIPE_WRITE(fdB));
        pthread_t th_write;
        pthread_t th_read;
		start_source_thread(&th_write, PIPE_WRITE(fdA));
		start_dest_thread(&th_read, PIPE_READ(fdB));
		pthread_join(th_write, NULL);
		close(PIPE_WRITE(fdA));
        close(PIPE_READ(fdB));
    }
    else if( pid == 0)
    {   // child: reading only, so close the write-descriptor
        printf("child process %d \n", getpid());
        close(PIPE_WRITE(fdA));
        close(PIPE_READ(fdB));
        pthread_t th_twc;
		start_twc_thread(&th_twc, PIPE_READ(fdA), PIPE_WRITE(fdB), "" );
        pthread_join(th_twc, NULL);
        close(PIPE_READ(fdA));
        close(PIPE_WRITE(fdB));
    }else{
        
    }
    
    return 0;
}
#else
int main(int argc, char **argv) {
    int fd[2];
    pipe(fd);
    int decor_1[2];
	pipe(decor_1);
    int decor_2[2];
	pipe(decor_2);
	
	pthread_t th_read;
    pthread_t th_write;
	pthread_t decorator1;
	pthread_t decorator2;
#ifdef TWODECS	
	start_source_thread(&th_write, PIPE_WRITE(fd) );
	start_decorator_thread(&decorator1, PIPE_READ(fd), PIPE_WRITE(decor_1), "DECORATOR 11111 " );
	start_decorator_thread(&decorator2, PIPE_READ(decor_1), PIPE_WRITE(decor_2), "DECORATOR 22222 " );
	start_dest_thread(&th_read, PIPE_READ(decor_2));
#else
	start_source_thread(&th_write, PIPE_WRITE(fd) );
	start_twc_thread(&decorator1, PIPE_READ(fd), PIPE_WRITE(decor_1), "DECORATOR 11111 " );
	start_dest_thread(&th_read, PIPE_READ(decor_1));
#endif	
	pthread_join(decorator1, NULL);
	pthread_join(decorator2, NULL);
	pthread_join(th_write, NULL);
    pthread_join(th_read, NULL);
	close(fd[1]);
    close(fd[0]);
    return 0;
}

#endif
#endif
