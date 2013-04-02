#ifndef STR_H
#define STR_H

extern char *str_dup(const char *s);

extern char *vstrcat(const char *s, ...);
extern char *vstrcatf(char *s, ...);

extern void str_chrrepl(char *s, char from, char to);

extern char *str_int(int i);
extern void str_inc(char **s);
extern void str_dec(char **s);

extern char *str_pid();
extern char *str_time();

extern char *str_date_yymmdd();
extern char *str_time_hhmmss();
extern char *str_user();
extern char *str_host();
extern char *str_arch();

extern int str_rm(char *s, char *t);
extern int str_rm_and_after(char *s, char *t);
extern int str_rm_and_before(char *s, char *t);

#endif
