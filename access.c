#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h> // man svipc(7)
#include <sys/sem.h> // man svipc(7)

#include "defaults.h"
#include "make.h"
#include "dep.h"
#include "filedef.h"
#include "job.h"
#include "commands.h"
#include "variable.h"
#include "debug.h"
#include "access.h"
#include "cache.h"
#include "var.h"
#include "chksum.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

/***************************************************************************
 * This function returns a beautified makefile command.                    *
 *                                                                         *
 * Arguments:                                                              *
 *     s = makefile command                                                *
 ***************************************************************************/
static char *clean(char *s) {
  char *cmd=(char *)malloc(strlen(s)+1);
  if (!cmd)
    ERR("clean(): malloc() failed");
  char *t=cmd;
  int margin=1;
  int lineno=0;
  int skip0=0;
  int skip;
  while (*s) {
    switch (*s) {
      case ' ': case '\t':
	if (margin) {
	  if (lineno==0) {
	    s++;
	    skip0++;
	  } else {
	    if (skip<skip0) {
	      s++;
	      skip++;
	    } else {
	      *t++=*s++;
	    }
	  }
	} else {
	  *t++=*s++;
	}
	break;
      case '\n':
	margin=1;
	lineno++;
	skip=0;
	*t++=*s++;
	break;
      case '@': case '-': case '+':
	if (margin && lineno==0)
	  s++;
	else
	  *t++=*s++;
	break;
      default:
	margin=0;
	*t++=*s++;
	break;
    }
  }
  *t=0;
  return cmd;
}

/***************************************************************************
 * This function opens, reads, and closes a file containing a              *
 * makefile target's command checksum and commands. If the file            *
 * does not exist it returns zero. Otherwise, it returns                   *
 * the checksum, which, may be free()-ed.                                  *
 *                                                                         *
 * Arguments:                                                              *
 *     fn = name of command file                                           *
 ***************************************************************************/
static char *rdcmdfile(char *fn) {
  char *cmdsum=0;
  FILE *f=fopen(fn,"r");
  if (f) {
    fscanf(f,"#!/bin/bash cmdsum=%ms\n",&cmdsum);
    fclose(f);
  }
  return cmdsum;
}

/***************************************************************************
 * This function opens, writes, and closes a file containing a             *
 * makefile target's command checksum and commands.                        *
 *                                                                         *
 * Arguments:                                                              *
 *     cmds = commands to build target                                     *
 *     fn = name of command file                                           *
 *     file = makefile target                                              *
 ***************************************************************************/
static void wrcmdfile(char *cmds, char *fn, struct file *file) {
  DB(DB_JOBS,(_("In wrcmdfile(\"%s\") cmdsum=\"%s\".\n"),
	      fn,file->access_cmdsum));
  FILE *f=fopen(fn,"w");
  if (!f)
    ERR("wrcmdfile(): fopen(\"%s\") failed",fn);
  fprintf(f,"#!/bin/bash\n\n");
  fprintf(f,"cmdsum=%s\n\n",file->access_cmdsum);
  fprintf(f,"%s",cmds);
  fclose(f);
  chmod(fn,0755);
}

/***************************************************************************
 * This function returns a string containing a makefile                    *
 * target's commands.                                                      *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
static char *mkcmds(struct file *file) {
  char *cmds=vstrcat("# ",file->cmds->fileinfo.filenm,
		     ":",str_int(file->cmds->fileinfo.lineno),
		     "\n",NULL);
  char *vars=GETVAR(ENVVARS);
  char *var;
  int i=0;
  while ((var=field(i++,vars,' '))) {
    char *val=0;
    if ((val=getvar(var,val,file)))
      cmds=vstrcatf(cmds,"export ",var,"=",val,"\n",NULL);
    else
      cmds=vstrcatf(cmds,"unset ",var,"\n",NULL);
    free(var);
  }
  int cnt=0;
  for (i=0; i<file->cmds->ncommand_lines; i++) {
    char *cmd=clean(variable_expand_for_file(file->cmds->command_lines[i],
					     file));
    DB(DB_JOBS,(_("In mkcmds(\"%s\") cmd=\"%s\".\n"),file->name,cmd));
    if (strlen(cmd)>0) {
      cmds=vstrcatf(cmds,cmd,"\n",NULL);
      cnt++;
    }
    free(cmd);
  }
  if (cnt==0) {
    free(cmds);
    cmds=0;
  }
  return cmds;
}

/***************************************************************************
 * This function compares a dependency's checksum to that                  *
 * required by its target. The function returns true if the                *
 * target must be updated. If the target does not require a                *
 * checksum the mtime-based decision is returned.                          *
 *                                                                         *
 * Arguments:                                                              *
 *     must_make_mtime = the decision so far                               *
 *     tgt = makefile target                                               *
 *     dep = makefile dependency                                           *
 ***************************************************************************/
