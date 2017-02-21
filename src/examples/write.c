#include <stdio.h>
#include <syscall.h>
#include <string.h>

int
main (int argc, char *argv[])
{
  write(1, argv[argc - 1], strlen(argv[argc - 1]));
  return 0;
}
