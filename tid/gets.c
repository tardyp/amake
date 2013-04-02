#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

#include "gets.h"
#include "options.h"
#include "err.h"

#define STRSIZE PATH_MAX

/***************************************************************************
 * This function reads a sequence of (assumed nonzero)                     *
 * characters, of bounded length, from a file. The sequence is             *
 * terminated by a newline or EOF. The sequence is returned as             *
 * a zero-terminated string, without the newline. Its return               *
 * value can be free()-ed.                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     s = destination                                                     *
 *     f = file pointer                                                    *
 *     env = setjmp() environment                                          *
 *     eof = allow EOF without error                                       *
 *     file = source-file name (for errors)                                *
 *     line = source-file line number (for errors)                         *
 ***************************************************************************/
extern void _gets(char **s, FILE *f, jmp_buf env, int eof,
		  char *file, int line) {
  void onalarm(int signo) {
    longjmp(env,1);
  }
  signal(SIGPIPE,onalarm);
  signal(SIGALRM,onalarm);
  alarm(opt_timeout());
  char a[STRSIZE]; 
  char *r=fgets(a,STRSIZE,f);
  int err=errno;
  signal(SIGPIPE,SIG_DFL);
  signal(SIGALRM,SIG_IGN);
  alarm(0);
  *s=0;
  if (!r) {
    if (feof(f))
      if (eof)
        return;
      else
        ERRLOC(file,line,"gets(): unexpected EOF");
    else
      ERRLOC(file,line,"gets() failed: %s",strerror(err));
  }
  int n=strlen(a);
  if (n>0) {
    n--;
    if (a[n]!='\n')
      ERRLOC(file,line,"gets() failed: line too long");
    a[n]=0;
  }
  *s=strdup(a);
  if (!*s)
    ERRLOC(file,line,"gets(): strdup() failed");
}
