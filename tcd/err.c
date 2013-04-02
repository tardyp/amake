#include "err.h"

extern void err_open() {
  if (!stderr) {
    char *cwd=getenv("PWD");
    if (!cwd)
      exit(errno);
    char *errfile=vstrcat(cwd,"/",opt_errfile(),".",str_pid(),NULL);
    FILE *f=fopen(errfile,"w");
    if (!f)
      exit(errno);
    free(errfile);
    if (dup2(fileno(f),2)<0)
      exit(errno);
    fclose(f);
    stderr=fdopen(2,"w");
    if (!stderr)
      exit(errno);
  }
}
