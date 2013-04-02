#ifndef VAR_H
#define VAR_H

extern char *field(int i, char *str, char sep);

extern char *get_env(char ***envp, char *var);
extern void put_env(char ***envp, char *var, const char *val);
extern void add_env(char ***envp, char *var, const char *val, char *sep);
extern void rem_env(char ***envp, char *var);

extern char *getvar(char *name, char *val, struct file *file);
extern int chkvar(char *name, struct file *file);

#endif
