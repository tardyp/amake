#include "path.h"
#include "field.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern HIDDEN char *absolute(char *file) {
  int len=0;
  len+=strlen(file);
  len++; // eos
  char *dir=0;
  if (*file!='/') {
    dir=getcwd(0,0);
    len+=strlen(dir);
    len++; // slash
  }
  char *path=(char *)malloc(len);
  if (!path)
    ERROR("absolute(): malloc() failed");
  *path=0;
  if (*file!='/') {
    strcat(path,dir);
    strcat(path,"/");
    free(dir);
  }
  strcat(path,file);
  return path;
}

extern HIDDEN char *path(char *file) {
  if (*file=='/')
    return file;
  if (strchr(file,'/'))
    return absolute(file);
  char *dir;
  int i=0;
  while ((dir=field(i++,getenv("PATH"),':'))) {
    if (!*dir) {
      dir=(char *)malloc(2);
      strcpy(dir,".");
    }
    int len=0;
    len+=strlen(dir);
    len++; // slash
    len+=strlen(file);
    len++; // eos
    char *p=(char *)malloc(len);
    if (!p)
      ERROR("path(): malloc() failed");
    *p=0;
    strcat(p,dir);
    strcat(p,"/");
    strcat(p,file);
    free(dir);
    if (!access(p,X_OK))
      return p;
    free(p);
  }
  ERROR("path(%s): cannot find it on $PATH", file);
  return 0;
}
