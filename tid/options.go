db_file   : g : string  : /tmp/db  : gdbm database file
debug     : d : boolean : false    : write debug messages
err_file  : e : string  : /tmp/err : file for debug/error messages
daemon    : f : boolean : true     : fork as daemon
help      : h : counter : 0        : print usage message
ctl_file  : c : string  : /tmp/ctl : file for ctl commands
ctl_time  : p : integer : 5        : frequency to poll ctl file (in secs)
port      : P : integer : 7777     : port number
timeout   : t : integer : 5        : timeout for socket fd
bindtries : b : integer : 5        : max bind() tries
report    : r : boolean : false    : report lock contention
sleep     : s : integer : 0        : sleep after lock acquisition (in secs)
queue     : q : integer : 5        : putter-preference queue length
