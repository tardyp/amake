#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "fpp.h"
#include "db.h"
#include "options.h"
#include "getline.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

static FPP *fpp;		/* file pointers of tid socket */

/***************************************************************************
 * This function opens a socket to the target index daemon                 *
 * (tid). It returns a file descriptor for the socket, or zero             *
 * for failure.                                                            *
 *                                                                         *
 * Arguments:                                                              *
 *     hostname = tid hostname                                             *
 *     port = tid port                                                     *
 ***************************************************************************/
static int db_tid_connect(char *hostname, int port) {
  int fd=socket(PF_INET,SOCK_STREAM,0);
  if (fd<0)
    ERR("db_tid_connect(): socket() failed");
  struct sockaddr_in addr;
  bzero(&addr,sizeof(addr));
  addr.sin_family=AF_INET;
  struct hostent *host=gethostbyname(hostname);
  if (!host)
    ERR("gethostbyname() failed");
  memcpy(&addr.sin_addr,host->h_addr,host->h_length);
  addr.sin_port=htons(port);
  if (connect(fd,(struct sockaddr *)&addr,sizeof(addr))<0) {
    close(fd);
    fd=0;
  }
  return fd;
}

/***************************************************************************
 * This function simply opens the database connection.                     *
 ***************************************************************************/
DB_DCL(open,tid_) {
  DEBUG("in db_tid_open()");
  int fd=db_tid_connect(opt_tid_host(),opt_tid_port());
  if (!fd)
    ERR("db_tid_connect() failed");
  fpp=fpp_open(fd);
  DEBUG("db_tid_open() done");
  return 0;
}

/***************************************************************************
 * This function simply closes the database connection.                    *
 ***************************************************************************/
DB_DCL(close,tid_) {
  fpp_close(fpp);
  return 0;
}

/***************************************************************************
 * This function performs the database access for a "get"                  *
 * command. It submits a query and processes the result. The               *
 * result is a zero-terminated array of strings, where each                *
 * string is a path in the cache of a candidate target. For                *
 * example:                                                                *
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
DB_DCL(get,tid_) {
  DEBUG("in db_tid_get(\"%s\",\"%s\",\"%s\")",name,arch,cmdsum);
  fprintf(fpp->w,"get\n%s\n%s\n%s\n",
	  name,
	  arch,
	  cmdsum);
  fflush(fpp->w);
  char *count=GETLINE(fpp->r);
  DEBUG("in db_tid_get() count=%s",count);
  int n=atoi(count);
  free(count);
  char **cachedirs=(char **)malloc((n+1)*sizeof(char *));
  int i;
  for (i=0; i<n; i++)
    cachedirs[i]=GETLINE(fpp->r);
  cachedirs[i]=0;
  DEBUG("db_tid_get() done");
  return cachedirs;
}

/***************************************************************************
 * This function performs the database access for a "put"                  *
 * command. It returns whether it was successful.                          *
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
DB_DCL(put,tid_) {
  DEBUG("in db_tid_put(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\")",
	name,arch,chksum,cmdsum,cachedir);
  fprintf(fpp->w,"put\n%s\n%s\n%s\n%s\n%s\n",
	  name,
	  arch,
	  chksum,
	  cmdsum,
	  cachedir);
  fflush(fpp->w);
  char *count=GETLINE(fpp->r);
  int n=atoi(count);
  free(count);
  DEBUG("db_tid_put() done, %s in cache",(n==1 ? "now" : "already"));
  return n;
}

/***************************************************************************
 * This function logs a database access.                                   *
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
DB_DCL(log,tid_) {
  if (!opt_dblog())
    return 0;
  return 0;
}
