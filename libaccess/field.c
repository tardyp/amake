#include "field.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern HIDDEN char *field(int i, char *str, char sep) {
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
    ERROR("field(): malloc() failed");
  memcpy(s,str,len);
  *(s+len)=0;
  return s;
}
