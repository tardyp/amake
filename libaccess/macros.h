#include <stdlib.h>
#include <errno.h>

#ifndef MACROS_H
#define MACROS_H

#define HIDDEN __attribute__ ((__visibility__("hidden")))

#ifndef NAME
#define NAME "ACCESS"
#endif

#define ENV(s) NAME "_" #s

#define DEBUG(...)                 \
  do                               \
    if (getenv(ENV(DEBUG)))        \
      fprintf(stderr,__VA_ARGS__); \
  while (0)

#define ERROR(...)                            \
  do {                                        \
    fprintf(stderr,__VA_ARGS__);              \
    fprintf(stderr,": %s\n",strerror(errno)); \
    exit(errno);                              \
  } while (0)

#endif
