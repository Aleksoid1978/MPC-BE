#if defined(_WIN32)

#include "win32thread.h"
#include <process.h>


/**
 * ===========================================================================
 * type defines
 * ===========================================================================
 */

/* number of times to spin a thread about to block on a locked mutex before retrying and sleeping if still locked */
#define AVS2_SPIN_COUNT 0

/* GROUP_AFFINITY struct */
typedef struct uavs3d_group_affinity_t {
    ULONG_PTR mask; // KAFFINITY = ULONG_PTR
    USHORT group;
    USHORT reserved[3];
} uavs3d_group_affinity_t;

typedef void (WINAPI *cond_func_t)(uavs3d_pthread_cond_t *cond);
typedef BOOL (WINAPI *cond_wait_t)(uavs3d_pthread_cond_t *cond, uavs3d_pthread_mutex_t *mutex, DWORD milliseconds);

typedef struct uavs3d_win32thread_control_t{
    /* global mutex for replacing MUTEX_INITIALIZER instances */
    uavs3d_pthread_mutex_t static_mutex;

    /* function pointers to conditional variable API on windows 6.0+ kernels */
    cond_func_t cond_broadcast;
    cond_func_t cond_init;
    cond_func_t cond_signal;
    cond_wait_t cond_wait;
} uavs3d_win32thread_control_t;

static uavs3d_win32thread_control_t thread_control;


/**
 * ===========================================================================
 * function defines
 * ===========================================================================
 */

/* _beginthreadex requires that the start routine is __stdcall */
static unsigned __stdcall uavs3d_win32thread_worker(void *arg)
{
    uavs3d_pthread_t *h = arg;
    h->ret = h->func(h->arg);
    return 0;
}

int uavs3d_pthread_create(uavs3d_pthread_t *thread, const uavs3d_pthread_attr_t *attr,
                        void *(*start_routine)(void *), void *arg)
{
    thread->func   = start_routine;
    thread->arg    = arg;
    thread->handle = (void *)_beginthreadex(NULL, 0, uavs3d_win32thread_worker, thread, 0, NULL);
    return !thread->handle;
}

int uavs3d_pthread_join(uavs3d_pthread_t thread, void **value_ptr)
{
    DWORD ret = WaitForSingleObject(thread.handle, INFINITE);
    if (ret != WAIT_OBJECT_0) {
        return -1;
    }
    if (value_ptr) {
        *value_ptr = thread.ret;
    }
    CloseHandle(thread.handle);
    return 0;
}

int uavs3d_pthread_mutex_init(uavs3d_pthread_mutex_t *mutex, const uavs3d_pthread_mutexattr_t *attr)
{
    return !InitializeCriticalSectionAndSpinCount(mutex, AVS2_SPIN_COUNT);
}

int uavs3d_pthread_mutex_destroy(uavs3d_pthread_mutex_t *mutex)
{
    DeleteCriticalSection(mutex);
    return 0;
}

int uavs3d_pthread_mutex_lock(uavs3d_pthread_mutex_t *mutex)
{
    static uavs3d_pthread_mutex_t init = { 0 };
    if (!memcmp(mutex, &init, sizeof(uavs3d_pthread_mutex_t))) {
        *mutex = thread_control.static_mutex;
    }
    EnterCriticalSection(mutex);
    return 0;
}

int uavs3d_pthread_mutex_unlock(uavs3d_pthread_mutex_t *mutex)
{
    LeaveCriticalSection(mutex);
    return 0;
}

/* for pre-Windows 6.0 platforms we need to define and use our own condition variable and api */
typedef struct uavs3d_win32_cond_t {
    uavs3d_pthread_mutex_t mtx_broadcast;
    uavs3d_pthread_mutex_t mtx_waiter_count;
    int waiter_count;
    HANDLE semaphore;
    HANDLE waiters_done;
    int is_broadcast;
} uavs3d_win32_cond_t;

int uavs3d_pthread_cond_init(uavs3d_pthread_cond_t *cond, const uavs3d_pthread_condattr_t *attr)
{
    uavs3d_win32_cond_t *win32_cond;
    if (thread_control.cond_init) {
        thread_control.cond_init(cond);
        return 0;
    }

    /* non native condition variables */
    win32_cond = calloc(1, sizeof(uavs3d_win32_cond_t));
    if (!win32_cond) {
        return -1;
    }
    cond->ptr = win32_cond;
    win32_cond->semaphore = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
    if (!win32_cond->semaphore) {
        return -1;
    }

    if (uavs3d_pthread_mutex_init(&win32_cond->mtx_waiter_count, NULL)) {
        return -1;
    }
    if (uavs3d_pthread_mutex_init(&win32_cond->mtx_broadcast, NULL)) {
        return -1;
    }

    win32_cond->waiters_done = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!win32_cond->waiters_done) {
        return -1;
    }

    return 0;
}

int uavs3d_pthread_cond_destroy(uavs3d_pthread_cond_t *cond)
{
    uavs3d_win32_cond_t *win32_cond;
    /* native condition variables do not destroy */
    if (thread_control.cond_init) {
        return 0;
    }

    /* non native condition variables */
    win32_cond = cond->ptr;
    CloseHandle(win32_cond->semaphore);
    CloseHandle(win32_cond->waiters_done);
    uavs3d_pthread_mutex_destroy(&win32_cond->mtx_broadcast);
    uavs3d_pthread_mutex_destroy(&win32_cond->mtx_waiter_count);
    free(win32_cond);

    return 0;
}

