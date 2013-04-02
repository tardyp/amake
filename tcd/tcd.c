#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "fpp.h"
#include "command.h"
#include "db.h"
#include "options.h"
#include "getline.h"
#include "fileutils.h"
#include "str.h"
#include "err.h"

/***************************************************************************
 * This function creates a socket for the server to receive                *
 * client requests. It return a file descriptor for the socket,            *
 * or zero for failure.                                                    *
 ***************************************************************************/
static int init() {
  struct sockaddr_un addr;
  int fd=socket(PF_LOCAL,SOCK_STREAM,0);
  if (fd<0)
    ERR("init(): socket() failed");
  int yes=1;
  if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)))
    ERR("init(): setsockopt() failed");
  addr.sun_family=AF_LOCAL;
  strncpy(addr.sun_path,opt_socket(),sizeof(addr.sun_path));
  addr.sun_path[sizeof(addr.sun_path)-1]=0;
  if (bind(fd,(struct sockaddr *)&addr,SUN_LEN(&addr))<0) {
    close(fd);
    fd=0;
    ERR("init(): bind() failed: %s",strerror(errno));
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
 * writes a result string to the socket.                                   *
 *                                                                         *
 * Arguments:                                                              *
 *     fd = client-service socket                                          *
 ***************************************************************************/
static void serve(int fd) {
  jmp_buf env;		/* setjmp/longjmp */
  void onalarm(int signo) {
    DEBUG("serve(): client timed out, pid=%d",getpid());
    longjmp(env,1);
  }
  FPP* fpp=fpp_open(fd);
  db_open();
  signal(SIGALRM,onalarm);
  alarm(opt_timeout());
  if (!setjmp(env)) {
    char *command=GETLINE(fpp->r);
    DEBUG("serve(): command=\"%s\"",command);
    char *result=command_do(fpp,command);
    DEBUG("serve(): result=\"%s\"",result);
    fprintf(fpp->w,"%s\n",result);
    fflush(fpp->w);
    free(command);
  }
  signal(SIGALRM,SIG_IGN);
  alarm(0);
  fpp_close(fpp);
  db_close();
}

/***************************************************************************
 * This function services a socket-based client request. It                *
 * uses SIGALRM to exit after a certain amount of idle time,               *
 * specified by a command-line option. When it receives a                  *
 * request, it forks a process to service it. The parent                   *
 * returns to the calling function, after possibly reaping                 *
 * previously forked children.                                             *
 *                                                                         *
 * Arguments:                                                              *
 *     fd = client-request socket                                          *
 ***************************************************************************/
static void server(int fd) {
  void onalarm(int signo) {
    while (waitpid(WAIT_ANY,0,0)>0); /* for all children */
    close(fd);
    unlink(opt_socket());
    DEBUG("server(): client idled out, pid=%d",getpid());
    exit(0);
  }
  signal(SIGALRM,onalarm);
  alarm(opt_idle());
  int cfd=accept(fd,0,0);
  signal(SIGALRM,SIG_IGN);
  alarm(0);
  if (cfd<0)
    ERR("server(): accept() failed");
  if (opt_daemon()) {
    DEBUG("server(): fork()-ing, pid=%d",getpid());
    if (fork()) {		/* parent */
      close(cfd);
    } else {			/* child */
      DEBUG("server(): fork()-ed, pid=%d",getpid());
      serve(cfd);
      exit(0);
    }
  } else {
    serve(cfd);
  }
  while (waitpid(WAIT_ANY,0,WNOHANG)>0); /* for any exited children */
}
  
/***************************************************************************
 * This is the main function. It processes command-line                    *
 * options, does the Unix server thing, opens the client/server            *
 * socket, connects to the database, and repeately services                *
 * client requests.                                                        *
 *                                                                         *
 * This program isn't intended to be a long-running server,                *
 * like sshd. It exits after a certain amount of idle time. A              *
 * client is expected to start the server, as needed.                      *
 ***************************************************************************/
int main(int argc, char *argv[]) {
  options_init(argc,argv);
  if (opt_help())
    options_usage();
  umask(0);
  close(0); stdin=0;
  close(1); stdout=0;
  close(2); stderr=0;
  if (opt_daemon()) {
    DEBUG("main(): fork()-ing, pid=%d",getpid());
    if (fork())
      exit(0);
    DEBUG("main(): fork()-ed, pid=%d",getpid());
    setsid();
    if (chdir("/")<0)
      ERR("main(): chdir() failed");
  }
  int fd=init();
  while (1)
    server(fd);
}
