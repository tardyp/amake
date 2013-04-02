#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "db.h"
#include "options.h"
#include "str.h"
#include "err.h"

static GDBM_FILE dbf=0;

/***************************************************************************
 * This function is the error callback for GDBM.                           *
 ***************************************************************************/
static void db_err(char *msg) {
  ERR("gdbm error: pid=%d, %s",getpid(),msg);
}

/***************************************************************************
 * The function creates or opens the database.                             *
 *                                                                         *
 * Arguments:                                                              *
 *     mode = RO or RW                                                     *
 ***************************************************************************/
extern void db_open(DBMODE mode) {
  DEBUG("in db_open(): pid=%d",getpid());
  if (dbf)
    ERR("db_open(): already open");
  int flags;
  int perms;
  switch (mode) {
    case RO:
      flags=GDBM_READER;
      perms=0;
      break;
    case RW:
      flags=GDBM_WRCREAT|GDBM_NOLOCK;
      perms=S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
      break;
    default:
      ERR("db_opendb(): invalid mode: %d",mode);
  }
  dbf=gdbm_open(opt_db_file(),0,flags,perms,db_err);
  if (!dbf)
    ERR("db_opendb(): gdbm_open() failed: "
	"gdbm_errno=\"%s\" errno=\"%s\"",
	gdbm_strerror(gdbm_errno),strerror(errno));
}

/***************************************************************************
 * The function closes the database.                                       *
 ***************************************************************************/
extern void db_close() {
  DEBUG("in db_close() pid=%d",getpid());
  if (!dbf)
    ERR("db_close(): not open");
  gdbm_close(dbf);
  dbf=0;
}

/***************************************************************************
 * This function is a safe wrapper for strcpy.                             *
 ***************************************************************************/
static void scpy(char *s, char *t) {
  if (t)
    strcpy(s,t);
  else
    *s=0;
}

/***************************************************************************
 * This function is a safe wrapper for strlen.                             *
 ***************************************************************************/
static int slen(char *s) {
  return s ? strlen(s) : 0;
}

/***************************************************************************
 * This function gets a field from a record. The return value              *
 * can be free()-ed.                                                       *
 *                                                                         *
 * Arguments:                                                              *
 *     r = record                                                          *
 *     name = name of field                                                *
 ***************************************************************************/
extern char *db_get(Record r, char *name) {
  int i;
  for (i=0; r[i].name && strcmp(r[i].name,name); i++);
  return str_dup(r[i].value);
}

/***************************************************************************
 * This function sets a field of a record to a new copy of a               *
 * value.                                                                  *
 *                                                                         *
 * Arguments:                                                              *
 *     r = record                                                          *
 *     name = name of field                                                *
 *     value = value to set                                                *
 ***************************************************************************/
extern void db_put(Record r, char *name, char *value) {
  int i;
  for (i=0; r[i].name && strcmp(r[i].name,name); i++);
  if (r[i].name) {
    if (r[i].value)
      free(r[i].value);
    r[i].value=str_dup(value);
  }
}

/***************************************************************************
 * This function copies a record to a datum. Values are new                *
 * copies.                                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     d = datum                                                           *
 *     r = record                                                          *
 ***************************************************************************/
extern void db_pack(datum *d, Record r) {
  // ...0...0
  // 012345678
  int size=0;
  int i;
  for (i=0; r[i].name; i++)
    size+=slen(r[i].value)+1;
  char *p=(char *)malloc(size); // allocate datum
  if (!p)
    ERR("db_pack(): malloc() failed");
  d->dptr=p;
  d->dsize=size;
  for (i=0; r[i].name; i++) { 	/* d <-- r */
    scpy(p,r[i].value);
    p+=slen(p)+1;		/* past 0 */
  }
}

/***************************************************************************
 * This function copies a datum to a record. Values are new                *
 * copies.                                                                 *
 *                                                                         *
 * Arguments:                                                              *
 *     r = record                                                          *
 *     d = datum                                                           *
 ***************************************************************************/
extern void db_unpack(Record r, datum *d) {
  // ...0...0
  // 012345678
  char *p=d->dptr;
  int i;
  for (i=0; r[i].name; i++) {	/* r <-- d */
    if (r[i].value)
      free(r[i].value);
    r[i].value=str_dup(p);
    p+=slen(p)+1;		/* past 0 */
    if (p>(d->dptr+d->dsize))
      ERR("db_unpack(): datum has too few fields (db corruption?)");
  }
  if (p!=(d->dptr+d->dsize))
    ERR("db_unpack(): datum has too many fields (db corruption?)");
}

/***************************************************************************
 * This function fetches a record from the database. Values are            *
 * new copies.                                                             *
 *                                                                         *
 * Arguments:                                                              *
 *     key = key to fetch                                                  *
 *     val = fetched value                                                 *
 ***************************************************************************/
extern int db_fetch(Record key, Record val) {
  datum keyd;
  db_pack(&keyd,key);
  datum vald=gdbm_fetch(dbf,keyd);
  free(keyd.dptr);
  if (!vald.dptr)
    return 0;
  db_unpack(val,&vald);
  free(vald.dptr);
  return 1;
}

/***************************************************************************
 * This function stores a record in the database. Values are               *
 * new copies.                                                             *
 *                                                                         *
 * Arguments:                                                              *
 *     key = key to store                                                  *
 *     val = value to store                                                *
 ***************************************************************************/
extern void db_store(Record key, Record val) {
  datum keyd;
  datum vald;
  db_pack(&keyd,key);
  db_pack(&vald,val);
  int r=gdbm_store(dbf,keyd,vald,GDBM_REPLACE);
  if (r)
    ERR("db_store(): gdbm_store() failed: %d",r);
  free(keyd.dptr);
  free(vald.dptr);
}

/***************************************************************************
 * This function is just a wrapper for the library function.               *
 ***************************************************************************/
extern datum db_firstkey() {
  return gdbm_firstkey(dbf);
}

/***************************************************************************
 * This function is just a wrapper for the library function.               *
 ***************************************************************************/
extern datum db_nextkey(datum key) {
  return gdbm_nextkey(dbf,key);
}

/***************************************************************************
 * This function prints a key or data record to stdout.                    *
 *                                                                         *
 * Arguments:                                                              *
 *     r = the record                                                      *
 *     sep = field separator                                               *
 *     end = record separator                                              *
 ***************************************************************************/
extern void db_print(Record r, char *sep, char *end) {
  char *s="";
  int i;
  for (i=0; r[i].name; i++) {
    printf("%s%s",sep,r[i].value);
    s=sep;
  }
  printf("%s",end);
}
