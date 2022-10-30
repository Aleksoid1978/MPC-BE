#ifndef __WIN32THREAD_H__
#define __WIN32THREAD_H__

#include <windows.h>

/* the following macro is used within cavs */
#undef ERROR

typedef struct uavs3d_pthread_t {
    void *handle;
    void *(*func)(void *arg);
    void *arg;
    void *ret;
} uavs3d_pthread_t;

#define uavs3d_pthread_attr_t int

/* the conditional variable api for windows 6.0+ uses critical sections and not mutexes */
typedef CRITICAL_SECTION uavs3d_pthread_mutex_t;

#define uavs3d_pthread_mutexattr_t int
#define pthread_exit(a)
/* This is the CONDITIONAL_VARIABLE typedef for using Window's native conditional variables on kernels 6.0+.
 * MinGW does not currently have this typedef. */
typedef struct uavs3d_pthread_cond_t {
    void *ptr;
} uavs3d_pthread_cond_t;

#define uavs3d_pthread_condattr_t int

int uavs3d_pthread_create(uavs3d_pthread_t *thread, const uavs3d_pthread_attr_t *attr,
                        void *(*start_routine)(void *), void *arg);
int uavs3d_pthread_join(uavs3d_pthread_t thread, void **value_ptr);

int uavs3d_pthread_mutex_init(uavs3d_pthread_mutex_t *mutex, const uavs3d_pthread_mutexattr_t *attr);
int uavs3d_pthread_mutex_destroy(uavs3d_pthread_mutex_t *mutex);
int uavs3d_pthread_mutex_lock(uavs3d_pthread_mutex_t *mutex);
int uavs3d_pthread_mutex_unlock(uavs3d_pthread_mutex_t *mutex);

int uavs3d_pthread_cond_init(uavs3d_pthread_cond_t *cond, const uavs3d_pthread_condattr_t *attr);
int uavs3d_pthread_cond_destroy(uavs3d_pthread_cond_t *cond);
int uavs3d_pthread_cond_broadcast(uavs3d_pthread_cond_t *cond);
int uavs3d_pthread_cond_wait(uavs3d_pthread_cond_t *cond, uavs3d_pthread_mutex_t *mutex);
int uavs3d_pthread_cond_signal(uavs3d_pthread_cond_t *cond);

#define uavs3d_pthread_attr_init(a) 0
#define uavs3d_pthread_attr_destroy(a) 0

int  uavs3d_win32_threading_init(void);
void uavs3d_win32_threading_destroy(void);

int uavs3d_pthread_num_processors_np(void);

#endif // #ifndef __WIN32THREAD_H__
