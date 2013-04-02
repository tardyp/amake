#ifndef RECORDS_H
#define RECORDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

/***************************************************************************
 * These are the "records" for the various db keys and                     *
 * values. They are a more convenient interface than the GDBM              *
 * datum type.                                                             *
 ***************************************************************************/

typedef struct {
  char *name;
  char *value;
} Field;

typedef Field Record[];

typedef struct {
  char *code;
  char *name;
  Record *key;
  Record *val;
} Type;

typedef Type Types[];

static char *rec_code(Types types, char *name) {
  int i;
  for (i=0; types[i].code && strcmp(types[i].name,name); i++);
  return types[i].code;
}

static char *rec_name(Types types, char *code) {
  int i;
  for (i=0; types[i].code && strcmp(types[i].code,code); i++);
  return types[i].name;
}

static Record *rec_key(Types types, char *code) {
  int i;
  for (i=0; types[i].code && strcmp(types[i].code,code); i++);
  return types[i].key;
}

static Record *rec_val(Types types, char *code) {
  int i;
  for (i=0; types[i].code && strcmp(types[i].code,code); i++);
  return types[i].val;
}

static void rec_free(Record r) {
  void nzfree(void *p) { if (p) free(p); }
  int i;
  for (i=0; r[i].name; i++)
    nzfree(r[i].value);
}

#define ALLOC_RECORDS                                      \
     Record serialkey={                                    \
      {"code",0},                                          \
      {"name",0},                                          \
      {"arch",0},                                          \
      {"cmdsum",0},                                        \
      {0,0},                                               \
     };                                                    \
                                                           \
     Record serialval={                                    \
      {"serial",0},                                        \
      {0,0},                                               \
     };                                                    \
                                                           \
     Record cachedkey={                                    \
      {"code",0},                                          \
      {"name",0},                                          \
      {"arch",0},                                          \
      {"cmdsum",0},                                        \
      {"serial",0},                                        \
      {0,0},                                               \
     };                                                    \
                                                           \
     Record cachedval={                                    \
      {"date",0},                                          \
      {"time",0},                                          \
      {"cachedir",0},                                      \
      {0,0},                                               \
     };                                                    \
                                                           \
     Record chksumkey={                                    \
      {"code",0},                                          \
      {"name",0},                                          \
      {"arch",0},                                          \
      {"chksum",0},                                        \
      {"cmdsum",0},                                        \
      {0,0},                                               \
     };                                                    \
                                                           \
     Record chksumval={                                    \
      {"serial",0},                                        \
      {0,0},                                               \
     };                                                    \
                                                           \
     Types types={                                         \
       {"a","serialkey",&serialkey,&serialval},            \
       {"b","cachedkey",&cachedkey,&cachedval},            \
       {"c","chksumkey",&chksumkey,&chksumval},            \
       {0,0},                                              \
     };                                                    \
                                                           \
     db_put(serialkey,"code",rec_code(types,"serialkey")); \
     db_put(cachedkey,"code",rec_code(types,"cachedkey")); \
     db_put(chksumkey,"code",rec_code(types,"chksumkey")); \

#define FREE_RECORDS     \
    rec_free(serialkey); \
    rec_free(serialval); \
    rec_free(cachedkey); \
    rec_free(cachedval); \
    rec_free(chksumkey); \
    rec_free(chksumval); \

#endif
