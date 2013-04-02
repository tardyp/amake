#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "getline.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

/***************************************************************************
 * This function returns the directory part of a path.  If the             *
 * path has no directory part "." is returned.  Its return                 *
 * value can be free()-ed.                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     s = path to analyze                                                 *
 ***************************************************************************/
extern char *dirname(const char *s) {
  char *p=strrchr(s,'/');
  if (!p)
    s="./";
  int len=(p ? p-s : 1);
  char *t=(char *)malloc(len+1);
  if (!t)
    ERR("dirname(): malloc() failed");
  memcpy(t,s,len);
  *(t+len)=0;
  return t;
}

/***************************************************************************
 * This function returns the nondirectory part of a path.  If              *
 * the path has no directory part the whole path is returned.              *
 * Its return value can be free()-ed.                                      *
 *                                                                         *
 * Arguments:                                                              *
 *     s = path to analyze                                                 *
 ***************************************************************************/
extern char *basname(const char *s) {
  const char *p=strrchr(s,'/');
  p=(p ? p+1 : s);
  char *t=(char *)malloc(strlen(p)+1);
  if (!t)
    ERR("basname(): malloc() failed");
  strcpy(t,p);
  return t;
}

/***************************************************************************
 * This function creates the directories and subdirectories                *
 * needed for a path to a directory to exist.                              *
 *                                                                         *
 * Arguments:                                                              *
 *     p = directory to create                                             *
 ***************************************************************************/
extern void mkdirp(char *p) {
  char *q=p;
  if (*p && *p=='/')
    p++;
  while (*p) {
    while (*p && *p!='/')
      p++;
    int replace=0;
    if (*p) {
      *p=0;
      replace=1;
    }
    if (mkdir(q,0777) && errno!=EEXIST)
      ERR("mkdirp(): mkdir(%s) failed PID %u",q,getpid());
    if (replace) {
      *p='/';
      p++;
    }
  }
}

/***************************************************************************
 * This function returns the inode number of an open file                  *
 * descriptor.                                                             *
 *                                                                         *
 * Arguments:                                                              *
 *     fd = file descriptor                                                *
 ***************************************************************************/
extern int inode(int fd) {
  struct stat buf;
  if (fstat(fd,&buf))
    ERR("inode(): fstat(%u) failed PID %u",fd,getpid());
  return buf.st_ino;
}

/***************************************************************************
 * This function closes all open file descriptors above, but               *
 * not including, a particular file descriptor.                            *
 *                                                                         *
 * Arguments:                                                              *
 *     fd = file descriptor                                                *
 ***************************************************************************/
extern void close_above(int fd) {
  char *fdd;
  if (asprintf(&fdd,"/proc/%u/fd",getpid())<0)
    ERR("close_above(): asprintf() failed");
  struct dirent **namelist;
  int selector(const struct dirent *d) { return 1; }
  int n=scandir(fdd,&namelist,selector,alphasort);
  if (n<0)
    ERR("close_above(): scandir(%s) failed PID %u",fdd,getpid());
  free(fdd);
  while(n--) {
    int fdi=atoi(namelist[n]->d_name);
    if (fdi>fd)
      close(fdi);
    free(namelist[n]);
  }
  free(namelist);
}

/***************************************************************************
 * This function returns whether a filename names a directory.             *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = filename                                                       *
 ***************************************************************************/
extern int isdir(const char *fn) {
  struct stat buf;
  return (stat(fn,&buf) ? 0 : S_ISDIR(buf.st_mode));
}

/***************************************************************************
 * This function returns whether a filename names a regular file.          *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = filename                                                       *
 ***************************************************************************/
extern int isreg(const char *fn) {
  struct stat buf;
  return (stat(fn,&buf) ? 0 : S_ISREG(buf.st_mode));
}

/***************************************************************************
 * This function returns whether a filename names a symbolic link.         *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = filename                                                       *
 ***************************************************************************/
extern int issym(const char *fn) {
  struct stat buf;
  return (lstat(fn,&buf) ? 0 : S_ISLNK(buf.st_mode));
}

/***************************************************************************
 * This function returns whether a filename names an empty file.           *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = filename                                                       *
 ***************************************************************************/
extern int isempty(const char *fn) {
  struct stat buf;
  return (stat(fn,&buf) ? 0 : buf.st_size==0);
}

