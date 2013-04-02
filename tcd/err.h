#ifndef ERR_H
#define ERR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "options.h"
#include "str.h"

extern void err_open();

#define LOG(args...) do {                      \
  err_open();                                  \
  fprintf(stderr,"%s.%s %s:%d: ",              \
	  str_date_yymmdd(),str_time_hhmmss(), \
	  __FILE__,__LINE__);		       \
  fprintf(stderr,args);                        \
  fprintf(stderr,"\n");                        \
  fflush(stderr);                              \
} while (0)

#define ERR(args...) do { \
  LOG(args);              \
  exit(1);                \
} while (0)

#define DEBUG(args...) do { \
  if (opt_debug())	    \
    LOG(args);              \
} while (0)

#endif
