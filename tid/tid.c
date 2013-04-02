#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gdbm.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "fpp.h"
#include "rwl.h"
#include "command.h"
#include "db.h"
#include "ctl.h"
#include "options.h"
#include "gets.h"
#include "fileutils.h"
#include "err.h"

/***************************************************************************
 * This function creates a socket for the server to receive                *
 * client requests. It returns a file descriptor for the socket,           *
 * or zero for failure.                                                    *
 ***************************************************************************/
static int init() {
  struct sockaddr_in addr;
  int fd=socket(PF_INET,SOCK_STREAM,0);
  if (fd<0)
    ERR("init(): socket() failed");
  int yes=1;
  if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)))
    ERR("init(): setsockopt() failed");
  bzero(&addr,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htons(INADDR_ANY);
  addr.sin_port=htons(opt_port());
  int tries=0;
  while (bind(fd,(struct sockaddr *)&addr,sizeof(addr))<0) {
    if (++tries>opt_bindtries())
      ERR("init(): bind() failed: %s",strerror(errno));
    sleep(1<<tries);
  }
  if (listen(fd,SOMAXCONN)<0)
    ERR("init(): listen() failed");
  return fd;
}

/***************************************************************************
 * This function services a socket-based client request. The               *
 * socket is for a single client's single request. The function            *
 * reads a command string from the socket, performs the command            *
 * (which may require reading/writing from/to the socket), and             *
 * writes a result string to the socket.  If the alarm expires             *
 * while servicing a client request, the request is aborted.               *
 *                                                                         *
 * Arguments:                                                              *
 *     fd = client-service socket                                          *
 ***************************************************************************/
static void serve(int fd) {
  Command *command=command_new();
  FPP* fpp=fpp_open(fd);
  command->arguments->fpp=fpp;
  jmp_buf env;
  if (!setjmp(env)) {
    GETS(&command->op,fpp->r,env);
    DEBUG("serve(): command=\"%s\"",command->op);
    if (!strcmp(command->op,"get")) {
      GETS(&command->arguments->name,fpp->r,env);
      GETS(&command->arguments->arch,fpp->r,env);
      GETS(&command->arguments->cmdsum,fpp->r,env);
      command->func=command_get;
      command->lock=rwl_get_lock;
      command->unlock=rwl_get_unlock;
      command_do(command);
    } else if (!strcmp(command->op,"put")) {
      GETS(&command->arguments->name,fpp->r,env);
      GETS(&command->arguments->arch,fpp->r,env);
      GETS(&command->arguments->chksum,fpp->r,env);
      GETS(&command->arguments->cmdsum,fpp->r,env);
      GETS(&command->arguments->cachedir,fpp->r,env);
      command->func=command_put;
      command->lock=rwl_put_lock;
      command->unlock=rwl_put_unlock;
      command_do(command);
    } else
      LOG("serve(): unknown command: \"%s\"",command->op);
  } else {
    LOG("serve(): client timed out: fd=%d",fd);
    command_free(command);
  }
}

/***************************************************************************
 * This function selects on the socket and services client                 *
 * requests. If idle, after the timeout, the control file is               *
 * checked for commands.                                                   *
 ***************************************************************************/
static void server(int fd) {
  void onterm(int signo) {
    LOG("server(): signal=%d",signo);
    rwl_put_lock();
    db_close();
    LOG("server(): exiting");
    exit(0);
  }
  signal(SIGHUP,onterm);
  signal(SIGTERM,onterm);
  struct timeval timeout;
  timeout.tv_sec=opt_ctl_time();
  timeout.tv_usec=0;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd,&fds);
  int count=select(FD_SETSIZE,&fds,0,0,&timeout);
  if (count<0)
    ERR("server(): select() failed");
  if (count==0)
    process_ctl();
  if (count==1 && FD_ISSET(fd,&fds)) {
    int cfd=accept(fd,0,0);
    if (cfd<0)
      ERR("server(): accept() failed");
    serve(cfd);
  }
}
  
/***************************************************************************
 * This is the main function. It processes command-line                    *
 * options, opens the error file, does the Unix server thing,              *
 * opens the client/server socket, and repeately services                  *
 * client requests.                                                        *
 ***************************************************************************/
int main(int argc, char *argv[]) {
  options_init(argc,argv);
  if (opt_help())
    options_usage();
  stderr=freopen(opt_err_file(),"w",stderr);
  if (!stderr)
    ERR("main(): freopen() failed");
  DEBUG("main(): \"%s\" opened",opt_err_file());
  if (opt_daemon()) {
    DEBUG("main(): fork()-ing, pid=%d",getpid());
    if (fork())
      exit(0);
    DEBUG("main(): fork()-ed, pid=%d",getpid());
    setsid();
    chdir("/");
    umask(0);
    close(0);
    close(1);
  }
  rwl_init();
  db_open(RW);
  int fd=init();
  while (1)
    server(fd);
}
