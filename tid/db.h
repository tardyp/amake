#ifndef DB_H
#define DB_H

#include <gdbm.h>

#include "records.h"

typedef enum {RO,RW} DBMODE;

extern void db_open(DBMODE mode);
extern void db_close();

extern char *db_get(Record r, char *name);
extern void db_put(Record r, char *name, char *value);

extern void db_pack(datum *d, Record r);
extern void db_unpack(Record r, datum *d);

extern int db_fetch(Record key, Record val);
extern void db_store(Record key, Record val);

extern datum db_firstkey();
extern datum db_nextkey(datum key);

extern void db_print(Record r, char *sep, char *end);

#endif
