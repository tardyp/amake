#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "fpp.h"
#include "command.h"
#include "db.h"
#include "cache.h"
#include "options.h"
#include "getline.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

#define CMDNAME(name) cmd_##name
#define CMDDEFN(name) static char *CMDNAME(name) (FPP *fpp)
#define CMDENTRY(name) {#name,CMDNAME(name)}

/***************************************************************************
 * This function performs a "get" command. It reads parameters             *
 * from the client socket, performs database and cache accesses            *
 * (which may write target and sibling names to the socket),               *
 * logs the access, and writes an empty string to the socket to            *
 * indicate the end of the target and sibling names.  It                   *
 * returns a result string of "success" or "failure".                      *
 *                                                                         *
 * Arguments:                                                              *
 *     fpp = client-service socket                                         *
 ***************************************************************************/
CMDDEFN(get) {
  char *result="failure";
  char *cwd=GETLINE(fpp->r);
  char *name=GETLINE(fpp->r);
  char *dir=GETLINE(fpp->r);
  char *cext=GETLINE(fpp->r);
  char *dext=GETLINE(fpp->r);
  char *sext=GETLINE(fpp->r);
  char *arch=GETLINE(fpp->r);
  char *cmdsum=GETLINE(fpp->r);
  signal(SIGALRM,SIG_IGN);
  alarm(0);
  char **cachedirs=db_get(name,arch,cmdsum);
  if (*cachedirs && cache_get(fpp,cwd,cachedirs,name,dir,cext,dext,sext))
    result="success";
  char *date_yymmdd=str_date_yymmdd(); /* static! */
  char *time_hhmmss=str_time_hhmmss(); /* static! */
  db_log("get",date_yymmdd,time_hhmmss,name,cwd,result);
  free(cwd);
  free(name);
  free(dir);
  free(cext);
  free(dext);
  free(sext);
  free(arch);
  free(cmdsum);
  char **cachedir=cachedirs;
  while (*cachedir)
    free(*cachedir++);
  free(cachedirs);
  fprintf(fpp->w,"\n");		// end of names
  fflush(fpp->w);
  return result;
}

/***************************************************************************
 * This function performs a "put" command. It reads parameters             *
 * from the client socket, performs database and cache accesses            *
 * (which may write target and sibling names to the socket),               *
 * logs the access, and writes an empty string to the socket to            *
 * indicate the end of the target and sibling names.  It                   *
 * returns a result string of "success" or "failure".                      *
 *                                                                         *
 * Arguments:                                                              *
 *     fpp = client-service socket                                         *
 ***************************************************************************/
CMDDEFN(put) {
  char *result="failure";
  char *cwd=GETLINE(fpp->r);
  char *name=GETLINE(fpp->r);
  char *dir=GETLINE(fpp->r);
  char *cext=GETLINE(fpp->r);
  char *dext=GETLINE(fpp->r);
  char *sext=GETLINE(fpp->r);
  char *arch=GETLINE(fpp->r);
  char *chksum=GETLINE(fpp->r);
  char *cmdsum=GETLINE(fpp->r);
  char *date_yymmdd=str_date_yymmdd(); /* static! */
  char *time_hhmmss=str_time_hhmmss(); /* static! */
  char *cachedir=vstrcat(opt_cachedir(),"/",
   			 str_dup(str_user()),"/",
  			 str_host(),"/",
  			 date_yymmdd,"/",
  			 str_pid(),"/",
  			 time_hhmmss,NULL);
  signal(SIGALRM,SIG_IGN);
  alarm(0);
  int cached=db_put(date_yymmdd,time_hhmmss,name,arch,chksum,cmdsum,cachedir);
  if (cached && cache_put(fpp,cwd,cachedir,name,dir,cext,dext,sext))
    result="success";
  db_log("put",date_yymmdd,time_hhmmss,name,cwd,result);
  free(cwd);
  free(name);
  free(dir);
  free(cext);
  free(dext);
  free(sext);
  free(arch);
  free(chksum);
  free(cmdsum);
  free(cachedir);
  fprintf(fpp->w,"\n");		// end of names
  fflush(fpp->w);
  return result;
}

/***************************************************************************
 * This function performs a "command" to service a single                  *
 * socket-based client request. It searches the array of                   *
 * command entries. If it finds a match, it calls the function             *
 * pointer for that entry, passing along the socket's file                 *
 * descriptor. A called function returns a result string, which            *
 * is also returned by this function.                                      *
 *                                                                         *
 * Arguments:                                                              *
 *     fd = client-service socket                                          *
 *     command = string naming client's request (e.g., "get")              *
 ***************************************************************************/
extern char *command_do(FPP *fpp, char *command) {
  typedef struct {
    char *name;
    char *(*func)(FPP *fpp);
  } Command;
  static const Command commands[]={
    CMDENTRY(get),
    CMDENTRY(put),
    {0,0}
  };
  int i;
  for (i=0; commands[i].name; i++)
    if (!strcmp(command,commands[i].name))
      return commands[i].func(fpp);
  return "failure: unknown command";
}
