#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "twc.h"

#ifdef TWC_DEBUG
#	define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#	define PRINTF(fmt, ...) 
#endif
//------------------------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------------------------
void cb_dump(cbuffer_t* cb)
{
	PRINTF("cbuffer: %lx buffer: %lx buffer_size: %d write_ptr: %lx read_ptr : %lx \n",
		(long)cb, (long)&(cb->buffer[0]), cb->buffer_size, (long)cb->write_ptr,  (long)cb->read_ptr);
}

cbuffer_t* 	cb_new(size_t buffer_size)
{
	void* tmp = malloc(sizeof(cbuffer_t));
	cbuffer_t* cb = (cbuffer_t*) tmp;
	cb_init(cb, buffer_size);
	cb->was_malloced = true;
	return cb;
}
void  cb_init(cbuffer_t* cb, size_t buffer_size)
{
	cb->was_malloced = false;
	if( buffer_size != 0){
		cb->buffer = malloc(buffer_size);
		cb->buffer_size = buffer_size;		
	}else{
		cb->buffer = malloc(CB_BUFFER_SIZE);
		cb->buffer_size = CB_BUFFER_SIZE;
	}
	cb->read_ptr = &(cb->buffer[0]);
    cb->write_ptr = &(cb->buffer[0]);
}

int  cb_read_available(cbuffer_t* cb)
{
    long  avail;
    avail = 1 + (&cb->buffer[cb->buffer_size - 1]) - cb->read_ptr;
    return (int) avail;
}
int  cb_write_available(cbuffer_t* cb)
{
    long  avail;
    avail = cb->read_ptr - cb->write_ptr;
    return (int) avail;
}
void cb_advance_read_ptr(cbuffer_t* cb, int n)
{
    assert( n <= cb_read_available(cb) );
    if( (char*)(cb->read_ptr + n) > (&cb->buffer[cb->buffer_size -1 ]) ){
        // error off end of buffer
        assert(false);
    }
    else if( (char*)(cb->read_ptr + n) == (&cb->buffer[cb->buffer_size -1 ]) ){
        // hit end of buffer, reset ptr
        cb->read_ptr = &(cb->buffer[0]);
        
    }else if( (char*)(cb->read_ptr + n) < (&cb->buffer[cb->buffer_size -1 ]) ){
        // did not reach end of buffer
        cb->read_ptr += n;
    }else{
        // should never get here
        assert(false);
    }
    return;
}
void cb_advance_write(cbuffer_t* cb, int n)
{
    assert( n <= cb_write_available(cb) );
    if( (cb->write_ptr + n) > (&cb->buffer[cb->buffer_size - 1]) ){
        assert(false);
    }else if( (cb->write_ptr + n) == (&cb->buffer[cb->buffer_size - 1]) ){
        cb->write_ptr = &(cb->buffer[0]);
    }else if( (cb->write_ptr + n) < (&cb->buffer[cb->buffer_size - 1]) ){
        cb->write_ptr += n;
    }else{
        assert(false);
    }
}
void cb_clean(cbuffer_t* cb)
{
	assert(cb->was_malloced == false);
	free(cb->buffer);
	cb->buffer_size = 0;
	cb->buffer = NULL;
}
void cb_free(cbuffer_t* cb)
{
	assert(cb->was_malloced == true);
	cb->was_malloced = false;
	cb_clean(cb);
	free(cb);
}
//------------------------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------------------------
void sm_dump(state_machine_t* sm)
{
	char* tf = (sm->read_suspended)? "TRUE": "FALSE";
	
	PRINTF("state_machine : %lx read_fd: %d write_fd : %d read_suspended: %s cbuffer: %lx\n",
		(long)sm, sm->read_fd, sm->write_fd, tf, (long)&sm->cbuffer);
}


