#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#if defined(_WIN32)
#include <windows.h>
#endif

typedef void*(*uavs3d_tfunc_a_t)(void *);
typedef volatile long atom_t;   // 32 bits, signed

#if defined(_WIN32)
#include "win32thread.h"
#else

#pragma comment(lib, "pthreadVC2.lib")

#include <pthread.h>
#define uavs3d_pthread_t                pthread_t
#define uavs3d_pthread_create           pthread_create
#define uavs3d_pthread_join             pthread_join
#define uavs3d_pthread_mutex_t          pthread_mutex_t
#define uavs3d_pthread_mutex_init       pthread_mutex_init
#define uavs3d_pthread_mutex_destroy    pthread_mutex_destroy
#define uavs3d_pthread_mutex_lock       pthread_mutex_lock
#define uavs3d_pthread_mutex_unlock     pthread_mutex_unlock
#define uavs3d_pthread_cond_t           pthread_cond_t
#define uavs3d_pthread_cond_init        pthread_cond_init
#define uavs3d_pthread_cond_destroy     pthread_cond_destroy
#define uavs3d_pthread_cond_broadcast   pthread_cond_broadcast
#define uavs3d_pthread_cond_wait        pthread_cond_wait
#define uavs3d_pthread_attr_t           pthread_attr_t
#define uavs3d_pthread_attr_init        pthread_attr_init
#define uavs3d_pthread_attr_destroy     pthread_attr_destroy
#define uavs3d_pthread_num_processors_np pthread_num_processors_np

#endif


/**
 * ===========================================================================
 * type defines
 * ===========================================================================
 */
typedef struct uavs3d_threadpool_job_t {
    void *(*func)(void *, void *);
    void *arg;
    void *ret;
    int wait;
    uavs3d_pthread_mutex_t mutex;
    uavs3d_pthread_cond_t  cv;
} threadpool_job_t;

typedef struct uavs3d_threadpool_job_list_t {
    threadpool_job_t   **list;
    int                     i_max_size;
    int                     i_size;
    uavs3d_pthread_mutex_t    mutex;
    uavs3d_pthread_cond_t     cv_fill;  /* event signaling that the list became fuller */
    uavs3d_pthread_cond_t     cv_empty; /* event signaling that the list became emptier */
} threadpool_job_list_t;

typedef struct uavs3d_threadpool_t {
    int            exit;
    int            threads;
    uavs3d_pthread_t *thread_handle;
    void* (*init_func)(void *);
    void (*deinit_func)(void *);
    void           *init_arg;

    threadpool_job_list_t uninit; /* list of jobs that are awaiting use */
    threadpool_job_list_t run;    /* list of jobs that are queued for processing by the pool */
    threadpool_job_list_t done;   /* list of jobs that have finished processing */
} threadpool_t;

int uavs3d_threadpool_init(threadpool_t **p_pool, int threads, int nodes, void*(*init_func)(void *), void *init_arg, void(*deinit_func)(void *));
void  uavs3d_threadpool_run(threadpool_t *pool, void *(*func)(void *, void *), void *arg, int wait_sign);
void *uavs3d_threadpool_wait(threadpool_t *pool, void *arg);
void  uavs3d_threadpool_delete(threadpool_t *pool);

int   uavs3d_threadpool_run_try(threadpool_t *pool, void *(*func)(void *, void *), void *arg, int wait_sign);
void *uavs3d_threadpool_wait_try(threadpool_t *pool, void *arg);


#endif /* #ifndef __THREADPOOL_H__ */
