#ifndef PIPE_UTES_H
#define PIPE_UTES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

typedef struct thread_data_s{
    pthread_t thread_id;
    int			fd;
	FILE*		f;
    char*		message;
} thread_data_t;

typedef struct decorator_thread_data_s{
    pthread_t thread_id;
    int			fd_in; 	// the read end of a pipe	
    int			fd_out;	// the write end of a pipe
	FILE*		f_in;
	FILE*		f_out;
    char*		decoration;
} decorator_thread_data_t;

void start_twc_thread(pthread_t *th, int fd_in, int fd_out, char* decoration);

/*
* 	start a thread that reads from fd_in and writes to fd_out. 
* 	It is designed to connect two pipes, thus
*
* 	If the fds have been created with the pipe() system call
* 	then fd_in = pipe_array[0] where pipe_array is the soure pipe,
*		that is the "read" end of the source pipe
*
*	and 
*
*	fd_out =  pipe_array[1] where pipe_array is the destination pipe,
*		that is the "write" end of the destination pipe
*
*/
void start_decorator_thread(pthread_t *th, int fd_in, int fd_out, char* decoration);
void* decorator_thread_func(void* arg);

/*
* 	start a thread that puts data into a pipe. 
*
* 	If the fd have been created with the pipe() system call
* 	then fd = pipe_array[1] where pipe_array is the pipe to be filled with data
* 	that is the "write" end of the pipe
*/
void start_source_thread(pthread_t *th, int fd_in);
void* source_thread_func(void* arg);

/*
* 	start a thread that reads and displays data from a pipe. 
*
* 	If the fd have been created with the pipe() system call
* 	then fd = pipe_array[0] where pipe_array is the pipe to be filled with data
*	that is the read end of the pipe
*/
void start_dest_thread(pthread_t *th, int fd_out);
void* dest_thread_func(void* arg);

#define PIPE_IN(p) p[1]
#define PIPE_WRITE(p) p[1]
#define PIPE_OUT(p) p[0]
#define PIPE_READ(p) p[0]

#endif