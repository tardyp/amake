#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "fpp.h"
#include "cache.h"
#include "options.h"
#include "chksum.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

/***************************************************************************
 * This function "copies" target-related files to the cache. It            *
 * can write the names of files to the client's socket, so the             *
 * client can write it to the screen.                                      *
 *                                                                         *
 * Arguments:                                                              *
 *     fpp = client-service socket                                         *
 *     src = directory of target in cache                                  *
 *     name = target name, relative to cwd                                 *
 *     tgtsrc = target in cache                                            *
 *     cmdfn = .cmd-file name                                              *
 *     depfn = .dep-file name                                              *
 *     sibfn = .sib-file name                                              *
 *     cmdsrc = .cmd file in cache                                         *
 *     depsrc = .dep file in cache                                         *
 *     sibsrc = .sib file in cache                                         *
 ***************************************************************************/
static int cache_get_cp(FPP *fpp, char *src, char *name, char *tgtsrc,
			char *cmdfn, char *depfn, char *sibfn,
			char *cmdsrc, char *depsrc, char *sibsrc) {
  DEBUG("in cache_get_cp(\"%s\",\"%s\")",name,tgtsrc);
  int succ=1;
  fprintf(fpp->w,"%s\n",name);
  fmove(tgtsrc,name,&succ);
  fmove(cmdsrc,cmdfn,&succ);
  fmove(depsrc,depfn,0);
  fmove(sibsrc,sibfn,0);
  if (isreg(sibsrc)) {
    FILE *sibf=fopen(sibsrc,"r");
    if (!sibf)
      ERR("in cache_get_cp(): fopen(%s) failed: %s",sibsrc,strerror(errno));
    int done=0;
    while (!done) {
      char *sib=0;
      int n=fscanf(sibf,"%ms\n",&sib);
      if (n<0) {
	done=1;
      }
      if (n==1) {
	char *sibsrc=vstrcat(src,"/",sib,NULL);
	fprintf(fpp->w,"%s\n",sib);
	fmove(sibsrc,sib,&succ);
	free(sib);
	free(sibsrc);
      }
    }
  }
  return succ;
}

/***************************************************************************
 * This function "copies" target-related files from the cache. It          *
 * can write the names of files to the client's socket, so the             *
 * client can write it to the screen.                                      *
 *                                                                         *
 * Arguments:                                                              *
 *     fpp = client-service socket                                         *
 *     dst = directory of target in cache                                  *
 *     name = target name, relative to cwd                                 *
 *     tgtdst = target in cache                                            *
 *     cmdfn = .cmd-file name                                              *
 *     depfn = .dep-file name                                              *
 *     sibfn = .sib-file name                                              *
 *     cmddst = .cmd file in cache                                         *
 *     depdst = .dep file in cache                                         *
 *     sibdst = .sib file in cache                                         *
 ***************************************************************************/
static int cache_put_cp(FPP *fpp, char *dst, char *name, char *tgtdst,
			char *cmdfn, char *depfn, char *sibfn,
			char *cmddst, char *depdst, char *sibdst) {
  DEBUG("in cache_put_cp(\"%s\",\"%s\")",name,tgtdst);
  int succ=1;
  fprintf(fpp->w,"%s\n",name);
  fmove(name,tgtdst,&succ);
  fmove(cmdfn,cmddst,&succ);
  fmove(depfn,depdst,0);
  fmove(sibfn,sibdst,0);
  if (isreg(sibfn)) {
    FILE *sibf=fopen(sibfn,"r");
    if (!sibf)
      ERR("in cache_put_cp(): fopen(%s) failed: %s",sibfn,strerror(errno));
    int done=0;
    while (!done) {
      char *sib=0;
      int n=fscanf(sibf,"%ms\n",&sib);
      if (n<0) {
	done=1;
      }
      if (n==1) {
	char *sibdst=vstrcat(dst,"/",sib,NULL);
	fprintf(fpp->w,"%s\n",sib);
	fmove(sib,sibdst,&succ);
	free(sib);
	free(sibdst);
      }
    }
  }
  return succ;
}

