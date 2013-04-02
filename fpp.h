#ifndef FPP_H
#define FPP_H

typedef struct {
  FILE *r;
  FILE *w;
} FPP;

extern FPP* fpp_open(int fd);
extern void fpp_close(FPP* fpp);

#endif
