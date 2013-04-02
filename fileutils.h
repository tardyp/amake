#ifndef FILEUTILS_H
#define FILEUTILS_H

extern char *dirname(const char *s);
extern char *basname(const char *s);

extern void mkdirp(char *p);

extern int inode(int fd);
extern void close_above(int fd);

extern int isdir(const char *fn);
extern int isreg(const char *fn);
extern int issym(const char *fn);
extern int isempty(const char *fn);

extern char *tolmfile(const char *fn);

extern char *mkcmdfn(const char *name, const char *dir, const char *ext);
extern char *mkdepfn(const char *name, const char *dir, const char *ext);
extern char *mksibfn(const char *name, const char *dir, const char *ext);

extern char *mkoldfn(const char *fn, const char *ext);

extern int samefs(const char *src, const char *dst);
extern int fcp(const char *src, const char *dst);

extern char *freadfirstln(const char *fn);

extern void fmove(const char *src, const char *dst, int *succ);

#endif
