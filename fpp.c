#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "fpp.h"
#include "err.h"

extern FPP* fpp_open(int fd) {
  FPP* fpp=(FPP *)malloc(sizeof(*fpp));
  if (!fpp)
    ERR("fpp_open(): malloc() failed");
  fpp->r=fdopen(fd,"r");
  fpp->w=fdopen(dup(fd),"w");
  if (!fpp->r)
    ERR("fpp_open(): fdopen(\"r\") failed");
  if (!fpp->w)
    ERR("fpp_open(): fdopen(\"w\") failed");
  return fpp;
}

extern void fpp_close(FPP* fpp) {
  if (fclose(fpp->r))
    ERR("fpp_close(): fclose(\"r\") failed: %s",strerror(errno));
  if (fclose(fpp->w))
    ERR("fpp_close(): fclose(\"w\") failed: %s",strerror(errno));
  free(fpp);
}