/***************************************************************************
 * This function tokenizes a generated variable assignment,                *
 * from a target's .dep file.                                              *
 *                                                                         *
 * Arguments:                                                              *
 *     var = assignment                                                    *
 *         (e.g., "ACCESS_SUM_wc:=6c...49")                                *
 *     pre = pointer to variable prefix                                    *
 *         (e.g., "SUM")                                                   *
 *     dep = pointer to dependency name                                    *
 *         (e.g., "wc")                                                    *
 *     req = pointer to the value required for the dependency              *
 *         (e.g., "6c...49")                                               *
 ***************************************************************************/
static int scanvar(char *var, char **pre, char **dep, char **req) {
  *pre=str_dup(var);
  *dep=str_dup(var);
  *req=str_dup(var);
  return
    str_rm(*pre,"ACCESS_") &&
    str_rm_and_after(*pre,"_") &&
    str_rm(*dep,"ACCESS_") &&
    str_rm_and_before(*dep,"_") &&
    str_rm_and_after(*dep,":=") &&
    str_rm_and_before(*req,":=");
}

/***************************************************************************
 * This function helps determine whether a cache entry is                  *
 * acceptable, by returning the checksum of a dependency, so it            *
 * can be compared to that in the target's .dep file                       *
 *                                                                         *
 * Arguments:                                                              *
 *     dep = path to dependency                                            *
 ***************************************************************************/
static char *acceptable_sum(char *dep) {
  return chksumfile(dep); /* should I try to cache this? */
}

/***************************************************************************
 * This function helps determine whether a cache entry is                  *
 * acceptable, by returning the time-of-last-modification of a             *
 * dependency, so it can be compared to that in the target's               *
 * .dep file                                                               *
 *                                                                         *
 * Arguments:                                                              *
 *     dep = path to dependency                                            *
 ***************************************************************************/
static char *acceptable_tom(char *dep) {
  return tolmfile(dep);
}

/***************************************************************************
 * This function determines whether a cache entry is                       *
 * acceptable, according to the commands that created it and               *
 * the files on which it depends.                                          *
 *                                                                         *
 * Arguments:                                                              *
 *     tgtsrc = target in target cache                                     *
 *     depsrc = .dep file in target cache                                  *
 ***************************************************************************/
static int acceptable(char *tgtsrc, char *depsrc) {
  void nzfree(void *p) { if (p) free(p); }
  if (!(isreg(tgtsrc) || issym(tgtsrc)))
    return 0;			/* unacceptable */
  if (!isreg(depsrc))
    return 1;			/* acceptable */
  FILE *depf=fopen(depsrc,"r");
  if (!depf)
    ERR("in acceptable(): fopen() failed: %s",strerror(errno));
  int ret=0;			/* assume not acceptable */
  int done=0;
  while (!done) {
    char *tgt=0;		/* e.g., "foo:" */
    char *var=0;		/* e.g., "ACCESS_SUM_wc:=6c...49" */
    int n=fscanf(depf,"%ms %ms\n",&tgt,&var);
    if (n<0) {
      ret=1;			/* acceptable */
      done=1;
    }
    if (n==2) {
      char *pre=0;		 /* e.g., "SUM" */
      char *dep=0;		 /* e.g., "wc" */
      char *req=0;		 /* e.g., "6c...49" */
      char *has=0;		 /* e.g., "6c...49" */
      if (scanvar(var,&pre,&dep,&req)) {
	if (!strcmp(pre,"SUM")) has=acceptable_sum(dep);
	if (!strcmp(pre,"TOM")) has=acceptable_tom(dep);
	if (has && strcmp(req,has))
	  done=1;		/* unacceptable */
	DEBUG("in acceptable(\"%s\",\"%s\")",tgtsrc,depsrc);
	DEBUG("    pre=\"%s\"",pre);
	DEBUG("    dep=\"%s\"",dep);
	DEBUG("    req=\"%s\"",req);
	DEBUG("    has=\"%s\"",has);
	DEBUG("    result: %s",(done ? "unacceptable" : "acceptable"));
      }
      nzfree(pre);
      nzfree(dep);
      nzfree(req);
      nzfree(has);
    }
    nzfree(tgt);
    nzfree(var);
  }
  fclose(depf);
  return ret;
}

