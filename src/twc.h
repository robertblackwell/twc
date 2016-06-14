#ifndef TWC_H
#define TWC_H

#include <stdio.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

/*-------------------------------------------------------------------------------
* circular buffer
---------------------------------------------------------------------------------*/

struct cbuffer_s;
typedef struct cbuffer_s cbuffer_t;

#define CB_BUFFER_SIZE 1000
struct cbuffer_s{
    char*	buffer;
    int		buffer_size;
    char*	read_ptr;
    char*	write_ptr;
	bool	was_malloced;
    
};


#ifdef __cplusplus
extern "C"
{
#endif

cbuffer_t* 	cb_new(size_t buffer_size);
void		cb_init(cbuffer_t* cb, size_t buffer_size);
int 		cb_read_available(cbuffer_t* cb);
int 		cb_write_available(cbuffer_t* cb);
void 		cb_advance_read_ptr(cbuffer_t* cb, int n);
void		cb_advance_write_ptr(cbuffer_t* cb, int n);
void		cb_dump(cbuffer_t* cb);
void 		cb_clean(cbuffer_t* cb);
void 		cb_free(cbuffer_t* cb);

#ifdef __cplusplus
}
#endif


/*-------------------------------------------------------------------------------
* state_machine interface
---------------------------------------------------------------------------------*/

#define SM_OK       0
#define SM_EOF      -4001
#define SM_IOERR    -4002
#define SM_TO       -4003


struct state_machine_s;
typedef struct state_machine_s state_machine_t;

struct state_machine_s {
    
    int         read_fd;
    int         write_fd;
    bool        read_suspended;
    cbuffer_t   cbuffer;
};


#ifdef __cplusplus
extern "C"
{
#endif

void sm_init(state_machine_t* sm, int read_fd, int write_fd, size_t buffer_size);
void sm_execute(state_machine_t* sm, fd_set* read_fds, fd_set* write_fds, int* status);
void sm_dump(state_machine_t* sm);
void sm_clean(state_machine_t* sm);

#ifdef __cplusplus
}
#endif


/*-------------------------------------------------------------------------------
* two_way_channel interface
---------------------------------------------------------------------------------*/

struct two_way_channel_s;
typedef struct two_way_channel_s two_way_channel_t;

#define SM_OK       0
#define SM_EOF      -4001
#define SM_IOERR    -4002
#define SM_TO       -4003

struct two_way_channel_s {
    int             fdA;
    state_machine_t sm_A2B;
    
    int             fdB;
    state_machine_t sm_B2A;
    
    int             errorno;
    
};

#ifdef __cplusplus
extern "C"
{
#endif
void twc_init(two_way_channel_t* twc, int fdA, int fdB, size_t buffer_size);
void twc_run(two_way_channel_t* twc, int* status);
void twc_dump(two_way_channel_t* twc);
void twc_clean(two_way_channel_t* twc);

#ifdef __cplusplus
}
#endif

#endif