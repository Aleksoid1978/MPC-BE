#include <stdlib.h>
#include <string.h>
#include "threadpool.h"

/* ---------------------------------------------------------------------------
 * multi line macros
 */
#if !defined(_WIN32)
#define __pragma(a)
#endif

#define MULTI_LINE_MACRO_BEGIN  do {
#if defined(__GNUC__)
#define MULTI_LINE_MACRO_END \
    } while(0)
#else
#define MULTI_LINE_MACRO_END \
    __pragma(warning(push))\
    __pragma(warning(disable:4127))\
    } while(0)\
    __pragma(warning(pop))
#endif

#define CHECKED_MALLOC(var, type, size)\
    MULTI_LINE_MACRO_BEGIN\
    (var) = (type)malloc(size);\
    if ((var) == NULL) {\
        goto fail;\
    }\
    MULTI_LINE_MACRO_END
#define CHECKED_MALLOCZERO(var, type, size)\
    MULTI_LINE_MACRO_BEGIN\
    CHECKED_MALLOC(var, type, size);\
    memset(var, 0, size);\
    MULTI_LINE_MACRO_END

#define XCHG(type, a, b)\
    MULTI_LINE_MACRO_BEGIN\
    type __tmp = (a); (a) = (b); (b) = __tmp;\
    MULTI_LINE_MACRO_END


/**
 * ===========================================================================
 * function defines
 * ===========================================================================
 */

/* ---------------------------------------------------------------------------
 */
threadpool_job_t *uavs3d_frame_shift(threadpool_job_t **list)
{
    threadpool_job_t *job = list[0];
    int i;
    for (i = 0; list[i]; i++) {
        list[i] = list[i+1];
    }
    return job;
}


/* ---------------------------------------------------------------------------
 */
void uavs3d_frame_delete(threadpool_job_t *job)
{
    uavs3d_pthread_mutex_destroy(&job->mutex);
    uavs3d_pthread_cond_destroy(&job->cv);
    free(job);
}
/* ---------------------------------------------------------------------------
 */
void uavs3d_frame_delete_list(threadpool_job_t **list)
{
    int i = 0;
    if (!list) {
        return;
    }
    while (list[i]) {
        uavs3d_frame_delete(list[i++]);
    }
    free(list);
}

/* ---------------------------------------------------------------------------
 */
void uavs3d_sync_frame_list_delete(threadpool_job_list_t *slist)
{
    uavs3d_pthread_mutex_destroy(&slist->mutex);
    uavs3d_pthread_cond_destroy(&slist->cv_fill);
    uavs3d_pthread_cond_destroy(&slist->cv_empty);
    uavs3d_frame_delete_list(slist->list);
}

/* ---------------------------------------------------------------------------
 */
void uavs3d_sync_frame_list_push(threadpool_job_list_t *slist, threadpool_job_t *job)
{
    uavs3d_pthread_mutex_lock(&slist->mutex);
    while (slist->i_size == slist->i_max_size) {
        uavs3d_pthread_cond_wait(&slist->cv_empty, &slist->mutex);
    }
    slist->list[slist->i_size++] = job;
    uavs3d_pthread_cond_broadcast(&slist->cv_fill);
    uavs3d_pthread_mutex_unlock(&slist->mutex);
}

/* ---------------------------------------------------------------------------
 */
threadpool_job_t *uavs3d_sync_frame_list_pop(threadpool_job_list_t *slist)
{
    threadpool_job_t *job;
    uavs3d_pthread_mutex_lock(&slist->mutex);
    while (!slist->i_size) {
        uavs3d_pthread_cond_wait(&slist->cv_fill, &slist->mutex);
    }
    job = slist->list[--slist->i_size];
    slist->list[slist->i_size] = NULL;
    uavs3d_pthread_cond_broadcast(&slist->cv_empty);
    uavs3d_pthread_mutex_unlock(&slist->mutex);
    return job;
}

/* ---------------------------------------------------------------------------
 */
threadpool_job_t *uavs3d_sync_frame_list_pop_try(threadpool_job_list_t *slist)
{
    threadpool_job_t *job;
    uavs3d_pthread_mutex_lock(&slist->mutex);
    if (!slist->i_size) {
        uavs3d_pthread_mutex_unlock(&slist->mutex);
        return NULL;
    }
    job = slist->list[--slist->i_size];
    slist->list[slist->i_size] = NULL;
    uavs3d_pthread_cond_broadcast(&slist->cv_empty);
    uavs3d_pthread_mutex_unlock(&slist->mutex);
    return job;
}

/* ---------------------------------------------------------------------------
 */
int uavs3d_sync_frame_list_init(threadpool_job_list_t *slist, int max_size)
{
    if (max_size < 0) {
        return -1;
    }
    slist->i_max_size = max_size;
    slist->i_size = 0;
    CHECKED_MALLOCZERO(slist->list, threadpool_job_t **, (max_size + 1) * sizeof(threadpool_job_t *));
    if (uavs3d_pthread_mutex_init(&slist->mutex, NULL) ||
        uavs3d_pthread_cond_init(&slist->cv_fill, NULL) ||
        uavs3d_pthread_cond_init(&slist->cv_empty, NULL)) {
        return -1;
    }
    return 0;
fail:
    return -1;
}

static void uavs3d_threadpool_list_delete(threadpool_job_list_t *slist)
{
    int i;
    for (i = 0; slist->list[i]; i++) {
        free(slist->list[i]);
        slist->list[i] = NULL;
    }
    uavs3d_sync_frame_list_delete(slist);
}

