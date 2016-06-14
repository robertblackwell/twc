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
#include <twc.h>
#include "pipe_utes.h"

//
// start a thread that operates a select state machine
//
void start_twc_thread(pthread_t *th, int fd_in, int fd_out, char* decoration)
{
	int status;
	printf("start_twc_thread\n");
	two_way_channel_t twc;
	twc_init(&twc, fd_in, fd_out, 2000);
	printf("start_twc_thread\n");
	twc_run(&twc, &status);
	printf("twc complete status : %d \n", status);
}


void* decorator_thread_func(void* arg){
	decorator_thread_data_t *td = (decorator_thread_data_t*)arg;
    FILE* f_in = td->f_in;
	int fd_in = td->fd_in;
    FILE* f_out = td->f_out;
	int fd_out = td->fd_out;
	char* decoration = td->decoration;
	
	assert(f_in != NULL);
	assert(f_out != NULL);
	assert(fd_in == fileno(f_in));
	assert(fd_out == fileno(f_out));
	
	for(;;){
        char* 	buf_ptr;
        size_t	len;
        int cc;
        if( true){
            buf_ptr =malloc(1000);
            len = 1000;
            cc = (int)read(fd_in, buf_ptr, len);
        }else{
            buf_ptr =malloc(1000);
            len = 1000;
            cc = (int)getline(&buf_ptr, &len, f_in);
        }
        if( cc > 0 ){
			
	        fprintf(f_out, "%d %s [ %.*s ]", getpid(), decoration, cc, buf_ptr);
	        fflush(f_out);
			
        } else if( cc == EOF || cc == 0 ) {
            printf("Child EOF pid : %d got cc: %d len: %d errno: %d %s \n",
                   getpid(), cc, (int)len, errno, strerror(errno));
            break;
        }else{
            printf("Child ERROR pid : %d got cc: %d len: %d errno: %d %s \n",
                   getpid(), cc, (int)len, errno, strerror(errno));
            break;
        }
        free(buf_ptr);
    }
	
	fclose(f_out);
    return NULL;
}
void start_decorator_thread(pthread_t *th, int fd_in, int fd_out, char* decoration)
{
	decorator_thread_data_t* td = malloc(sizeof(decorator_thread_data_t));
	td->fd_in = fd_in;
	td->f_in = fdopen(fd_in, "r");
	td->fd_out = fd_out;
	td->f_out = fdopen(fd_out, "w");
	td->decoration = decoration;
	void* arg = (void*) td;
    int rc = pthread_create(th, NULL, decorator_thread_func, arg);
}

void* source_thread_func(void* arg){
	thread_data_t *td = (thread_data_t*)arg;
    FILE* f = td->f;
	int fd = td->fd;
	
	assert(f != NULL);
	assert(fd == fileno(f));
	
    for(int i = 0; i <10 ; i++){
        printf("Message from writer %d  %d \n", getpid(), i);
        fprintf(f, "Message from writer %d  %d \n", getpid(), i);
        fflush(f);
        sleep(1);
    }
    fclose(f);
	close(fd);
    return NULL;
}

void start_source_thread(pthread_t *th, int fd_in){
    int local_fd = fd_in;
    printf("start_write_thread fd %d \n", local_fd);
    FILE* f = fdopen(local_fd, "w");

    thread_data_t *td = malloc(sizeof(thread_data_t));
    td->fd = local_fd;
	td->f = f;
    td->message = "This is a message from write thread";
	
    int rc = pthread_create(th, NULL, source_thread_func, (void*) td);
	
}

void* dest_thread_func(void* arg){
	thread_data_t *td = (thread_data_t*)arg;
	FILE* f = td->f;
    int fd = fileno(f);
	
	assert(fd == td->fd);
    assert(f != NULL);
    
	for(;;){
        printf("Inside read loop pid: %d f: %lx \n", getpid(), (long)f);
        char* 	buf_ptr;
        size_t	len;
        int cc;
        if( true){
            buf_ptr =malloc(1000);
            len = 1000;
            cc = (int)read(fd, buf_ptr, len);
        }else{
            buf_ptr =malloc(1000);
            len = 1000;
            cc = (int)getline(&buf_ptr, &len, f);
        }
        if( cc > 0 ){
            printf("OUTPUT %d : %.*s", getpid(), (int)len, buf_ptr);
            // printf("Child GOOD pid : %d got cc: %d len: %d  [%.*s]\n",
            //        getpid(), cc, (int)len, (int)len, buf_ptr);
            
        } else if( cc == EOF || cc == 0 ) {
            printf("Child EOF pid : %d got cc: %d len: %d errno: %d %s \n",
                   getpid(), cc, (int)len, errno, strerror(errno));
            break;
        }else{
            printf("Child ERROR pid : %d got cc: %d len: %d errno: %d %s \n",
                   getpid(), cc, (int)len, errno, strerror(errno));
            break;
        }
        free(buf_ptr);
    }
    return NULL;
}

void start_dest_thread(pthread_t *th, int fd_out){

    FILE* fin = fdopen(fd_out, "r");
    thread_data_t *td = malloc(sizeof(thread_data_t));
    td->fd = fd_out;
	td->f = fin;
    td->message = "This is a message from write thread";
    int rc = pthread_create(th, NULL, dest_thread_func, (void*)td);
	
}

