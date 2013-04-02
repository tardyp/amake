#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "ctl.h"
#include "options.h"
#include "rwl.h"
#include "db.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

/***************************************************************************
 * These function implement the ctl-file commands.                         *
 ***************************************************************************/

static void ctl_delete() {
  DEBUG("ctl_delete()");
  char *old=opt_db_file();
  if (!isreg(old))
    return;
  if (unlink(old))
    ERR("ctl_delete(): unlink() failed");
}

static void ctl_copy() {
  DEBUG("ctl_copy()");
  char *old=opt_db_file();
  if (!isreg(old))
    return;
  char *new=vstrcat(old,".",str_date_yymmdd(),".",str_time_hhmmss(),NULL);
  if (!fcp(old,new))
    ERR("ctl_copy(): copy failed");
  free(new);
}

static void ctl_move() {
  DEBUG("ctl_move()");
  char *old=opt_db_file();
  if (!isreg(old))
    return;
  char *new=vstrcat(old,".",str_date_yymmdd(),".",str_time_hhmmss(),NULL);
  if (rename(old,new))
    ERR("ctl_move(): rename() failed: \"%s\"",strerror(errno));
  free(new);
}

/***************************************************************************
 * This function is called periodically to process a ctl                   *
 * file. If the file exists a line is read from it and the file            *
 * is removed. The line should be a command.                               *
 ***************************************************************************/
extern void process_ctl() {
  char *command=freadfirstln(opt_ctl_file());
  if (!command)
    return;
  DEBUG("process_ctl(): command=\"%s\"",command);
  rwl_put_lock();
  db_close();
  if (!strcmp(command,"delete"))
    ctl_delete();
  if (!strcmp(command,"copy"))
    ctl_copy();
  if (!strcmp(command,"move"))
    ctl_move();
  if (!strcmp(command,"exit")) {
    free(command);
    unlink(opt_ctl_file());
    DEBUG("ctl_exit()");
    exit(0);
  }
  free(command);
  db_open(RW);
  rwl_put_unlock();
  unlink(opt_ctl_file());
}