static int access_chksum(int must_make_mtime,
			 struct file *tgt,
			 struct file *dep) {
  char *var=vstrcat(ENV(SUM),"_",dep->name,NULL);
  char *req=getvar(var,0,tgt);
  free(var);
  char *has=dep->access_sum;
  if (!has)
    has=chksumfile(dep->name);
  dep->access_sum=has;
  DB(DB_JOBS,(_("In access_chksum() req chksum=\"%s\".\n"),req));
  DB(DB_JOBS,(_("In access_chksum() has chksum=\"%s\".\n"),has));
  return (req ? (!has || strcmp(req,has)) : must_make_mtime);
}

/***************************************************************************
 * This function compares a dependency's                                   *
 * time-of-last-modification (LMTIME) to that required by its              *
 * target. Times are compared for equality, only. The function             *
 * returns true if the target must be updated. If the target               *
 * does not require a LMTIME the usual mtime-based decision is             *
 * returned.                                                               *
 *                                                                         *
 * Arguments:                                                              *
 *     must_make_mtime = the decision so far                               *
 *     tgt = makefile target                                               *
 *     dep = makefile dependency                                           *
 ***************************************************************************/
static int access_lmtime(int must_make_mtime,
			 struct file *tgt,
			 struct file *dep) {
  char *var=vstrcat(ENV(TOM),"_",dep->name,NULL);
  char *req=getvar(var,0,tgt);
  free(var);
  char *has=dep->access_tom;
  if (!has)
    has=tolmfile(dep->name);
  dep->access_tom=has;
  DB(DB_JOBS,(_("In access_time() req tom=\"%s\".\n"),req));
  DB(DB_JOBS,(_("In access_time() has tom=\"%s\".\n"),has));
  return (req ? (!has || strcmp(req,has)) : must_make_mtime);
}

/***************************************************************************
 * This function returns true if the target must be                        *
 * updated. The function returns false if the dependency is a              *
 * directory.                                                              *
 *                                                                         *
 * Arguments:                                                              *
 *     must_make_mtime = the decision so far                               *
 *     dep = makefile dependency                                           *
 ***************************************************************************/
extern int access_mustmake(int must_make_mtime, struct file *dep) {
  struct file *tgt=dep->parent;
  DB(DB_JOBS,(_("In access_mustmake() tgt=\"%s\" dep=\"%s\".\n"),
	      tgt->name,dep->name));
  int lmtime=chkvar(ENV(LMTIME),tgt);
  int chksum=chkvar(ENV(CHKSUM),tgt);
  if (!lmtime && !chksum)
    return must_make_mtime;
  if (isdir(dep->name))
    return 0;
  if (lmtime) {
    int mustmake=access_lmtime(must_make_mtime,tgt,dep);
    if (mustmake)
      return mustmake;
  }
  if (chksum) {
    int mustmake=access_chksum(must_make_mtime,tgt,dep);
    if (mustmake)
      return mustmake;
  }
  return 0;
}