/***************************************************************************
 * This function performs the cache access for a "get"                     *
 * command. For each cache directory, it checks to see if the              *
 * content is acceptable. If so, it copies the content to the              *
 * destination directory. It returns whether an acceptable                 *
 * cache directory was found.                                              *
 *                                                                         *
 * Arguments:                                                              *
 *     fpp = client-service socket                                         *
 *     cwd = directory for which name is relative                          *
 *     cachedirs = zero-terminated array of paths in target cache          *
 *     name = target name, relative to cwd                                 *
 *     dir = .dep-file directory                                           *
 *     cext = .cmd-file extension                                          *
 *     dext = .dep-file extension                                          *
 *     sext = .sib-file extension                                          *
 ***************************************************************************/
extern int cache_get(FPP *fpp, char *cwd, char **cachedirs,
		     char *name, char *dir,
		     char *cext, char *dext, char *sext) {
  DEBUG("in cache_get(\"%s\",\"%s\")",cwd,name);
  if (chdir(cwd))
    ERR("cache_get(): chdir() failed");
  char **cdp;
  int succ=0;
  for (cdp=cachedirs; *cdp && !succ; cdp++) {
    char *bn=basname(name);
    char *src=vstrcat(*cdp,"/",bn,NULL);     // /tmp/.../foo
    char *tgtsrc=vstrcat(src,"/",name,NULL); // /tmp/.../foo/x/y/z/foo
    char *cmdfn=mkcmdfn(name,dir,cext);	     // x/y/z/foo.cmd
    char *depfn=mkdepfn(name,dir,dext);
    char *sibfn=mksibfn(name,dir,sext);
    char *cmdsrc=vstrcat(src,"/",cmdfn,NULL); // /tmp/.../foo/x/y/z/foo.cmd
    char *depsrc=vstrcat(src,"/",depfn,NULL);
    char *sibsrc=vstrcat(src,"/",sibfn,NULL);
    if (acceptable(tgtsrc,depsrc)) {
      succ=cache_get_cp(fpp,src,name,tgtsrc,
			cmdfn,depfn,sibfn,
			cmdsrc,depsrc,sibsrc);
      free(bn);
      free(src);
      free(tgtsrc);
      free(cmdfn);
      free(depfn);
      free(sibfn);
      free(cmdsrc);
      free(depsrc);
      free(sibsrc);
    }
  }
  return succ;
}

/***************************************************************************
 * This function performs the cache access for a "put"                     *
 * command. It creates a cache directory and copies to it the              *
 * content to the source directory. It returns whether it was              *
 * successful.                                                             *
 *                                                                         *
 * Arguments:                                                              *
 *     fpp = client-service socket                                         *
 *     cwd = directory for which name is relative                          *
 *     cachedir = path in target cache                                     *
 *     name = target name, relative to cwd                                 *
 *     dir = .dep-file directory                                           *
 *     cext = .cmd-file extension                                          *
 *     dext = .dep-file extension                                          *
 *     sext = .sib-file extension                                          *
 ***************************************************************************/
extern int cache_put(FPP *fpp, char *cwd, char *cachedir,
		     char *name, char *dir,
		     char *cext, char *dext, char *sext) {
  DEBUG("in cache_put(\"%s\",\"%s\")",cwd,name);
  if (chdir(cwd))
    ERR("cache_put(): chdir() failed");
  char *bn=basname(name);
  char *dst=vstrcat(cachedir,"/",bn,NULL); // /tmp/.../foo
  char *tgtdst=vstrcat(dst,"/",name,NULL); // /tmp/.../foo/x/y/z/foo
  char *cmdfn=mkcmdfn(name,dir,cext);	   // x/y/z/foo.cmd
  char *depfn=mkdepfn(name,dir,dext);
  char *sibfn=mksibfn(name,dir,sext);
  char *cmddst=vstrcat(dst,"/",cmdfn,NULL); // /tmp/.../foo/x/y/z/foo.cmd
  char *depdst=vstrcat(dst,"/",depfn,NULL);
  char *sibdst=vstrcat(dst,"/",sibfn,NULL);
  int succ=cache_put_cp(fpp,dst,name,tgtdst,
			cmdfn,depfn,sibfn,
			cmddst,depdst,sibdst);
  free(bn);
  free(dst);
  free(tgtdst);
  free(cmdfn);
  free(depfn);
  free(sibfn);
  free(cmddst);
  free(depdst);
  free(sibdst);
  return succ;
}
