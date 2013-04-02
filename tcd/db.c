#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "options.h"
#include "err.h"

/***************************************************************************
 * This is the type for a vtable.                                          *
 ***************************************************************************/
typedef struct {
  DB_PTR(open);
  DB_PTR(close);
  DB_PTR(get);
  DB_PTR(put);
  DB_PTR(log);
} VT;

/***************************************************************************
 * This is the type for a database manager/vtable map entry.               *
 ***************************************************************************/
typedef struct {
  char *dbmgr;
  VT *vt;
} VTS;

/***************************************************************************
 * This is a vtable.                                                       *
 ***************************************************************************/
#define VT_STRUCT(n) static VT vt_##n={ \
    db_##n##_open, \
    db_##n##_close, \
    db_##n##_get, \
    db_##n##_put, \
    db_##n##_log \
}

/***************************************************************************
 * This is the set of function declarations for a database manager.        *
 ***************************************************************************/
#define DB_DCLS(m) \
  DB_DCL(open,m##_); \
  DB_DCL(close,m##_); \
  DB_DCL(get,m##_); \
  DB_DCL(put,m##_); \
  DB_DCL(log,m##_)

/***************************************************************************
 * These are function declarations.                                        *
 ***************************************************************************/

DB_DCLS(tid);
DB_DCLS(myd);

/***************************************************************************
 * These are vtables.                                                      *
 ***************************************************************************/

VT_STRUCT(tid);
VT_STRUCT(myd);

/***************************************************************************
 * This is the a database manager/vtable map.                              *
 ***************************************************************************/
static const VTS vts[]={
  {"tid",&vt_tid},
  {"myd",&vt_myd},
  {0,0}
};

/***************************************************************************
 * This is a pointer to the active vtable.                                 *
 ***************************************************************************/
static VT *vt=0;

/***************************************************************************
 * Set the active vtable.                                                  *
 ***************************************************************************/
static void init() {
  const VTS *p;
  for (p=vts; p->dbmgr; p++)
    if (!strcmp(opt_dbmgr(),p->dbmgr)) {
      vt=p->vt;
      return;
    }
  ERR("db.c: init(): unsupported database manager: %s",opt_dbmgr());
}

/***************************************************************************
 * A function definition calls through the vtable.                         *
 ***************************************************************************/
#define DB_DEF(n) DB_DCL(n) { \
  if (!vt) init(); \
  return vt->n(DB_##n##_ARGS); \
}

/***************************************************************************
 * These are the function definitions.                                     *
 ***************************************************************************/

DB_DEF(open);
DB_DEF(close);
DB_DEF(get);
DB_DEF(put);
DB_DEF(log);