/***************************************************************************
 * This function creates a file containing a script that                   *
 * executes a makefile target's commands. It indicates to the              *
 * caller whether or not the target needs to be updated due to             *
 * the commands being different than last time the target was              *
 * updated. The command file is only modified if its content is            *
 * different.                                                              *
 *                                                                         *
 * The case of a single empty command is tricky. The Linux                 *
 * kernel uses several Makefile rules like this:                           *
 *                                                                         *
 *     foo: bar zap ;                                                      *
 *                                                                         *
 * where bar and zap are directories. A single empty command is            *
 * different than no commands: it prevents a pattern rule from             *
 * applying. In this case, the target does not need to be                  *
 * updated, since the empty command cannot create or change                *
 * it. Furthermore, an empty .cmd file is not created, since it            *
 * would overwrite the non-empty .cmd file that previously                 *
 * created the target.                                                     *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
extern void access_commands(struct file *file, int *must_make) {
  if (!file || file->phony || !file->cmds || !file->cmds->commands)
    return;
  DB(DB_JOBS,(_("In access_commands(\"%s\").\n"),file->name));
  initialize_file_variables(file,0);
  set_file_variables(file);
  chop_commands(file->cmds);
  DB(DB_JOBS,(_("In access_commands(\"%s\") makefile=\"%s\" line=%d.\n"),
	      file->name,
	      file->cmds->fileinfo.filenm,
	      file->cmds->fileinfo.lineno));
  if (!chkvar(ENV(ENABLE),file) || chkvar(ENV(ANYCMD),file))
    return;
  char *cmdfn=mkcmdfn(file->name,GETVAR(DIR),GETVAR(CEXT));
  DB(DB_JOBS,(_("In access_commands() cmdfn=\"%s\".\n"),cmdfn));
  char *cmds=mkcmds(file);
  if (!cmds) {			       /* empty? */
    DB(DB_JOBS,(_("In access_commands() empty %s.\n"),cmdfn));
    *must_make=0;
  } else {
    char *req=rdcmdfile(cmdfn);
    char *has=chksum(cmds);
    DB(DB_JOBS,(_("In access_commands() req=%s has=%s\n"),req,has));
    file->access_cmdsum=has;
    if (!req || strcmp(req,has)) {     /* different? */
      char *dir=dirname(cmdfn);
      mkdirp(dir);
      free(dir);
      if (req) {
	char *oldfn=mkoldfn(cmdfn,GETVAR(OEXT));
	rename(cmdfn,oldfn);
	free(oldfn);
	free(req);
      }
      wrcmdfile(cmds,cmdfn,file);
      DB(DB_JOBS,(_("In access_commands() %s different.\n"),cmdfn));
      *must_make=1;
    } else {
      DB(DB_JOBS,(_("In access_commands() %s same.\n"),cmdfn));
    }
    free(cmds);
  }
  free(cmdfn);
}

/***************************************************************************
 * This function is called when a target's commands are about              *
 * to be executed. It checks the cache for a suitable                      *
 * target. It returns whether one was found.                               *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
extern int access_cache(struct file *file) {
  if (!chkvar(ENV(ENABLE),file))
    return 0;			/* miss */
  if (cache_get(file)) {
    file->update_status=0;
    file->command_state=cs_finished;
    return 1;			/* hit */
  }
  return 0;			/* miss */
}

