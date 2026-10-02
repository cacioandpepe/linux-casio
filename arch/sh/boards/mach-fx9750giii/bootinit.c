/* SPDX-License-Identifier: GPL-2.0 */
/* Small native PID 1: no libc, floating point, dynamic loader or extra payload. */
#include <asm/unistd.h>

static long syscall3(long nr, long a, long b, long c)
{
    register long r3 __asm__("r3") = nr;
    register long r4 __asm__("r4") = a;
    register long r5 __asm__("r5") = b;
    register long r6 __asm__("r6") = c;
    register long r0 __asm__("r0");
    __asm__ volatile("trapa #0x13\n\t nop\n\t nop\n\t nop\n\t nop\n\t nop"
        : "=r"(r0) : "r"(r3), "r"(r4), "r"(r5), "r"(r6) : "memory", "t");
    return r0;
}

static void emit(long fd, const char *s, unsigned n)
{
    while(n) {
        long written = syscall3(__NR_write, fd, (long)s, n);
        if(written <= 0) return;
        s += written;
        n -= written;
    }
}

__attribute__((noreturn)) void _start(void)
{
    static const char started[] = "<6>Linux PID 1 started\n";
    static const char alive[] = "<6>Timer/syscalls OK\n";
    static const char failed[] = "<3>PID 1 sleep failed\n";
    struct { long seconds, nanoseconds; } interval = { 2, 0 };
    long fd = syscall3(__NR_open, (long)"/dev/kmsg", 1, 0);
    if(fd < 0 || syscall3(__NR_getpid, 0, 0, 0) != 1)
        syscall3(__NR_exit, 1, 0, 0);
    emit(fd, started, sizeof(started)-1);
    for(;;) {
        long rc = syscall3(__NR_nanosleep, (long)&interval, 0, 0);
        if(rc && rc != -4) {
            emit(fd, failed, sizeof(failed)-1);
            syscall3(__NR_exit, 2, 0, 0);
        }
        if(!rc) emit(fd, alive, sizeof(alive)-1);
    }
}
