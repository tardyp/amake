#ifndef ERR_H
#define ERR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "options.h"
#include "str.h"

#define LOGLOC(file,line,args...) do {         \
  fprintf(stderr,"%s.%s %s:%d: ",              \
          str_date_yymmdd(),str_time_hhmmss(), \
          file,line);                          \
  fprintf(stderr,args);                        \
  fprintf(stderr,"\n");                        \
  fflush(stderr);                              \
} while (0)

#define LOG(args...) LOGLOC(__FILE__,__LINE__,args)

#define ERRLOC(file,line,args...) do { \
  LOGLOC(file,line,args);              \
  exit(1);                             \
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
