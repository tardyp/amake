#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "defaults.h"
#include "fpp.h"
#include "make.h"
#include "filedef.h"
#include "debug.h"
#include "cache.h"
#include "var.h"
#include "chksum.h"
#include "getline.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

/***************************************************************************
 * This function forks and executes the target-cache daemon                *
 * (tcd), which will be kill()-ed on exit().                               *
 *                                                                         *
 * Arguments:                                                              *
 *     tcd = name of the tcd                                               *
 *     sfn = the filename of the socket used for                           *
 *           communication with the tcd                                    *
 *     idle = the number of seconds the tcd should                         *
 *          idle before exitting                                           *
 ***************************************************************************/
static void tcd_exec(char *tcd, char *sfn,
		     char *cachedir, char *dbmgr, char *idle) {
  void tcd_kill(int status, void *pid) {
    kill((long)pid,SIGALRM);
  }
  DB(DB_JOBS,(_("In tcd_exec(): execvp(%s).\n"),tcd));
  long pid=fork();
  if (pid) {			/* parent */
    if (on_exit(tcd_kill,(void *)pid))
      ERR("tcd_exec(): on_exit() failed");
    return;
  }                             /* child */
  DB(DB_JOBS,(_("In tcd_exec(): child pid: %s\n"),str_pid()));
  close_above(fileno(stderr));
  rem_env(0,ENV(FDS));
  rem_env(0,"LD_PRELOAD");
  char *argv[]={tcd,
		"--daemon","true",
		"--socket",sfn,
		"--cachedir",cachedir,
		"--dbmgr",dbmgr,
		"--idle",idle,
		"--debug",(db_level ? "true" : "false"),
		0};
  execvp(tcd,&argv[0]);
  ERR("tcd_exec(): execvp() failed");
}

/***************************************************************************
 * This function opens a socket to the target-cache daemon                 *
 * (tcd).It returns a file pointer for the socket, or zero                 *
 * for failure.                                                            *
 *                                                                         *
 * Arguments:                                                              *
 *     sfn = socket filename                                               *
 ***************************************************************************/
static FPP *tcd_open(char *sfn) {
  int fd=socket(PF_LOCAL,SOCK_STREAM,0);
  if (fd<0)
    ERR("tcd_open(): socket() failed");
  struct sockaddr_un addr;
  addr.sun_family=AF_LOCAL;
  strncpy(addr.sun_path,sfn,sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path)-1]=0;
  if (connect(fd,(struct sockaddr *)&addr,SUN_LEN(&addr))<0) {
    close(fd);
    return 0;
  }
  return fpp_open(fd);
}

