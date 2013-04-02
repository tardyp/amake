#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <netdb.h>
#include <sys/utsname.h>

#include "str.h"
#include "err.h"

/***************************************************************************
 * This function is a safe wrapper for strdup.                             *
 *                                                                         *
 * Arguments:                                                              *
 *     s = first string                                                    *
 ***************************************************************************/
extern char *str_dup(const char *s) {
  char *r=0;
  if (s) {
    r=strdup(s);
    if (!r)
      ERR("str_dup(): strdup() failed");
  }
  return r;
}

/***************************************************************************
 * This function catenates a sequence of strings.  Its return              *
 * value can be free()-ed. Its last argument must be 0.                    *
 *                                                                         *
 * Arguments:                                                              *
 *     s = first string                                                    *
 ***************************************************************************/
extern char *vstrcat(const char *s, ...) {
  const char *a;
  va_list ap;
  int l=0;
  for (va_start(ap,s),a=s; a; a=va_arg(ap,char *))
    l+=strlen(a);
  va_end(ap);
  char *r=(char *)malloc(l+1);
  if (!r)
    ERR("vstrcat(): malloc() failed");
  *r=0;
  for (va_start(ap,s),a=s; a; a=va_arg(ap,char *))
    strcat(r,a);
  va_end(ap);
  return r;
}

/***************************************************************************
 * This function is like vstrcat, but it free()-s its first                *
 * argument.                                                               *
 *                                                                         *
 * Arguments:                                                              *
 *     s = first string                                                    *
 ***************************************************************************/
extern char *vstrcatf(char *s, ...) {
  char *a;
  va_list ap;
  int l=0;
  for (va_start(ap,s),a=s; a; a=va_arg(ap,char *))
    l+=strlen(a);
  va_end(ap);
  char *r=(char *)malloc(l+1);
  if (!r)
    ERR("vstrcat(): malloc() failed");
  *r=0;
  for (va_start(ap,s),a=s; a; a=va_arg(ap,char *))
    strcat(r,a);
  va_end(ap);
  free(s);
  return r;
}

/***************************************************************************
 * This function replaces every occurrence of one character in             *
 * a string with another character.                                        *
 *                                                                         *
 * Arguments:                                                              *
 *     s = string to process                                               *
 *     from = character to replace                                         *
 *     to = replacement character                                          *
 ***************************************************************************/
extern void str_chrrepl(char *s, char from, char to) {
  while (*s) {
    *s=(*s==from ? to : *s);
    s++;
  }
}

/***************************************************************************
 * This function returns a string that looks like a number.                *
 *                                                                         *
 * Arguments:                                                              *
 *     i = number                                                          *
 ***************************************************************************/
extern char *str_int(int i) {
  static char buf[24]; // big enough for 64-bit unsigned integer
  sprintf(buf,"%d",i);
  return buf;
}

/***************************************************************************
 * This function adds one to a string that looks like a number.            *
 *                                                                         *
 * Arguments:                                                              *
 *     s = pointer to string to process                                    *
 ***************************************************************************/
extern void str_inc(char **s) {
  char *buf=str_int(atoi(*s)+1);
  if (strlen(buf)<=strlen(*s)) {
    strcpy(*s,buf);
    return;
  }
  free(*s);
  *s=str_dup(buf);
}

/***************************************************************************
 * This function subtracts one from a string that looks like a number.     *
 *                                                                         *
 * Arguments:                                                              *
 *     s = pointer to string to process                                    *
 ***************************************************************************/
extern void str_dec(char **s) {
  char *buf=str_int(atoi(*s)-1);
  if (strlen(buf)<=strlen(*s)) {
    strcpy(*s,buf);
    return;
  }
  free(*s);
  *s=str_dup(buf);
}

/***************************************************************************
 * This function returns a string representation of the                    *
 * process's PID.  Its return value is static.                             *
 ***************************************************************************/
extern char *str_pid() {
  static char buf[24]; // big enough for 64-bit unsigned integer
  sprintf(buf,"%d",getpid());
  return buf;
}

/***************************************************************************
 * This function returns a string representation of the number             *
 * of seconds past the Epoch.  Its return value is static.                 *
 ***************************************************************************/
extern char *str_time() {
  static char buf[24]; // big enough for 64-bit unsigned integer
  sprintf(buf,"%ld",time(0));
  return buf;
}

/***************************************************************************
 * This function returns a string representation of the current            *
 * date.  Its return value is static.                                      *
 ***************************************************************************/
extern char *str_date_yymmdd() {
  time_t t=time(0);
  struct tm *tp=localtime(&t);
  // static char buf[strlen("yyyy-mm-dd")+1];
  static char buf[10+1];
  sprintf(buf,"%04d-%02d-%02d",tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday);
  return buf;
}

/***************************************************************************
 * This function returns a string representation of the current            *
 * time.  Its return value is static.                                      *
 ***************************************************************************/
extern char *str_time_hhmmss() {
  time_t t=time(0);
  struct tm *tp=localtime(&t);
  // static char buf[strlen("hh:mm:ss")+1];
  static char buf[8+1];
  sprintf(buf,"%02d:%02d:%02d",tp->tm_hour,tp->tm_min,tp->tm_sec);
  return buf;
}

/***************************************************************************
 * This function returns the user's login name.  Its return                *
 * value is static.                                                        *
 ***************************************************************************/
extern char *str_user() {
  struct passwd *pw=getpwuid(getuid());
  return pw->pw_name;
}

/***************************************************************************
 * This function returns the host machine's name.  Its return              *
 * value is static.                                                        *
 ***************************************************************************/
extern char *str_host() {
  static char *name=0;
  if (!name) {
    const int max=80;
    name=(char *)malloc(max+1);
    if (!name)
      ERR("str_host(): malloc() failed");
    gethostname(name,max);
  }
  return name;
}

/***************************************************************************
 * This function returns the host machine's architecture.  Its             *
 * return value is static.                                                 *
 ***************************************************************************/
extern char *str_arch() {
  static char *arch=0;
  if (!arch) {
    struct utsname buf;
    if (uname(&buf))
      ERR("str_arch(): uname() failed");
    arch=vstrcat(buf.sysname,"_",
    		 buf.machine,"_",
    		 buf.release,
    		 NULL);
  }
  return arch;
}

/***************************************************************************
 * This function returns the result of the following string                *
 * operation:                                                              *
 *                                                                         *
 *     aaaTTTbbb => aaabbb                                                 *
 ***************************************************************************/
extern int str_rm(char *s, char *t) {
  char *p=strstr(s,t);		/* &TTT */
  if (!p)
    return 0;
  strcpy(p,p+strlen(t));
  return 1;
}

/***************************************************************************
 * This function returns the result of the following string                *
 * operation:                                                              *
 *                                                                         *
 *     aaaTTTbbb => aaa                                                    *
 ***************************************************************************/
extern int str_rm_and_after(char *s, char *t) {
  char *p=strstr(s,t);		/* &TTT */
  if (!p)
    return 0;
  *p=0;
  return 1;
}

/***************************************************************************
 * This function returns the result of the following string                *
 * operation:                                                              *
 *                                                                         *
 *     aaaTTTbbb => bbb                                                    *
 ***************************************************************************/
extern int str_rm_and_before(char *s, char *t) {
  char *p=strstr(s,t);		/* &TTT */
  if (!p)
    return 0;
  strcpy(s,p+strlen(t));
  return 1;
}
