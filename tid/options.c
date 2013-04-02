#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "str.h"

static char *name="";

typedef struct {
  char *def; 
  char *str;
  union {
    int i;
    char *s;
  } val;
} Option;

static struct {
  Option bindtries; // (-b) <integer> max bind() tries [5]
  Option ctl_file;  // (-c) <string> file for ctl commands [/tmp/ctl]
  Option ctl_time;  // (-p) <integer> frequency to poll ctl file (in secs) [5]
  Option daemon;    // (-f) <boolean> fork as daemon [true]
  Option db_file;   // (-g) <string> gdbm database file [/tmp/db]
  Option debug;     // (-d) <boolean> write debug messages [false]
  Option err_file;  // (-e) <string> file for debug/error messages [/tmp/err]
  Option help;      // (-h) <counter> print usage message [0]
  Option port;      // (-P) <integer> port number [7777]
  Option queue;     // (-q) <integer> putter-preference queue length [5]
  Option report;    // (-r) <boolean> report lock contention [false]
  Option sleep;     // (-s) <integer> sleep after lock acquisition (in secs) [0]
  Option timeout;   // (-t) <integer> timeout for socket fd [5]
} options;

static char *usage_bindtries() {
  char *def=options.bindtries.def;
  char *str=options.bindtries.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_ctl_file() {
  char *def=options.ctl_file.def;
  char *str=options.ctl_file.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_ctl_time() {
  char *def=options.ctl_time.def;
  char *str=options.ctl_time.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_daemon() {
  char *def=options.daemon.def;
  char *str=options.daemon.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_db_file() {
  char *def=options.db_file.def;
  char *str=options.db_file.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_debug() {
  char *def=options.debug.def;
  char *str=options.debug.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_err_file() {
  char *def=options.err_file.def;
  char *str=options.err_file.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_help() {
  char *def=options.help.def;
  char *str=options.help.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_port() {
  char *def=options.port.def;
  char *str=options.port.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_queue() {
  char *def=options.queue.def;
  char *str=options.queue.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_report() {
  char *def=options.report.def;
  char *str=options.report.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_sleep() {
  char *def=options.sleep.def;
  char *str=options.sleep.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}

static char *usage_timeout() {
  char *def=options.timeout.def;
  char *str=options.timeout.str;
  return (str
    ? vstrcat("[",str,"[",def,"]","]",NULL)
    : vstrcat("[",def,"]",NULL)
  );
}


extern void options_usage() {
  printf("usage: %s <options>\n",name);
  printf("  --bindtries (-b) <integer>\n");
  printf("    max bind() tries                         %s\n",usage_bindtries());
  printf("  --ctl_file (-c) <string>\n");
  printf("    file for ctl commands                    %s\n",usage_ctl_file());
  printf("  --ctl_time (-p) <integer>\n");
  printf("    frequency to poll ctl file (in secs)     %s\n",usage_ctl_time());
  printf("  --daemon (-f) <boolean>\n");
  printf("    fork as daemon                           %s\n",usage_daemon());
  printf("  --db_file (-g) <string>\n");
  printf("    gdbm database file                       %s\n",usage_db_file());
  printf("  --debug (-d) <boolean>\n");
  printf("    write debug messages                     %s\n",usage_debug());
  printf("  --err_file (-e) <string>\n");
  printf("    file for debug/error messages            %s\n",usage_err_file());
  printf("  --help (-h) \n");
  printf("    print usage message                      %s\n",usage_help());
  printf("  --port (-P) <integer>\n");
  printf("    port number                              %s\n",usage_port());
  printf("  --queue (-q) <integer>\n");
  printf("    putter-preference queue length           %s\n",usage_queue());
  printf("  --report (-r) <boolean>\n");
  printf("    report lock contention                   %s\n",usage_report());
  printf("  --sleep (-s) <integer>\n");
  printf("    sleep after lock acquisition (in secs)   %s\n",usage_sleep());
  printf("  --timeout (-t) <integer>\n");
  printf("    timeout for socket fd                    %s\n",usage_timeout());
  exit(1);
}

#include "options_get.h"

extern void options_init(int argc, char *argv[]) {
  name=argv[0];
  options.bindtries.def="5";
  options.bindtries.str=0;
  options.bindtries.val.i=5;
  options.ctl_file.def="/tmp/ctl";
  options.ctl_file.str=0;
  options.ctl_file.val.s="/tmp/ctl";
  options.ctl_time.def="5";
  options.ctl_time.str=0;
  options.ctl_time.val.i=5;
  options.daemon.def="true";
  options.daemon.str=0;
  options.daemon.val.i=1;
  options.db_file.def="/tmp/db";
  options.db_file.str=0;
  options.db_file.val.s="/tmp/db";
  options.debug.def="false";
  options.debug.str=0;
  options.debug.val.i=0;
  options.err_file.def="/tmp/err";
  options.err_file.str=0;
  options.err_file.val.s="/tmp/err";
  options.help.def="0";
  options.help.str=0;
  options.help.val.i=0;
  options.port.def="7777";
  options.port.str=0;
  options.port.val.i=7777;
  options.queue.def="5";
  options.queue.str=0;
  options.queue.val.i=5;
  options.report.def="false";
  options.report.str=0;
  options.report.val.i=0;
  options.sleep.def="0";
  options.sleep.str=0;
  options.sleep.val.i=0;
  options.timeout.def="5";
  options.timeout.str=0;
  options.timeout.val.i=5;
  int i=1;
  while (i<argc) {
    if (get_integer(&i,argc,argv,"--bindtries","-b",&options.bindtries)) continue;
    if (get_string(&i,argc,argv,"--ctl_file","-c",&options.ctl_file)) continue;
    if (get_integer(&i,argc,argv,"--ctl_time","-p",&options.ctl_time)) continue;
    if (get_boolean(&i,argc,argv,"--daemon","-f",&options.daemon)) continue;
    if (get_string(&i,argc,argv,"--db_file","-g",&options.db_file)) continue;
    if (get_boolean(&i,argc,argv,"--debug","-d",&options.debug)) continue;
    if (get_string(&i,argc,argv,"--err_file","-e",&options.err_file)) continue;
    if (get_counter(&i,argc,argv,"--help","-h",&options.help)) continue;
    if (get_integer(&i,argc,argv,"--port","-P",&options.port)) continue;
    if (get_integer(&i,argc,argv,"--queue","-q",&options.queue)) continue;
    if (get_boolean(&i,argc,argv,"--report","-r",&options.report)) continue;
    if (get_integer(&i,argc,argv,"--sleep","-s",&options.sleep)) continue;
    if (get_integer(&i,argc,argv,"--timeout","-t",&options.timeout)) continue;
    options_usage();
  }
}

extern int opt_bindtries() {
  return options.bindtries.val.i;
}

extern char* opt_ctl_file() {
  return options.ctl_file.val.s;
}

extern int opt_ctl_time() {
  return options.ctl_time.val.i;
}

extern int opt_daemon() {
  return options.daemon.val.i;
}

extern char* opt_db_file() {
  return options.db_file.val.s;
}

extern int opt_debug() {
  return options.debug.val.i;
}

extern char* opt_err_file() {
  return options.err_file.val.s;
}

extern int opt_help() {
  return options.help.val.i;
}

extern int opt_port() {
  return options.port.val.i;
}

extern int opt_queue() {
  return options.queue.val.i;
}

extern int opt_report() {
  return options.report.val.i;
}

extern int opt_sleep() {
  return options.sleep.val.i;
}

extern int opt_timeout() {
  return options.timeout.val.i;
}


