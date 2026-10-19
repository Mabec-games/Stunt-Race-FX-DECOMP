/* host_posix.c: time and files for POSIX systems (macOS, Linux, iOS). */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#include "host.h"

uint64_t host_ticks_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

void host_sleep_ms(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
        ;
}

void host_thread_interactive(void)
{
#ifdef __APPLE__
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
}

int host_mkdir(const char *path) { return mkdir(path, 0755) == 0 || errno == EEXIST; }
