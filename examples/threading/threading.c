#include "threading.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

static void ms_sleep(int ms) {
    if (ms <= 0) return;
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if (thread_func_args == NULL) {
        ERROR_LOG("threadfunc received NULL thread_param");
        return NULL;
    }
    // Wait to obtain the mutex
    ms_sleep(thread_func_args->wait_to_obtain_ms);  
    // Obtain the mutex
    

    int rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0) {
        
        ERROR_LOG("Failed to lock mutex: %s", strerror(rc));
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }
    // Wait while holding the mutex
    ms_sleep(thread_func_args->wait_to_release_ms);  
    // Release the mutex
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc != 0) {
        printf("hello");
        ERROR_LOG("Failed to unlock mutex: %s", strerror(rc));
        thread_func_args->thread_complete_success = false;
        return thread_func_args;
    }
    // Indicate successful completion
    thread_func_args->thread_complete_success = true;  

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data* tdata = malloc(sizeof(struct thread_data));
    if (tdata == NULL) {
        ERROR_LOG("Failed to allocate memory for thread_data"); 
        return false;
    }
    // Check for NULL mutex pointer
    if(mutex == NULL) {
        ERROR_LOG("Mutex pointer is NULL");
        free(tdata);
        return false;
    }
    // Check for NULL thread pointer
    if(thread == NULL) {
        ERROR_LOG("Thread pointer is NULL");
        free(tdata);
        return false;
    }   
    tdata->thread_complete_success = false;
    tdata->mutex = mutex;
    tdata->wait_to_obtain_ms = wait_to_obtain_ms;
    tdata->wait_to_release_ms = wait_to_release_ms;

    int result = pthread_create(thread, NULL, threadfunc, (void*)tdata);
    if (result == 0) {
        DEBUG_LOG("Thread created successfully");
        return true;
    } else {
        ERROR_LOG("Failed to create thread: %s", strerror(result));
        free(tdata);
    }
    return false;
}

