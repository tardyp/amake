#ifndef COMMAND_H
#define COMMAND_H

#include "fpp.h"

/***************************************************************************
 * This Command/Arguments trickery is to avoided leaking memory            *
 * in a multithreaded setjmp()/longjmp() alarm() environment.              *
 ***************************************************************************/

typedef struct {
  FPP *fpp;
  char *name;
  char *arch;
  char *chksum;
  char *cmdsum;
  char *cachedir;
} Arguments;

typedef struct Command {
  char *op;
  char *(*func)(Arguments *arguments);
  void (*lock)();
  void (*unlock)();
  Arguments *arguments;
} Command;

extern char *command_get(Arguments *arguments);
extern char *command_put(Arguments *arguments);

extern Command *command_new();
extern void command_free(Command *command);

extern void command_do(Command *command);

#endif
