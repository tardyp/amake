#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "make.h"
#include "filedef.h"
#include "variable.h"
#include "var.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

/***************************************************************************
 * This function is a string tokenizer. It returns a copy of               *
 * the substring in a particular field. The original string is             *
 * not modified.  Its return value can be free()-ed.                       *
 *                                                                         *
 * Arguments:                                                              *
 *     i = zero-origin field number                                        *
 *     s = string to tokenize                                              *
 *     sep = field separator                                               *
 ***************************************************************************/
extern char *field(int i, char *str, char sep) {
  while (str && i--) {
    str=strchr(str,sep);
    if (!str)
      return 0;
    if (*str==sep)
      str++;
  }
  char *end=strchr(str,sep);
  int len=(end ? end-str : strlen(str));
  char *s=(char *)malloc(len+1);
  if (!s)
    ERR("field(): malloc() failed");
  memcpy(s,str,len);
  *(s+len)=0;
  return s;
}


/***************************************************************************
 * This predicate returns true iff the variable-name parts of              *
 * two environment entries (i.e., "var=val") are equal. The                *
 * value parts are ignored.                                                *
 *                                                                         *
 * Arguments:                                                              *
 *     p = environment entry                                               *
 *     q = environment entry                                               *
 ***************************************************************************/
static int env_eq(char *p, char *q) {
  int more(char *s) { return *s && *s!='='; }
  for (; more(p) && more(q) && *p==*q; p++,q++);
  return more(p)||more(q);
}

/***************************************************************************
 * This function returns the value part of an environment entry            *
 * (i.e., "var=val"). If there is no value part "" is returned.            *
 *                                                                         *
 * Arguments:                                                              *
 *     v = environment entry                                               *
 ***************************************************************************/
static char *env_val(char *v) {
  while (*v && *v!='=')
    v++;
  if (*v=='=')
    v++;
  return v;
}

/***************************************************************************
 * This function returns the value of an environment variable,             *
 * or 0 if it does not exist.                                              *
 *                                                                         *
 * Arguments:                                                              *
 *     envp = ptr-to ptr-to zero-terminated sequence of                    *
 *            "var=val" entries (0 means use environ)                      *
 *     var = variable name to search for                                   *
 ***************************************************************************/
extern char *get_env(char ***envp, char *var) {
  if (!envp)
    envp=&environ;
  char **env=*envp;
  while (*env) {		/* search */
    if (!env_eq(*env,var))
      return env_val(*env);
    env++;
  }
  return 0;
}

/***************************************************************************
 * This function sets the value of an environment variable,                *
 * whether it exists or not. If the variable does not exist,               *
 * the environment is copied so a new entry can be added. The              *
 * old environment is free()-ed, unless it came from environ.              *
 *                                                                         *
 * Arguments:                                                              *
 *     envp = ptr-to ptr-to zero-terminated sequence of                    *
 *            "var=val" entries (0 means use environ)                      *
 *     var = variable to add or change                                     *
 *     val = variable's value                                              *
 ***************************************************************************/
extern void put_env(char ***envp, char *var, const char *val) {
  char *varval=vstrcat(var,"=",val,0);
  if (!envp)
    envp=&environ;
  char **env=*envp;
  char **o=env;
  while (*o) {			/* search */
    if (!env_eq(*o,varval)) {
      *o=varval;
      return;
    }
    o++;
  }
  char **new=(char **)malloc((o-env+2)*sizeof(char *));
  if (!new)
    ERR("put_env(): malloc() failed");
  char **n=new;
  *n++=varval;			/* add */
  o=env;
  while (*o)			/* copy */
    *n++=*o++;
  *n=0;
  if (envp!=&environ)
    free(env);
  *envp=new;
}

/***************************************************************************
 * This function adds to the value of an environment variable,             *
 * if it exists. If the variable does not exist, a new variable            *
 * is added with that value.                                               *
 *                                                                         *
 * Arguments:                                                              *
 *     envp = ptr-to ptr-to zero-terminated sequence of                    *
 *            "var=val" entries (0 means use environ)                      *
 *     var = variable name to add, change, or add to                       *
 *     val = variable value                                                *
 *     sep = inter-value separator                                         *
 ***************************************************************************/
extern void add_env(char ***envp, char *var, const char *val, char *sep) {
  if (!envp)
    envp=&environ;
  char *old=0;
  if (sep) {
    old=get_env(envp,var);
    if (old) {
      char *v;
      int i=0;
      while ((v=field(i++,old,' '))) {
	if (!strcmp(v,val)) {
	  free(v);
	  return;
	}
	free(v);
      }
    }
  }
  if (!old)
    old="";
  char *newval=(*old ? vstrcat(val,sep,old,0) : str_dup(val));
  put_env(envp,var,newval);
  free(newval);
}

/***************************************************************************
 * This function removes an environment variable. If the it                *
 * exists, the environment is copied so an entry can be                    *
 * removed. The old environment is free()-ed, unless it came               *
 * from environ.                                                           *
 *                                                                         *
 * Arguments:                                                              *
 *     envp = ptr-to ptr-to zero-terminated sequence of                    *
 *            "var=val" entries (0 means use environ)                      *
 *     var = variable name to add, change, or add to                       *
 ***************************************************************************/
extern void rem_env(char ***envp, char *var) {
  if (!get_env(envp,var))
    return;
  if (!envp)
    envp=&environ;
  char **env=*envp;
  char **o=env;
  while (*o++);
  char **new=(char **)malloc((o-env)*sizeof(char *));
  if (!new)
    ERR("rem_env(): malloc() failed");
  char **n=new;
  o=env;
  while (*o)			/* copy */
    if (!env_eq(*o,var))
      o++;
    else
      *n++=*o++;
  *n=0;
  if (envp!=&environ)
    free(env);
  *envp=new;
}

/***************************************************************************
 * This function returns the value of a makefile variable, or a            *
 * default value if it does not exist.                                     *
 *                                                                         *
 * Arguments:                                                              *
 *     name = variable name                                                *
 *     val = default value                                                 *
 *     file = context for variables value                                  *
 ***************************************************************************/
extern char *getvar(char *name, char *val, struct file *file) {
  if (file) {
    struct variable_set_list *save=current_variable_set_list;
    current_variable_set_list=file->variables;
    struct variable *var=lookup_variable(name,strlen(name));
    if (var)
      val=(var->recursive ? recursively_expand_for_file(var,file) : var->value);
    current_variable_set_list=save;
  } else {
    struct variable *var=lookup_variable(name,strlen(name));
    if (var)
      val=var->value;
  }
  return val;
}

/***************************************************************************
 * This function returns whether a makefile variable is set to             *
 * a legitimate value.                                                     *
 *                                                                         *
 * Arguments:                                                              *
 *     name = variable name                                                *
 *     file = context for variables value                                  *
 ***************************************************************************/
extern int chkvar(char *name, struct file *file) {
  char *val=getvar(name,0,file);
  return val && *val;
}