/***************************************************************************
 * This function returns the time-of-last-modification (tolm)              *
 * of a file named by a filename, as a string. Its return value            *
 * can be free()-ed.                                                       *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = filename                                                       *
 ***************************************************************************/
extern char *tolmfile(const char *fn) {
  struct stat buf;
  if (stat(fn,&buf))
    return 0;
  char tolm[24]; // big enough for 64-bit unsigned integer
  sprintf(tolm,"%ld",buf.st_mtime);
  return vstrcat(tolm,NULL);
}

/***************************************************************************
 * This function returns a pathname. Its return value can be               *
 * free()-ed. For example, if:                                             *
 *                                                                         *
 *     name = a/b/c/foo.o                                                  *
 *     dir = x/y/z                                                         *
 *     ext = dep                                                           *
 *                                                                         *
 * then the returned pathname is:                                          *
 *                                                                         *
 *     a/b/c/x/y/z/foo.o.dep                                               *
 *                                                                         *
 * Arguments:                                                              *
 *     name = pathname (with filename)                                     *
 *     dir = sub pathname (without filename)                               *
 *     ext = extension to add to filename, as suffix                       *
 ***************************************************************************/
static char *mkfn(const char *name, const char *dir, const char *ext) {
  char *dn=dirname(name);
  char *bn=basname(name);
  char *fn=vstrcat(dn,"/",
		   (dir && *dir ? dir : "."),"/",
		   bn,".",
		   ext,NULL);
  free(dn);
  free(bn);
  return fn;
}

/***************************************************************************
 * This function returns a pathname to a .cmd file. Its return             *
 * value can be free()-ed.                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     name = pathname (with filename)                                     *
 *     dir = sub pathname (without filename)                               *
 *     ext = extension to add to filename, as suffix                       *
 ***************************************************************************/
extern char *mkcmdfn(const char *name, const char *dir, const char *ext) {
  return mkfn(name,dir,ext);
}

/***************************************************************************
 * This function returns a pathname to a .dep file. Its return             *
 * value can be free()-ed.                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     name = pathname (with filename)                                     *
 *     dir = sub pathname (without filename)                               *
 *     ext = extension to add to filename, as suffix                       *
 ***************************************************************************/
extern char *mkdepfn(const char *name, const char *dir, const char *ext) {
  return mkfn(name,dir,ext);
}

/***************************************************************************
 * This function returns a pathname to a .sib file. Its return             *
 * value can be free()-ed.                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     name = pathname (with filename)                                     *
 *     dir = sub pathname (without filename)                               *
 *     ext = extension to add to filename, as suffix                       *
 ***************************************************************************/
extern char *mksibfn(const char *name, const char *dir, const char *ext) {
  return mkfn(name,dir,ext);
}

/***************************************************************************
 * This function returns a pathname to a .old file. Its return             *
 * value can be free()-ed.                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = pathname (with filename)                                       *
 *     ext = extension to add to filename, as suffix                       *
 ***************************************************************************/
extern char *mkoldfn(const char *fn, const char *ext) {
  return vstrcat(fn,".",ext,NULL);
}

/***************************************************************************
 * This function returns whether two pathnames are on the same             *
 * filesystem.                                                             *
 *                                                                         *
 * Arguments:                                                              *
 *     src = pathname                                                      *
 *     dst = pathname                                                      *
 ***************************************************************************/
extern int samefs(const char *src, const char *dst) {
  struct stat s;
  struct stat d;
  if (stat(src,&s))
    ERR("in samefs(): stat(\"%s\") failed: %s",src,strerror(errno));
  if (stat(dst,&d))
    ERR("in samefs(): stat(\"%s\") failed: %s",dst,strerror(errno));
  return s.st_dev==d.st_dev;
}

/***************************************************************************
 * This function copies the content of one file to another. The            *
 * src file must exists. It returns whether it was successful.             *
 *                                                                         *
 * Arguments:                                                              *
 *     src = pathname                                                      *
 *     dst = pathname                                                      *
 ***************************************************************************/
