/* мінімальні заглушки syscalls  * для того, щоб лінкер не видавав помилок при відсутності ОС
  * (newlib-nano, без -specs=nosys.specs)
  *
 */
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

void _init(void) {}

void _exit(int status) {
  (void)status;
  while (1) {
    /* немає ОС, якій можна повернути керування */
  }
}

int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

int _getpid(void) { return 1; }

void* _sbrk(int increment) {
  (void)increment;
  errno = ENOMEM;
  return (void*)-1;
}
