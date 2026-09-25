/*
 * GLIBC compatibility shims for Debian 11 (glibc 2.31).
 *
 * Symbols that moved or appeared in glibc 2.32-2.38:
 *   pthread_join, pthread_create, pthread_detach — moved from libpthread to libc in 2.34
 *   pthread_once         — moved from libpthread to libc in 2.34
 *   __pthread_key_create — moved from libpthread to libc in 2.34
 *   fstat64              — version bumped to GLIBC_2.33
 *   __libc_single_threaded — new data symbol in 2.32
 *   _dl_find_object      — new function in 2.35 (used by GCC unwinder)
 *   dl* functions        — moved from libdl to libc in 2.34
 *   __isoc23_strtol/ul   — new C23 functions in 2.38
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <math.h>

/* --- pthread_join: redirect to old GLIBC_2.2.5 version --- */
__asm__(".symver compat_pthread_join, pthread_join@GLIBC_2.2.5");
extern int compat_pthread_join(pthread_t, void **);

int __wrap_pthread_join(pthread_t thread, void **retval)
{
    return compat_pthread_join(thread, retval);
}

/* --- pthread_create: redirect to old GLIBC_2.2.5 version --- */
__asm__(".symver compat_pthread_create, pthread_create@GLIBC_2.2.5");
extern int compat_pthread_create(pthread_t *, const pthread_attr_t *,
                                  void *(*)(void *), void *);

int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine)(void *), void *arg)
{
    return compat_pthread_create(thread, attr, start_routine, arg);
}

/* --- pthread_detach: redirect to old GLIBC_2.2.5 version --- */
__asm__(".symver compat_pthread_detach, pthread_detach@GLIBC_2.2.5");
extern int compat_pthread_detach(pthread_t);

int __wrap_pthread_detach(pthread_t thread)
{
    return compat_pthread_detach(thread);
}

/* --- pthread_once: redirect to old GLIBC_2.2.5 version --- */
__asm__(".symver compat_pthread_once, pthread_once@GLIBC_2.2.5");
extern int compat_pthread_once(pthread_once_t *, void (*)(void));

int __wrap_pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
    return compat_pthread_once(once_control, init_routine);
}

/* --- __pthread_key_create: redirect to old GLIBC_2.2.5 version --- */
__asm__(".symver compat_pthread_key_create, __pthread_key_create@GLIBC_2.2.5");
extern int compat_pthread_key_create(pthread_key_t *, void (*)(void *));

int __wrap___pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    return compat_pthread_key_create(key, destructor);
}

/* --- fstat64: implement via syscall to avoid version dependency --- */
int __wrap_fstat64(int fd, struct stat64 *buf)
{
    long ret = syscall(SYS_fstat, fd, buf);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

/*
 * __libc_single_threaded (GLIBC_2.32): boolean flag.
 * 0 = multi-threaded (safe default).
 */
char __libc_single_threaded __attribute__((weak)) = 0;

/*
 * _dl_find_object (GLIBC_2.35): GCC unwinder optimization.
 * Return -1 = not found, unwinder falls back to dl_iterate_phdr.
 */
struct dl_find_object;
int _dl_find_object(void *address, struct dl_find_object *result) __attribute__((weak));
int _dl_find_object(void *address, struct dl_find_object *result)
{
    (void)address;
    (void)result;
    return -1;
}

/*
 * GLIBC 2.38 compatibility:
 * __isoc23_strtol/strtoul - C23 version of strtol/strtoul
 */

/* Use old strtoul */
__asm__(".symver compat_strtoul, strtoul@GLIBC_2.2.5");
extern unsigned long compat_strtoul(const char *, char **, int);

unsigned long __isoc23_strtoul(const char *nptr, char **endptr, int base)
{
    return compat_strtoul(nptr, endptr, base);
}

/* Use old strtol */
__asm__(".symver compat_strtol, strtol@GLIBC_2.2.5");
extern long compat_strtol(const char *, char **, int);

long __isoc23_strtol(const char *nptr, char **endptr, int base)
{
    return compat_strtol(nptr, endptr, base);
}

/*
 * GLIBC 2.34 compatibility:
 * dl* functions moved from libdl to libc in 2.34
 * Redirect to old GLIBC_2.2.5 versions
 */

__asm__(".symver compat_dlopen, dlopen@GLIBC_2.2.5");
extern void* compat_dlopen(const char *, int);
void* __wrap_dlopen(const char *filename, int flags)
{
    return compat_dlopen(filename, flags);
}

__asm__(".symver compat_dlclose, dlclose@GLIBC_2.2.5");
extern int compat_dlclose(void *);
int __wrap_dlclose(void *handle)
{
    return compat_dlclose(handle);
}

__asm__(".symver compat_dlsym, dlsym@GLIBC_2.2.5");
extern void* compat_dlsym(void *, const char *);
void* __wrap_dlsym(void *handle, const char *symbol)
{
    return compat_dlsym(handle, symbol);
}

__asm__(".symver compat_dladdr, dladdr@GLIBC_2.2.5");
extern int compat_dladdr(const void *, Dl_info *);
int __wrap_dladdr(const void *addr, Dl_info *info)
{
    return compat_dladdr(addr, info);
}

__asm__(".symver compat_dlinfo, dlinfo@GLIBC_2.3.3");
extern int compat_dlinfo(void *, int, void *);
int __wrap_dlinfo(void *handle, int request, void *info)
{
    return compat_dlinfo(handle, request, info);
}

/* fmodf - wrap to GLIBC_2.2.5 */
__asm__(".symver compat_fmodf, fmodf@GLIBC_2.2.5");
extern float compat_fmodf(float x, float y);

float __wrap_fmodf(float x, float y)
{
    return compat_fmodf(x, y);
}