extern int fcp(const char *src, const char *dst) {
  struct stat statbuf;
  if (stat(src,&statbuf))
    ERR("in fcp(): stat(\"%s\") failed: %s",src,strerror(errno));
  const int size=statbuf.st_blksize;
  char buf[size];
  int ifd=open(src,O_RDONLY);
  if (ifd<0)
    ERR("in fcp(): open(\"%s\") failed: %s",src,strerror(errno));
  int ofd=creat(dst,statbuf.st_mode);
  if (ofd<0)
    ERR("in fcp(): creat(\"%s\") failed: %s",dst,strerror(errno));
  int n;
  while ((n=read(ifd,buf,size))>0)
    if (write(ofd,buf,n)!=n)
      ERR("in fcp(): write(\"%s\") failed: %s",dst,strerror(errno));
  if (n<0)
    ERR("in fcp(): read(\"%s\") failed: %s",src,strerror(errno));
  close(ifd);
  close(ofd);
  return 1;
}

/***************************************************************************
 * This function returns the first line in the file named by a             *
 * filename. It opens and closes the file. It returns zero if              *
 * the file does not exist. Otherwise, its return value can be             *
 * free()-ed.                                                              *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = filename                                                       *
 ***************************************************************************/
extern char *freadfirstln(const char *fn) {
  FILE *f=fopen(fn,"r");
  if (!f)
    return 0;
  char *ln=GETLINE(f);
  fclose(f);
  return ln;
}

/***************************************************************************
 * This function sets the timestamps of a file, to be that of              *
 * another file.                                                           *
 *                                                                         *
 * Arguments:                                                              *
 *     src = path to source file                                           *
 *     dst = path to destination file                                      *
 ***************************************************************************/
static void fut(const char *src, const char *dst) {
  struct stat buf;
  if (stat(src,&buf))
    ERR("in fut() src=\"%s\" dst=\"%s\": stat() failed",src,dst);
  struct utimbuf times;
  times.actime=buf.st_atime;
  times.modtime=buf.st_mtime;
  if (utime(dst,&times))
    ERR("in fut() src=\"%s\" dst=\"%s\": utime() failed",src,dst);
}

/***************************************************************************
 * This function "copies" a file, perhaps as a hard or soft link.          *
 *                                                                         *
 * Arguments:                                                              *
 *     src = path to source file                                           *
 *     dst = path to destination file                                      *
 *     succ = pointer to success/failure value                             *
 ***************************************************************************/
static void fsymlink(const char *src, const char *dst) {
  // from the info page for readlink()
  char *readlink_malloc(const char *filename) {
    int size=100;
    char *buffer=NULL;
    while (1) {
      buffer=(char *)realloc(buffer, size);
      if (!buffer)
	ERR("in fsymlink(): realloc() failed: %s",strerror(errno));
      int nchars=readlink(filename,buffer,size);
      if (nchars<0) {
	free(buffer);
	return NULL;
      }
      if (nchars<size) {
	buffer[nchars]=0;
	return buffer;
      }
      size*=2;
    }
  }
  char *sl=readlink_malloc(src);
  if (!sl)
    ERR("in fsymlink(): readlink(%s) failed: %s",src,strerror(errno));
  if (symlink(sl,dst))
    ERR("in fsymlink(): symlink(%s,%s) failed: %s",src,dst,strerror(errno));
  free(sl);
}

/***************************************************************************
 * This function "copies" a file, perhaps as a hard or soft link.          *
 *                                                                         *
 * Arguments:                                                              *
 *     src = path to source file                                           *
 *     dst = path to destination file                                      *
 *     succ = pointer to success/failure value                             *
 ***************************************************************************/
extern void fmove(const char *src, const char *dst, int *succ) {
  if (isdir(src))
    ERR("in fmove() src=\"%s\" dst=\"%s\": src must not be a directory",src,dst);
  if (isdir(dst))
    ERR("in fmove() src=\"%s\" dst=\"%s\": dst must not be a directory",src,dst);
  if (!(issym(src) || isreg(src))) {
    if (succ)
      *succ=0;
    return;
  }
  if (isreg(dst) || issym(dst))
    unlink(dst);
  char *dstdir=dirname(dst);
  mkdirp(dstdir);
  if (issym(src)) {		/* symbolic link */
    fsymlink(src,dst);
  } else if (isreg(src)) {	/* regular file */
    if (samefs(src,dstdir)) {
      if (link(src,dst))
	ERR("in fmove(): link(%s,%s) failed: %s",src,dst,strerror(errno));
    } else {
      if (!fcp(src,dst))
	ERR("in fmove(): fcp(%s,%s) failed",src,dst);
      fut(src,dst);
    }
  }
  free(dstdir);
}
