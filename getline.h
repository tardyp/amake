#ifndef GETLINE_H
#define GETLINE_H

extern char *_getline(FILE *f, int eof, char *file, int line);

#define GETLINE(f) _getline(f,0,__FILE__,__LINE__)
#define TRYLINE(f) _getline(f,1,__FILE__,__LINE__)

#endif
