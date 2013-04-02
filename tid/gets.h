#ifndef GETS_H
#define GETS_H

#include <stdio.h>
#include <setjmp.h>

extern void _gets(char **s, FILE *f, jmp_buf env, int eof,
		  char *file, int line);

#define GETS(s,f,env) _gets(s,f,env,0,__FILE__,__LINE__)

#endif