int uavs3d_pthread_cond_broadcast(uavs3d_pthread_cond_t *cond)
{
    uavs3d_win32_cond_t *win32_cond;
    int have_waiter = 0;
    if (thread_control.cond_broadcast) {
        thread_control.cond_broadcast(cond);
        return 0;
    }

    /* non native condition variables */
    win32_cond = cond->ptr;
    uavs3d_pthread_mutex_lock(&win32_cond->mtx_broadcast);
    uavs3d_pthread_mutex_lock(&win32_cond->mtx_waiter_count);

    if (win32_cond->waiter_count) {
        win32_cond->is_broadcast = 1;
        have_waiter = 1;
    }

    if (have_waiter) {
        ReleaseSemaphore(win32_cond->semaphore, win32_cond->waiter_count, NULL);
        uavs3d_pthread_mutex_unlock(&win32_cond->mtx_waiter_count);
        WaitForSingleObject(win32_cond->waiters_done, INFINITE);
        win32_cond->is_broadcast = 0;
    } else {
        uavs3d_pthread_mutex_unlock(&win32_cond->mtx_waiter_count);
    }
    return uavs3d_pthread_mutex_unlock(&win32_cond->mtx_broadcast);
}

int uavs3d_pthread_cond_signal(uavs3d_pthread_cond_t *cond)
{
    uavs3d_win32_cond_t *win32_cond;
    int have_waiter;
    if (thread_control.cond_signal) {
        thread_control.cond_signal(cond);
        return 0;
    }

    /* non-native condition variables */
    win32_cond = cond->ptr;
    uavs3d_pthread_mutex_lock(&win32_cond->mtx_waiter_count);
    have_waiter = win32_cond->waiter_count;
    uavs3d_pthread_mutex_unlock(&win32_cond->mtx_waiter_count);

    if (have_waiter) {
        ReleaseSemaphore(win32_cond->semaphore, 1, NULL);
    }
    return 0;
}

int uavs3d_pthread_cond_wait(uavs3d_pthread_cond_t *cond, uavs3d_pthread_mutex_t *mutex)
{
    uavs3d_win32_cond_t *win32_cond;
    int last_waiter;
    if (thread_control.cond_wait) {
        return !thread_control.cond_wait(cond, mutex, INFINITE);
    }

    /* non native condition variables */
    win32_cond = cond->ptr;

    uavs3d_pthread_mutex_lock(&win32_cond->mtx_broadcast);
    uavs3d_pthread_mutex_unlock(&win32_cond->mtx_broadcast);

    uavs3d_pthread_mutex_lock(&win32_cond->mtx_waiter_count);
    win32_cond->waiter_count++;
    uavs3d_pthread_mutex_unlock(&win32_cond->mtx_waiter_count);

    // unlock the external mutex
    uavs3d_pthread_mutex_unlock(mutex);
    WaitForSingleObject(win32_cond->semaphore, INFINITE);

    uavs3d_pthread_mutex_lock(&win32_cond->mtx_waiter_count);
    win32_cond->waiter_count--;
    last_waiter = !win32_cond->waiter_count && win32_cond->is_broadcast;
    uavs3d_pthread_mutex_unlock(&win32_cond->mtx_waiter_count);

    if (last_waiter) {
        SetEvent(win32_cond->waiters_done);
    }

    // lock the external mutex
    return uavs3d_pthread_mutex_lock(mutex);
}

int uavs3d_win32_threading_init(void)
{
    /* find function pointers to API functions, if they exist */
    HMODULE kernel_dll = GetModuleHandle(TEXT("kernel32"));
    thread_control.cond_init = (cond_func_t)GetProcAddress(kernel_dll, "InitializeConditionVariable");
    if (thread_control.cond_init) {
        /* we're on a windows 6.0+ kernel, acquire the rest of the functions */
        thread_control.cond_broadcast = (cond_func_t)GetProcAddress(kernel_dll, "WakeAllConditionVariable");
        thread_control.cond_signal = (cond_func_t)GetProcAddress(kernel_dll, "WakeConditionVariable");
        thread_control.cond_wait = (cond_wait_t)GetProcAddress(kernel_dll, "SleepConditionVariableCS");
    }
    return uavs3d_pthread_mutex_init(&thread_control.static_mutex, NULL);
}

void uavs3d_win32_threading_destroy(void)
{
    uavs3d_pthread_mutex_destroy(&thread_control.static_mutex);
    memset(&thread_control, 0, sizeof(uavs3d_win32thread_control_t));
}

int uavs3d_pthread_num_processors_np()
{
    DWORD_PTR system_cpus, process_cpus = 0;
    int cpus = 0;
    DWORD_PTR bit;

    /* GetProcessAffinityMask returns affinities of 0 when the process has threads in multiple processor groups.
     * On platforms that support processor grouping, use GetThreadGroupAffinity to get the current thread's affinity instead. */
#if ARCH_X86_64
    /* find function pointers to API functions specific to x86_64 platforms, if they exist */
    HANDLE kernel_dll = GetModuleHandle(TEXT("kernel32.dll"));
    BOOL (*get_thread_affinity)(HANDLE thread, uavs3d_group_affinity_t * group_affinity) = (void *)GetProcAddress(kernel_dll, "GetThreadGroupAffinity");
    if (get_thread_affinity) {
        /* running on a platform that supports >64 logical cpus */
        uavs3d_group_affinity_t thread_affinity;
        if (get_thread_affinity(GetCurrentThread(), &thread_affinity)) {
            process_cpus = thread_affinity.mask;
        }
    }
#endif
    if (!process_cpus) {
        GetProcessAffinityMask(GetCurrentProcess(), &process_cpus, &system_cpus);
    }
    for (bit = 1; bit; bit <<= 1) {
        cpus += !!(process_cpus & bit);
    }

    return cpus ? cpus : 1;
}

#endif