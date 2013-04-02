#ifndef OPTIONS_H
#define OPTIONS_H

extern void options_usage();
extern void options_init(int argc, char *argv[]);
extern int opt_bindtries();
extern char* opt_ctl_file();
extern int opt_ctl_time();
extern int opt_daemon();
extern char* opt_db_file();
extern int opt_debug();
extern char* opt_err_file();
extern int opt_help();
extern int opt_port();
extern int opt_queue();
extern int opt_report();
extern int opt_sleep();
extern int opt_timeout();

#endif
