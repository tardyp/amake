#include <stdio.h>
#include <string.h>
#include <my_global.h>
#include <mysql.h>

#include "db.h"
#include "options.h"
#include "str.h"
#include "err.h"

static MYSQL *conn=0;

/***************************************************************************
 * This function simply opens the database connection.                     *
 ***************************************************************************/
DB_DCL(open,myd_) {
  DEBUG("in db_myd_open()");
  conn=mysql_init(0);
  if (!conn)
    ERR(vstrcat("mysql_init(): ",mysql_error(conn),NULL));
  if (!mysql_real_connect(conn,0,opt_myd_user(),0,opt_dbname(),0,0,0))
    ERR(vstrcat("db_myd_open(): ",mysql_error(conn),NULL));
  DEBUG("db_myd_open() done");
  return 0;
}

/***************************************************************************
 * This function simply closes the database connection.                    *
 ***************************************************************************/
DB_DCL(close,myd_) {
  mysql_close(conn);
  return 0;
}

/***************************************************************************
 * This function performs the database access for a "get"                  *
 * command. It builds an SQL query, submits it, and processes              *
 * the result. The result is a zero-terminated array of                    *
 * strings, where each string is a path in the cache of a                  *
 * candidate target. For example:                                          *
 *                                                                         *
 *     /tmp/tcd/buff/spud/2010-04-20/25245/14:46:39/foo.o                  *
 *                                                                         *
 * Arguments:                                                              *
 *     name = target name                                                  *
 *         (e.g., x/y/z/foo.o)                                             *
 *     arch = target architecture                                          *
 *         (e.g., "Linux_i686_2.6.30.10-105.fc11.i686.PAE")                *
 *     cmdsum = checksum of shell commands that build target               *
 *         (e.g., "gcc -c x/y/z/foo.c")                                    *
 ***************************************************************************/
DB_DCL(get,myd_) {
  char *query;
  query=vstrcat("select distinct cachedir from entries where "
		"name = "  ,"'",name,"'"," and ",
		"arch = "  ,"'",arch,"'"," and ",
		"cmdsum = ","'",cmdsum,"';",
		NULL);
  DEBUG("in db_myd_get(), query=\"%s\"",query);
  if (mysql_query(conn,query))
    ERR(vstrcat("db_myd_get(): ",mysql_error(conn),NULL));
  free(query);
  MYSQL_RES *result=mysql_store_result(conn);
  if (!result)
    ERR(vstrcat("db_myd_get(): ","use_result: ",mysql_error(conn),NULL));
  int n=mysql_num_rows(result);
  char **cachedirs=(char **)malloc((n+1)*sizeof(char *));
  if (!cachedirs)
    ERR("db_myd_get(): malloc() failed");
  int i;
  for (i=0; i<n; i++) {
    MYSQL_ROW row=mysql_fetch_row(result);
    cachedirs[i]=str_dup(row[0]);
  }
  cachedirs[i]=0;
  mysql_free_result(result);
  DEBUG("db_myd_get() done, %d in cache",n);
  return cachedirs;
}

/***************************************************************************
 * This function performs the database access for a "put"                  *
 * command.                                                                *
 *                                                                         *
 * First, it builds an SQL "select" query, submits it, and                 *
 * processes the result. This determines if the target is                  *
 * already in the database. If so, it returns 0.                           *
 *                                                                         *
 * If not, it then builds an SQL "insert" query, submits it,               *
 * and returns 1.                                                          *
 *                                                                         *
 * Arguments:                                                              *
 *     date = current date: "yyyy-mm-dd"                                   *
 *     time = current time: "hh:mm:ss"                                     *
 *     name = target name                                                  *
 *         (e.g., x/y/z/foo.o)                                             *
 *     arch = target architecture                                          *
 *         (e.g., "Linux_i686_2.6.30.10-105.fc11.i686.PAE")                *
 *     chksum = checksum of content of target file, to avoid               *
 *         duplicate database/cache entries                                *
 *         (e.g., "x/y/z/foo.c")                                           *
 *     cmdsum = checksum of shell commands that build target               *
 *         (e.g., "gcc -c x/y/z/foo.c")                                    *
 *     cachedir = path in the cache of the target                          *
 *         (e.g., "/tmp/tcd/buff/spud/2010-04-20/25245/14:46:39/foo.o")    *
 ***************************************************************************/
DB_DCL(put,myd_) {
  DEBUG("in db_myd_put(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\")",
	name,arch,chksum,cmdsum,cachedir);
  char *query;
  query=vstrcat("select distinct cachedir from entries where "
  		"name = "  ,"'",name,"'"  ," and ",
  		"arch = "  ,"'",arch,"'"  ," and ",
  		"chksum = ","'",chksum,"'"," and ",
  		"cmdsum = ","'",cmdsum,"';",
  		NULL);
  DEBUG("in db_myd_put(), query=\"%s\"",query);
  if (mysql_query(conn,query))
    ERR(vstrcat("db_myd_put(): ","select: ",mysql_error(conn),NULL));
  free(query);
  MYSQL_RES *result=mysql_use_result(conn);
  if (!result)
    ERR(vstrcat("db_myd_put(): ","use_result: ",mysql_error(conn),NULL));
  MYSQL_ROW row=mysql_fetch_row(result);
  mysql_free_result(result);
  if (row) {
    DEBUG("db_myd_put() done, already in cache");
    return 0;
  }
  query=vstrcat("insert into entries values ("
		"'",date," ",time,"',",
		"'",name,"',",
		"'",arch,"',",
		"'",chksum,"',",
		"'",cmdsum,"',",
		"'",cachedir,"');",
		NULL);
  DEBUG("in db_myd_put(), query=\"%s\"",query);
  if (mysql_query(conn,query))
    ERR(vstrcat("db_myd_put(): ","insert: ",mysql_error(conn),NULL));
  free(query);
  DEBUG("db_myd_put() done, now in cache");
  return 1;
}

/***************************************************************************
 * This function logs a database access.  It builds an SQL                 *
 * "insert" query, and submits it.                                         *
 *                                                                         *
 * Arguments:                                                              *
 *     op = operation                                                      *
 *         (e.g., "get")                                                   *
 *     date = current date: "yyyy-mm-dd"                                   *
 *     time = current time: "hh:mm:ss"                                     *
 *     name = target name                                                  *
 *         (e.g., x/y/z/foo.o)                                             *
 *     cwd = current working directory                                     *
 *         (e.g., "/home/buff/foo")                                        *
 *     result = result of access                                           *
 *         (e.g., "success").                                              *
 ***************************************************************************/
DB_DCL(log,myd_) {
  if (!opt_dblog())
    return 0;
  char *query=vstrcat("insert into log values ("
		      "'",op,"',",
		      "'",date," ",time,"',",
		      "'",str_host(),"',",
		      "'",str_user(),"',",
		      "'",name,"',",
		      "'",cwd,"',",
		      "'",result,"');",
		      NULL);
  if (mysql_query(conn,query))
    ERR(vstrcat("db_myd_log(): ",mysql_error(conn),NULL));
  free(query);
  return 0;
}
