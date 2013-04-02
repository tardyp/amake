#define _GNU_SOURCE

#include "path.h"
#include "output.h"

#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>

#define GETREAL(ret,args)   \
  static ret (*real)args=0; \
  if (!real)                \
    real=(ret (*)args)dlsym(RTLD_NEXT,__func__)

#define PERM                 \
  va_list list;              \
  va_start(list,mode);       \
  int perm=va_arg(list,int); \
  va_end(list)

#define L2VE                                 \
  va_list args;                              \
  int argc;                                  \
  va_start(args,arg);                        \
  for (argc=0; va_arg(args,char *); argc++); \
  va_end(args);                              \
  char *argv[argc+2];                        \
  char **envp __attribute__ ((unused));	     \
  int i=0;                                   \
  argv[i++]=arg;                             \
  va_start(args,arg);                        \
  while (i<=argc)                            \
    argv[i++]=va_arg(args,char *);           \
  argv[i]=0;                                 \
  envp=va_arg(args,char **);                 \
  va_end(args)

static char *getmode(int m) {
  char *modes[]={"r","w","w+"};
  return modes[m&3];
}

#define OUTFILE(f,m)   output(__func__,f,m)

/************************************************************/

extern int creat(char *fname, mode_t mode) {
  OUTFILE(fname,"w");
  GETREAL(int,(char *fname, mode_t mode));
  return real(fname,mode);
}

extern int creat64(char *fname, mode_t mode) {
  OUTFILE(fname,"w");
  GETREAL(int,(char *fname, mode_t mode));
  return real(fname,mode);
}

extern int open(char *fname, int mode, ...) {
  OUTFILE(fname,getmode(mode));
  PERM;
  GETREAL(int,(char *fname, int mode, ...));
  return real(fname,mode,perm);
}

extern int open64(char *fname, int mode, ...) {
  OUTFILE(fname,getmode(mode));
  PERM;
  GETREAL(int,(char *fname, int mode, ...));
  return real(fname,mode,perm);
}

extern void *freopen(char *fname, char  *mode,  void *stream) {
  OUTFILE(fname,mode);
  GETREAL(void *,(char *filename, char *mode, void *stream));
  return real(fname,mode,stream);
}

extern void *freopen64(char *fname, char  *mode,  void *stream) {
  OUTFILE(fname,mode);
  GETREAL(void *,(char *filename, char *mode, void *stream));
  return real(fname,mode,stream);
}

extern void *fopen(char *fname, char *mode) {
  OUTFILE(fname,mode);
  GETREAL(void *,(char *fname, char *mode));
  return real(fname,mode);
}

extern void *fopen64(char *fname, char *mode) {
  OUTFILE(fname,mode);
  GETREAL(void *,(char *fname, char *mode));
  return real(fname,mode);
}

extern int unlink(char *fname) {
  OUTFILE(fname,"u");
  GETREAL(int,(char *fname));
  return real(fname);
}

extern int unlink64(char *fname) {
  OUTFILE(fname,"u");
  GETREAL(int,(char *fname));
  return real(fname);
}

extern int rename(char *oldpath, char *newpath) {
  OUTFILE(oldpath,"r");
  OUTFILE(newpath,"w");
  OUTFILE(oldpath,"u");
  GETREAL(int,(char *oldpath, char *newpath));
  return real(oldpath,newpath);
}

extern int execve(char *path, char **argv, char **envp) {
  OUTFILE(path,"x");
  GETREAL(int,(char *path, char **argv, char **envp));
  return real(path,argv,envp);
}

extern int execv(char *path, char **argv) {
  extern char **environ;
  return execve(path,argv,environ);
}

extern int execvp(char *file, char **argv) {
  return execv(path(file),argv);
}

extern int execl(char *path, char *arg, ...) {
  L2VE;
  return execv(path,argv);
}

extern int execle(char *path, char *arg, ...) {
  L2VE;
  return execve(path,argv,envp);
}

extern int execlp(char *file, char *arg, ...) {
  L2VE;
  return execvp(file,argv);
}
