#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "getline.h"

/***************************************************************************
 * This function reads a sequence of (assumed nonzero)                     *
 * characters, of unbounded length, from a file. The sequence              *
 * is terminated by a newline or EOF. The sequence is returned             *
 * as a zero-terminated string, without the newline. Its return            *
 * value can be free()-ed.                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     f = file pointer                                                    *
 *     eof = allow EOF without error                                       *
 *     file = source-file name (for errors)                                *
 *     line = source-file line number (for errors)                         *
 ***************************************************************************/
extern char *_getline(FILE *f, int eof, char *file, int line) {
  char *s=0;
  size_t z=0;
  ssize_t n=getline(&s,&z,f);
  int err=errno;
  if (n<0) {
    if (feof(f))
      if (eof)
        return 0;
      else
        fprintf(stderr,"%s:%d: getline(): unexpected EOF\n",file,line);
    else
      fprintf(stderr,"%s:%d: getline() failed: %s\n",file,line,strerror(err));
    fflush(stderr);
    exit(1);
  }
  if (n>0)
    s[--n]=0;
  return s;
}
