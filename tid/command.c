#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "fpp.h"
#include "command.h"
#include "options.h"
#include "db.h"
#include "rwl.h"
#include "getline.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

#include "records.h"

/***************************************************************************
 * This function performs a "get" command:                                 *
 *     1) Fetch the serial record, if it exists.                           *
 *     2) Write count to socket.                                           *
 *     3) Count down, fetching cachedir records, writing to the            *
 *        socket.                                                          *
 *                                                                         *
 * It returns a result string of "success" or "failure".                   *
 *                                                                         *
 * Arguments:                                                              *
 *     a = arguments for command                                           *
 ***************************************************************************/
extern char *command_get(Arguments *a) {
  ALLOC_RECORDS;
  char *result="failure";
  DEBUG("         name=\"%s\"",a->name);
  DEBUG("         arch=\"%s\"",a->arch);
  DEBUG("         cmdsum=\"%s\"",a->cmdsum);
  db_put(serialkey,"name",a->name);
  db_put(serialkey,"arch",a->arch);
  db_put(serialkey,"cmdsum",a->cmdsum);
  char *serial;
  // 1) Fetch the serial record, if it exists.
  if (db_fetch(serialkey,serialval))
    serial=db_get(serialval,"serial");
  else
    serial=str_dup("0");
  // 2) Write count to socket.
  DEBUG("         serial=\"%s\"",serial);
  fprintf(a->fpp->w,"%s\n",serial);
  fflush(a->fpp->w);
  db_put(cachedkey,"name",a->name);
  db_put(cachedkey,"arch",a->arch);
  db_put(cachedkey,"cmdsum",a->cmdsum);
  // 3) Count down, fetching cachedir records, writing to the socket.
  for (; strcmp(serial,"0"); str_dec(&serial)) {
    db_put(cachedkey,"serial",serial);
    char *cachedir;
    if (db_fetch(cachedkey,cachedval)) {
      cachedir=db_get(cachedval,"cachedir");
      result="success";
    } else {
      cachedir=str_dup("/");
      LOG("get(): cachedir record missing:");
      LOG("       Is the database corrupt?");
      LOG("       name=\"%s\"",a->name);
      LOG("       arch=\"%s\"",a->arch);
      LOG("       cmdsum=\"%s\"",a->cmdsum);
      LOG("       serial=\"%s\"",serial);
    }
    fprintf(a->fpp->w,"%s\n",cachedir);
    DEBUG("       cachedir=\"%s\"",cachedir);
    free(cachedir);
  }
  fflush(a->fpp->w);
  free(serial);
  FREE_RECORDS;
  return result;
}

/***************************************************************************
 * This function performs a "put" command:                                 *
 *     1) Fetch the chksum record, if it exists.                           *
 *     2) Fetch the serial record, if it exists.                           *
 *     3) Store the updated serial record.                                 *
 *     4) Store the chksum record.                                         *
 *     5) Store the cachedir record.                                       *
 *     6) Write count to socket.                                           *
 *                                                                         *
 * It returns a result string of "success" or "failure".                   *
 *                                                                         *
 * Arguments:                                                              *
 *     a = arguments for command                                           *
 ***************************************************************************/
extern char *command_put(Arguments *a) {
  ALLOC_RECORDS;
  char *result="failure";
  char *count="0";
  DEBUG("         name=\"%s\"",a->name);
  DEBUG("         arch=\"%s\"",a->arch);
  DEBUG("         chksum=\"%s\"",a->chksum);
  DEBUG("         cmdsum=\"%s\"",a->cmdsum);
  DEBUG("         cachedir=\"%s\"",a->cachedir);
  db_put(chksumkey,"name",a->name);
  db_put(chksumkey,"arch",a->arch);
  db_put(chksumkey,"chksum",a->chksum);
  db_put(chksumkey,"cmdsum",a->cmdsum);
  // 1) Fetch the chksum record, if it exists.
  if (!db_fetch(chksumkey,chksumval)) {
    db_put(serialkey,"name",a->name);
    db_put(serialkey,"arch",a->arch);
    db_put(serialkey,"cmdsum",a->cmdsum);
    char *serial;
    // 2) Fetch the serial record, if it exists.
    if (db_fetch(serialkey,serialval))
      serial=db_get(serialval,"serial");
    else
      serial=str_dup("0");
    str_inc(&serial);
    db_put(serialval,"serial",serial);
    // 3) Store the updated serial record.
    db_store(serialkey,serialval);
    db_put(chksumval,"serial",serial);
    // 4) Store the chksum record.
    db_store(chksumkey,chksumval);
    db_put(cachedkey,"name",a->name);
    db_put(cachedkey,"arch",a->arch);
    db_put(cachedkey,"cmdsum",a->cmdsum);
    db_put(cachedkey,"serial",serial);
    db_put(cachedval,"date",str_date_yymmdd());
    db_put(cachedval,"time",str_time_hhmmss());
    db_put(cachedval,"cachedir",a->cachedir);
    // 5) Store the cachedir record.
    db_store(cachedkey,cachedval);
    free(serial);
    result="success";
    count="1";
  }
  // 6) Write count to socket.
  fprintf(a->fpp->w,"%s\n",count);
  fflush(a->fpp->w);
  FREE_RECORDS;
  return result;
}

/***************************************************************************
 * This function allocates, zeroes, and returns a                          *
 * Command/Arguments structure.                                            *
 ***************************************************************************/
extern Command *command_new() {
  Command *command=(Command *)calloc(1,sizeof(*command));
  if (!command)
    ERR("command_new(): malloc() failed");
  Arguments *arguments=(Arguments *)calloc(1,sizeof(*arguments));
  if (!arguments)
    ERR("command_new(): malloc() failed");
  command->arguments=arguments;
  return command;
}

/***************************************************************************
 * This function free()-s the freeable fields in a                         *
 * Command/Arguments structure.                                            *
 *                                                                         *
 * Arguments:                                                              *
 *     command = Command/Arguments structure                               *
 ***************************************************************************/
extern void command_free(Command *command) {
  void nzfree(void *p) { if (p) free(p); }
  nzfree(command->op);
  nzfree(command->arguments->name);
  nzfree(command->arguments->arch);
  nzfree(command->arguments->chksum);
  nzfree(command->arguments->cmdsum);
  nzfree(command->arguments->cachedir);
  fpp_close(command->arguments->fpp);
  free(command->arguments);
  free(command);
}

/***************************************************************************
 * This function executes a get or put command, in a new                   *
 * thread. The new thread blocks all signals. A command                    *
 * locks/unlocks according to the readers/writers protocol.                *
 *                                                                         *
 * Arguments:                                                              *
 *      command = functions and arguments                                  *
 ***************************************************************************/
extern void command_do(Command *command) {
  void *f(void *a) {
    Command *command=(Command *)a;
    command->lock();
    if (opt_report())
      rwl_report();
    if (opt_sleep()>0)
      sleep(opt_sleep());
    command->func(command->arguments);
    command->unlock();
    command_free(command);
    return 0;
  }
  pthread_t thread;
  sigset_t new;
  sigset_t old;
  sigfillset(&new);
  pthread_sigmask(SIG_BLOCK,&new,&old);
  if (pthread_create(&thread,0,f,command))
    ERR("command_do(): pthread_create() failed: %s",strerror(errno));
  pthread_sigmask(SIG_SETMASK,&old,0);
}