static void uavs3d_threadpool_thread(threadpool_t *pool)
{
    void *handle = NULL;

    if (pool->init_func) {
        handle = pool->init_func(pool->init_arg);
    }

    while (!pool->exit) {
        threadpool_job_t *job = NULL;
        uavs3d_pthread_mutex_lock(&pool->run.mutex);
        while (!pool->exit && !pool->run.i_size) {
            uavs3d_pthread_cond_wait(&pool->run.cv_fill, &pool->run.mutex);
        }
        if (pool->run.i_size) {
            job = (void *)uavs3d_frame_shift(pool->run.list);
            pool->run.i_size--;
        }
        
        uavs3d_pthread_mutex_unlock(&pool->run.mutex);
        if (!job) {
            continue;
        }
        job->ret = job->func(handle, job->arg);   /* execute the function */
        if (job->wait) {
            uavs3d_sync_frame_list_push(&pool->done, (void *)job);
        }
        else {
            uavs3d_sync_frame_list_push(&pool->uninit, (void *)job);
        }
    }

    if (pool->deinit_func) {
        pool->deinit_func(handle);
    }

    pthread_exit(0);
}

int uavs3d_threadpool_init(threadpool_t **p_pool, int threads, int nodes, void* (*init_func)(void *), void *init_arg, void(*deinit_func)(void *))
{
    int i;
    threadpool_t *pool;

    if (threads <= 0) {
        return -1;
    }

    CHECKED_MALLOCZERO(pool, threadpool_t *, sizeof(threadpool_t));
    *p_pool = pool;

    pool->init_func   = init_func;
    pool->init_arg    = init_arg;
    pool->deinit_func = deinit_func;
    pool->threads     = threads;

    CHECKED_MALLOC(pool->thread_handle, uavs3d_pthread_t *, pool->threads * sizeof(uavs3d_pthread_t));

    if (uavs3d_sync_frame_list_init(&pool->uninit, nodes) ||
        uavs3d_sync_frame_list_init(&pool->run,    nodes) ||
        uavs3d_sync_frame_list_init(&pool->done,   nodes)) {
        goto fail;
    }

    for (i = 0; i < nodes; i++) {
        threadpool_job_t *job;
        CHECKED_MALLOC(job, threadpool_job_t*, sizeof(threadpool_job_t));
        uavs3d_sync_frame_list_push(&pool->uninit, (void *)job);
    }
    for (i = 0; i < pool->threads; i++) {
        if (uavs3d_pthread_create(pool->thread_handle + i, NULL, (uavs3d_tfunc_a_t)uavs3d_threadpool_thread, pool)) {
            goto fail;
        }
    }

    return 0;
fail:
    return -1;
}

void uavs3d_threadpool_run(threadpool_t *pool, void *(*func)(void *, void *), void *arg, int wait_sign)
{
    threadpool_job_t *job = (void *)uavs3d_sync_frame_list_pop(&pool->uninit);
    job->func = func;
    job->arg  = arg;
    job->wait = wait_sign;
    uavs3d_sync_frame_list_push(&pool->run, (void *)job);
}

int uavs3d_threadpool_run_try(threadpool_t *pool, void *(*func)(void*, void *), void *arg, int wait_sign)
{
    threadpool_job_t *job = (void *)uavs3d_sync_frame_list_pop_try(&pool->uninit);

    if (NULL == job) {
        return -1;
    }
    job->func = func;
    job->arg = arg;
    job->wait = wait_sign;
    uavs3d_sync_frame_list_push(&pool->run, (void *)job);
    return 0;
}

void *uavs3d_threadpool_wait(threadpool_t *pool, void *arg)
{
    threadpool_job_t *job = NULL;
    int i;
    void *ret;

    uavs3d_pthread_mutex_lock(&pool->done.mutex);
    while (!job) {
        for (i = 0; i < pool->done.i_size; i++) {
            threadpool_job_t *t = (void *)pool->done.list[i];
            if (t->arg == arg) {
                job = (void *)uavs3d_frame_shift(pool->done.list + i);
                pool->done.i_size--;
            }
        }
        if (!job) {
            uavs3d_pthread_cond_wait(&pool->done.cv_fill, &pool->done.mutex);
        }
    }
    uavs3d_pthread_mutex_unlock(&pool->done.mutex);

    ret = job->ret;
    uavs3d_sync_frame_list_push(&pool->uninit, (void *)job);
    return ret;
}

void *uavs3d_threadpool_wait_try(threadpool_t *pool, void *arg)
{
    threadpool_job_t *job = NULL;
    int i;
    void *ret;

    uavs3d_pthread_mutex_lock(&pool->done.mutex);

    for (i = 0; i < pool->done.i_size; i++) {
        threadpool_job_t *t = (void *)pool->done.list[i];
        if (t->arg == arg) {
            job = (void *)uavs3d_frame_shift(pool->done.list + i);
            pool->done.i_size--;
        }
    }

    uavs3d_pthread_mutex_unlock(&pool->done.mutex);

    if (job) {
        ret = job->ret;
        uavs3d_sync_frame_list_push(&pool->uninit, (void *)job);
        return ret;
    } else {
        return NULL;
    }
}



void uavs3d_threadpool_delete(threadpool_t *pool)
{
    int i;

    uavs3d_pthread_mutex_lock(&pool->run.mutex);
    pool->exit = 1;
    uavs3d_pthread_cond_broadcast(&pool->run.cv_fill);
    uavs3d_pthread_mutex_unlock(&pool->run.mutex);
    for (i = 0; i < pool->threads; i++) {
        uavs3d_pthread_join(pool->thread_handle[i], NULL);
    }

    uavs3d_threadpool_list_delete(&pool->uninit);
    uavs3d_threadpool_list_delete(&pool->run);
    uavs3d_threadpool_list_delete(&pool->done);
    free(pool->thread_handle);
    free(pool);
}