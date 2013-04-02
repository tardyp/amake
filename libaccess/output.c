#define _GNU_SOURCE

#include "output.h"
#include "field.h"
#include "path.h"
#include "shlibs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>

static int inode(int fd) {
  struct stat buf;
  if (fstat(fd,&buf))
    ERROR("inode(): fstat(%u) failed PID %u",fd,getpid());
  return buf.st_ino;
}

static void pipesig(int signo) {
  static int fd;
  static sighandler_t old;
  switch (signo) {
    case SIGPIPE:
      ERROR("pipesig(): write(%d(%d)) SIGPIPE PID %d",fd,inode(fd),getpid());
    case 0:
      signal(SIGPIPE,old);
      break;
    default:
      fd=-signo;
      if ((old=signal(SIGPIPE,pipesig))==SIG_ERR)
	ERROR("pipesig(): signal(SIGPIPE) failed");
      break;
  }
}
 
static void wrchr(int fd, char c) {
  int n;
  while ((n=write(fd,&c,1))!=1)
    if (n<0)
      ERROR("wrchr(): write(%d(%d)) failed PID %d",fd,inode(fd),getpid());
}

static void wrstr(int fd, const char *s) {
  int l=strlen(s);
  while (l>0) {
    int n=write(fd,s,l);
    if (n<0)
      ERROR("wrshr(): write(%d(%d)) failed PID %d",fd,inode(fd),getpid());
    s+=n;
    l-=n;
  }
}

static void out(const char *call, char *file, char *mode, int fd, char *f) {
  if (!fd)
    return;
  for (; *f; f++) {
    switch (*f) {
      case '%':
	switch (*++f) {
          case 'c': wrstr(fd,call);  break;
          case 'f': wrstr(fd,file);  break;
          case 'm': wrstr(fd,mode);  break;
          case 'M': wrchr(fd,*mode); break;
          default:  wrchr(fd,*--f);  break;
	}
	break;
      case '\\':
	switch (*++f) {
	  case 'n': wrchr(fd,'\n'); break;
	  case 't': wrchr(fd,'\t'); break;
	  default:  wrchr(fd,*f); break;
	}
	break;
      default: wrchr(fd,*f); break;
    }
  }
}

static void P(int sd) {
  if (!sd)
    return;
  struct sembuf op; 
  op.sem_num=0;
  op.sem_op=-1;
  op.sem_flg=0;
  while (1) {
    if (!semop(sd,&op,1))
      return;
    if (errno!=EINTR)
      ERROR("P(): semop() failed");
  }
}

static void V(int sd) {
  if (!sd)
    return;
  struct sembuf op; 
  op.sem_num=0;
  op.sem_op=1;
  op.sem_flg=0;
  if (semop(sd,&op,1))
    ERROR("V(): semop() failed");
}

static int stoi(char *s) {
  int i=0;
  while (*s && *s>='0' && *s<='9')
    i=i*10+(*s++)-'0';
  if (*s)
    ERROR("stoi(): bad digit (%c)",*s);
  return i;
}

extern HIDDEN void output(const char *call, char *file, char *mode) {
  char *fds=getenv(ENV(FDS));
  if (!fds)
    return;
  file=absolute(file);
  char *format=getenv(ENV(FORMAT));
  if (!format)
    format="%f\n";
  DEBUG("in output(%s %s) %s=\"%s\" %s=\"%s\" PPID %d PID %d\n",
	call,file,ENV(FDS),fds,"LD_PRELOAD",getenv("LD_PRELOAD"),
	getppid(),getpid());
  char *fdsd;
  int i=0;
  while ((fdsd=field(i++,fds,' '))) {
    char *fd=field(0,fdsd,':');
    char *sd=field(1,fdsd,':');
    free(fdsd);
    int fdi=stoi(fd);
    int sdi=stoi(sd);
    free(fd);
    free(sd);
    P(sdi);
    DEBUG("in output(%s %s) P %d(%d):%d PID %d\n",call,file,fdi,inode(fdi),
	  sdi,getpid());
    pipesig(-fdi);
    out(call,file,mode,fdi,format);
    pipesig(0);
    DEBUG("in output(%s %s) V %d(%d):%d PID %d\n",call,file,fdi,inode(fdi),
	  sdi,getpid());
    V(sdi);
  }
  shlibs(call,file,mode);
  free(file);
}
