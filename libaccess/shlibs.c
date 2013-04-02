#include "shlibs.h"
#include "output.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern HIDDEN void shlibs(const char *call, char *file, char *mode) {
  char *ldd=getenv(ENV(LDD));
  if (!ldd)
    ldd="/lib/ld-linux.so.2";
  if (!strcmp(file,ldd))
    return;
  if (strcmp(mode,"x"))
    return;
  if (!getenv(ENV(SHLIBS)))
    return;
  DEBUG("in shlibs(%s %s %s)\n",call,file,mode);
  int pd[2];
  if (pipe(pd))
    ERROR("shlibs(): pipe() failed");
  if (!fork()) {
    // child
    dup2(pd[1],1);
    close(pd[0]);
    close(pd[1]);
    execl(ldd,ldd,"--verify","--list",file,(char *)NULL);
    ERROR("shlibs(): execl() failed");
  }
  // parent    
  FILE *deps=fdopen(pd[0],"r");
  if (!deps)
    ERROR("shlibs(): fdopen() failed");
  close(pd[1]);
  while (!feof(deps)) {
    if (fgetc(deps)=='=' && fgetc(deps)=='>') {
      char *dep=0;
      fscanf(deps,"%ms",&dep);
      if (dep && *dep=='/') {
	output(call,dep,"l");
	free(dep);
      }
      while (fgetc(deps)!='\n');
    }
  }
  fclose(deps);
}

