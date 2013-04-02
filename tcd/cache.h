#ifndef CACHE_H
#define CACHE_H

extern int cache_get(FPP *fpp, char *cwd, char **cachedirs,
		     char *name, char *dir,
		     char *cext, char *dext, char *sext);

extern int cache_put(FPP *fpp, char *cwd, char *cachedir,
		     char *name, char *dir,
		     char *cext, char *dext, char *sext);

#endif
