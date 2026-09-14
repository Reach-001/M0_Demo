#include <sys/stat.h>
#include "zf_common_debug.h"
#include "zf_driver_uart.h"

int _close(int file)
{
    (void) file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void) file;
    if (st != 0) {
        st->st_mode = S_IFCHR;
    }
    return 0;
}

int _isatty(int file)
{
    (void) file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void) file;
    (void) ptr;
    (void) dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void) file;
    (void) ptr;
    (void) len;
    return 0;
}

/**
 * @brief   printf 底层输出 — 将数据通过调试串口发送
 *
 * newlib-nano 的 printf / puts / putchar 最终都调用本函数。
 * 之前实现为空导致所有串口打印被丢弃。
 * safe: main() 中 debug_init() 最先被调用，之后 printf 才工作
 */
int _write(int file, char *ptr, int len)
{
    (void) file;
    uart_write_buffer(DEBUG_UART_INDEX, (const uint8 *)ptr, len);
    return len;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void) pid;
    (void) sig;
    return -1;
}
