cachedir : c : string  : /tmp/tcd : cache directory
debug    : d : boolean : false    : write debug messages
errfile  : e : string  : tcd.err  : file for debug/error messages
daemon   : f : boolean : false    : fork as daemon
help     : h : counter : 0        : print usage message
dblog    : l : boolean : false    : log database operations
dbmgr    : m : string  : tid      : database manager
dbname   : n : string  : tcd      : database name
socket   : s : string  : tcd.0    : client/server socket
timeout  : t : integer : 60       : timeout for read()
idle     : i : integer : 60       : timeout for accept() (0=never)

myd_user   : U : string  : tcd       : mysql database user

tid_host   : H : string  : localhost : tid host name
tid_port   : P : integer : 7777      : tid port number