/***************************************************************************
 * This function is called when a target is about to be                    *
 * updated. It creates a pipe and a semaphore for communication            *
 * with the collect process. It then fork and execs the collect            *
 * process.                                                                *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
extern void access_open(struct file *file) {
  if (!chkvar(ENV(ENABLE),file))
    return;
  if (file->phony)
    return;
  DB(DB_JOBS,(_("In access_open(\"%s\").\n"),file->name));
  if (file->access_pd)
    ERR("access_open(): already has access file descriptor");
  int pd[2];
  if (pipe(pd))
    ERR("access_open(): pipe() failed");
  DB(DB_JOBS,(_("In access_open(\"%s\")"
		" creating pipe %u(%u)/%u(%u) PID %u.\n"),
	      file->name,pd[0],inode(pd[0]),pd[1],inode(pd[1]),getpid()));
  int sd=semget(IPC_PRIVATE,1,0600);
  if (sd==-1)
    ERR("access_open(): semget() failed");
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
  } sv={1};
  semctl(sd,0,SETVAL,sv);
  file->access_sd=sd;
  file->access_spid=getpid();
  DB(DB_JOBS,(_("In access_open(\"%s\")"
		" %s=\"%s\" creating semaphore %u PID %u.\n"),
	      file->name,ENV(FDS),get_env(0,ENV(FDS)),
	      file->access_sd,file->access_spid));
  if ((file->access_ppid=fork())) { // parent
    close(pd[0]);
    file->access_pd=pd[1];
  } else {			    // child    
    dup2(pd[0],0);
    close(pd[0]);
    close(pd[1]);
    close_above(fileno(stderr));
    add_env(0,ENV(DIR),GETVAR(DIR),0);
    add_env(0,ENV(CEXT),GETVAR(CEXT),0);
    add_env(0,ENV(DEXT),GETVAR(DEXT),0);
    add_env(0,ENV(SEXT),GETVAR(SEXT),0);
    add_env(0,ENV(OEXT),GETVAR(OEXT),0);
    add_env(0,ENV(PASS),GETVAR(PASS),0);
    add_env(0,ENV(FAIL),GETVAR(FAIL),0);
    add_env(0,ENV(CHKSUM),GETVAR(CHKSUM),0);
    add_env(0,ENV(LMTIME),GETVAR(LMTIME),0);
    add_env(0,ENV(NAME),file->name,0);
    add_env(0,ENV(TIME),str_time(),0);
    char *val;
    asprintf(&val,"%u",file->access_sd);
    add_env(0,ENV(SEMID),val,0);
    rem_env(0,ENV(FDS));
    rem_env(0,"LD_PRELOAD");
    char *prog=GETVAR(PROGRAM);
    DB(DB_JOBS,(_("In access_open(\"%s\") executing %s fd 0(%u) PID %u.\n"),
		file->name,prog,inode(0),getpid()));
    execlp(prog,prog,0);
    ERR("access_open(): execlp(%s): %s",prog,strerror(errno));
  }
}

/***************************************************************************
 * This function is called when a command for a target is about            *
 * to be executed. It adjusts the child's environment to allow             *
 * shared-library calls to be intercepted and to allow                     *
 * communication with the collect process.                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     child = the target's command structure                              *
 ***************************************************************************/
extern void access_child(struct child *child) {
  struct file *file=child->file;
  if (!chkvar(ENV(ENABLE),file))
    return;
  if (file->phony)
    return;
  if (!file->access_pd)
    ERR("access_child(): no access pipe descriptor");
  char **env=child->environment;
  char *val;
  asprintf(&val,"%u:%u",file->access_pd,file->access_sd);
  DB(DB_JOBS,(_("In access_child(\"%s\") %s=\"%s\" %u(%u) PID %u.\n"),
	      file->name,ENV(FDS),val,
	      file->access_pd,inode(file->access_pd),getpid()));
  add_env(&env,ENV(FDS),val," ");
  val=GETVAR(SHLIBS);
  if (val && *val)
    add_env(&env,ENV(SHLIBS),val,0);
  val=GETVAR(DEBUG);
  if (val && *val)
    add_env(&env,ENV(DEBUG),val,0);
  val=GETVAR(LIBRARY);
  add_env(&env,"LD_PRELOAD",val,0);
  put_env(&env,ENV(FORMAT),"%M %f\n");
  child->environment=env;
}

/***************************************************************************
 * This function is called after a target has been updated. It             *
 * removes the pipe and semaphore that was used to communicate             *
 * with the collect process. It then waits for the collect                 *
 * process to exit.                                                        *
 *                                                                         *
 * Arguments:                                                              *
 *     file = makefile target                                              *
 ***************************************************************************/
extern void access_close(struct file *file) {
  if (!chkvar(ENV(ENABLE),file))
    return;
  if (file->phony)
    return;
  DB(DB_JOBS,(_("In access_close(\"%s\").\n"),file->name));
  if (file->access_spid==getpid()) {
    DB(DB_JOBS,(_("In access_close(\"%s\") close(%u(%u)) PID %u.\n"),
		file->name,file->access_pd,inode(file->access_pd),
		file->access_spid));
    close(file->access_pd);
    file->access_pd=0;
    DB(DB_JOBS,(_("In access_close(\"%s\") removing semaphore %u PID %u.\n"),
		file->name,file->access_sd,file->access_spid));
    semctl(file->access_sd,0,IPC_RMID);
    DB(DB_JOBS,(_("In access_close(\"%s\") PID %u waitpid(%u).\n"),
		file->name,file->access_spid,file->access_ppid));
    waitpid(file->access_ppid,0,0);
    DB(DB_JOBS,(_("In access_close(\"%s\") PID %u done waitpid(%u).\n"),
		file->name,file->access_spid,file->access_ppid));
    cache_put(file);
  }
}