void sm_init(state_machine_t* sm, int read_fd, int write_fd, size_t buffer_size)
{
    sm->read_fd = read_fd;
    sm->write_fd  = write_fd;
    //
    // TODO: make sure fds are in non-blocking mode - need to do this
    //
    sm->read_suspended = false;
    cb_init( &(sm->cbuffer), buffer_size );
}
void sm_clean(state_machine_t* sm)
{
    cb_clean(&sm->cbuffer);
}
void sm_execute(state_machine_t* sm, fd_set* read_fds, fd_set* write_fds, int* status)
{
    int w_avail;
    int r_avail;
    // do write first
    if( FD_ISSET(sm->write_fd, write_fds) ){
        w_avail = cb_write_available(&sm->cbuffer);
        if( w_avail > 0){
            //
            // TODO : do write and advance pointer what about EOF and io errors
            //
            PRINTF("state_machine sm: %lx write avail %d \n", (long)sm, w_avail);
            int n = (int)write(sm->write_fd, sm->cbuffer.write_ptr, w_avail);
            if( n == 0 ){
                *status = SM_IOERR;
            }else if( n > 0 ){
                cb_advance_write(&sm->cbuffer, n);
                *status = SM_OK;
            }else{
                *status = SM_IOERR;
			}
        }
    }
    if( FD_ISSET(sm->read_fd, read_fds) && ! (sm->read_suspended) ){
        r_avail = cb_read_available(&sm->cbuffer);
        if( r_avail > 0 ){
            //
            // TODO: do a read, advance the pointer, what about io errors
            //
            int n = (int)read(sm->read_fd, sm->cbuffer.read_ptr, r_avail );
            if( n == 0 ){
                *status = SM_EOF;
            }else if( n > 0 ){
                PRINTF("state_machine sm: %lx read n: %d\n", (long)sm, n);
                cb_advance_read_ptr(&sm->cbuffer, n);
                *status = SM_OK;
            }else{
                *status = SM_IOERR;
            }
        }
    }
    if( *status != 0 ){
		PRINTF("sm: %ld exiting with status %d \n", (long)sm, *status);
        return;
	}
    w_avail = cb_write_available(&sm->cbuffer);
    r_avail = cb_read_available(&sm->cbuffer);
    if( w_avail > 0 )
        FD_SET(sm->write_fd, write_fds);
    if( r_avail > 0 ){
        FD_SET(sm->read_fd, read_fds);
        sm->read_suspended = false;
    }else{
        FD_CLR(sm->read_fd, read_fds);
        sm->read_suspended = true;
    }
    *status = SM_OK;
}

//------------------------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------------------------

void twc_dump(two_way_channel_t* twc)
{
	PRINTF("twc: %lx fdA : %d fdB : %d sm_A2B: %lx sm_B2A: %lx\n",
		(long)twc, twc->fdA, twc->fdB, (long)&twc->sm_A2B, (long)&twc->sm_B2A);
}
void twc_init(two_way_channel_t* twc, int fdA, int fdB, size_t buffer_size)
{
	twc->fdA = fdA;
	twc->fdB = fdB;
    sm_init(&twc->sm_A2B, fdA, fdB, buffer_size);
    sm_init(&twc->sm_B2A, fdB, fdA, buffer_size);
}
void twc_clean(two_way_channel_t* twc)
{
    sm_clean(&twc->sm_A2B);
    sm_clean(&twc->sm_B2A);
}
void twc_run(two_way_channel_t* twc, int* status)
{
    fd_set  read_fds, write_fds;
    struct timeval tv;
    int retval;
    int saved_errno;
    
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    int statusA2B = SM_OK;
    int statusB2A = SM_OK;
    
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    sm_execute(&twc->sm_A2B, &read_fds, &write_fds, &statusA2B);
    sm_execute(&twc->sm_B2A, &read_fds, &write_fds, &statusB2A);
    int max_fd = (twc->fdA > twc->fdB) ? twc->fdA : twc->fdB;
    for(;;)
    {
        do{
            retval = select(max_fd + 1, &read_fds, &write_fds, NULL, &tv);
            saved_errno = errno;
        } while( (retval < 0) && ( saved_errno == EINTR));
        if( retval == -1 )
        {
            // an io error of some kind happened
            *status = SM_IOERR;
            twc->errorno = errno;
            return;
        }
        else if( retval > 0 )
        {
            
            sm_execute(&twc->sm_A2B, &read_fds, &write_fds, &statusA2B);
            sm_execute(&twc->sm_B2A, &read_fds, &write_fds, &statusB2A);
            
            if( statusA2B != SM_OK ){
                PRINTF("twc : %lx A2B exiting status: %d \n", (long)twc, statusA2B);
                *status = statusA2B;
                return;
            }else if( statusB2A != SM_OK ){
                PRINTF("twc : %lx B2A exiting status: %d \n", (long)twc, statusB2A);
                *status = statusB2A;
                return;
            }else{
                // keep going
            }
        }
        else
        {
            // a time out happened
            *status = SM_TO;
            return;
        }
    }
}