/***************************************************************************
 * This function opens a socket to the target-cache daemon                 *
 * (tcd), executing it if needed. The function only tries to               *
 * execute the tcd once, regardless of how many times the                  *
 * function is called.                                                     *
 *                                                                         *
 * It returns a file pointer for the socket, or zero for                   *
 * failure.                                                                *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
static FPP *tcd_start(struct file *file) {
  char *tcd=GETVAR(CACHE_DAEMON);
  char *tmp=GETVAR(TMP);
  char *sfn=vstrcat(tmp,"/",tcd,".",str_pid(),NULL);
  DB(DB_JOBS,(_("In tcd_start(\"%s\") tcd=\"%s\" sfn=\"%s\".\n"),
	      file->name,tcd,sfn));
  FPP *fpp=tcd_open(sfn);
  static int failed=0;
  if (!fpp && !failed) {
    unlink(sfn);
    char *cachedir=GETVAR(CACHE_DIR);
    if (cachedir && *cachedir)
      cachedir=vstrcat(cachedir,NULL);
    else
      cachedir=vstrcat(tmp,"/",tcd,NULL);
    char *dbmgr=GETVAR(CACHE_DBMGR);
    char *idle=GETVAR(CACHE_TIMEOUT);
    tcd_exec(tcd,sfn,cachedir,dbmgr,idle);
    free(cachedir);
    int tries=atoi(GETVAR(CACHE_TRIES));
    int try;
    for (try=0; try<tries; try++) {
      DB(DB_JOBS,(_("In tcd_start(\"%s\") tcd=\"%s\" sfn=\"%s\" try=%d.\n"),
		  file->name,tcd,sfn,try));
      if ((fpp=tcd_open(sfn)))
	break;
      sleep(1<<try);
    }
  }
  if (!fpp)
    failed=1;
  DB(DB_JOBS,(_("In tcd_start(\"%s\") tcd=\"%s\" sfn=\"%s\" %s.\n"),
	      file->name,tcd,sfn,(failed ? "failed" : "succeeded")));
  free(sfn);
  return fpp;
}

/***************************************************************************
 * This function checks if the cache contains a suitable                   *
 * target. It starts the target-cache daemon (tcd), if needed.             *
 * It returns success or failure.                                          *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
extern int cache_get(struct file *file) {
  if (!chkvar(ENV(CACHE),file))
    return 0;			/* miss: fail */
  if (!file->access_cmdsum)
    return 0;			/* miss: fail */
  if (file->phony)
    return 0;			/* miss: fail */
  DB(DB_JOBS,(_("In cache_get(\"%s\").\n"),file->name));
  FPP *fpp=tcd_start(file);
  if (!fpp)
    return 0;			/* miss: fail*/
  char *cwd=getcwd(0,0);
  if (!cwd)
    ERR("cache_get(): getcwd() failed");
  fprintf(fpp->w,"get\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
	  cwd,
	  file->name,
	  GETVAR(DIR),
	  GETVAR(CEXT),
	  GETVAR(DEXT),
	  GETVAR(SEXT),
	  GETVAR(ARCH),
	  file->access_cmdsum);
  fflush(fpp->w);
  free(cwd);
  int done=0;
  while (!done) {
    char *name=GETLINE(fpp->r);
    if (!*name)
      done=1;
    else
      printf("Cache get: %s\n",name);
    free(name);
  }
  fflush(stdout);
  char *result=GETLINE(fpp->r);
  int ret=!strcmp("success",result);
  free(result);
  fpp_close(fpp);
  char *cmdfn=mkcmdfn(file->name,GETVAR(DIR),GETVAR(CEXT));
  char *oldfn=mkoldfn(cmdfn,GETVAR(OEXT));
  unlink(oldfn);
  free(oldfn);
  free(cmdfn);
  return ret; 			/* hit: success */
}

/***************************************************************************
 * This function adds a target, and possibly siblings, to the              *
 * cache. It starts the target-cache daemon (tcd), if                      *
 * needed. It returns success or failure.                                  *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
extern int cache_put(struct file *file) {
  if (!chkvar(ENV(CACHE),file))
    return 0;			/* fail */
  file->access_sum=chksumfile(file->name);
  if (!file->access_sum || !file->access_cmdsum)
    return 0;			/* fail */
  if (file->phony)
    return 0;			/* fail */
  DB(DB_JOBS,(_("In cache_put(\"%s\") chksum=\"%s\" cmdsum=\"%s\".\n"),
	      file->name,file->access_sum,file->access_cmdsum));
  FPP *fpp=tcd_start(file);
  if (!fpp)
    return 0;			/* fail */
  char *cwd=getcwd(0,0);
  if (!cwd)
    ERR("cache_put(): getcwd() failed");
  fprintf(fpp->w,"put\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
	  cwd,
	  file->name,
	  GETVAR(DIR),
	  GETVAR(CEXT),
	  GETVAR(DEXT),
	  GETVAR(SEXT),
	  GETVAR(ARCH),
	  file->access_sum,
	  file->access_cmdsum);
  fflush(fpp->w);
  free(cwd);
  int done=0;
  while (!done) {
    char *name=GETLINE(fpp->r);
    if (!*name)
      done=1;
    else
      printf("Cache put: %s\n",name);
    free(name);
  }
  fflush(stdout);
  char *result=GETLINE(fpp->r);
  int ret=!strcmp("success",result);
  free(result);
  fpp_close(fpp);
  return ret; 			/* success */
}
