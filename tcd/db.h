/***************************************************************************
 * I'm trying to achieve a sort of Bridge(151) pattern.                    *
 ***************************************************************************/

#ifndef DB_H
#define DB_H

/***************************************************************************
 * This is a function declaration.                                         *
 ***************************************************************************/
#define DB__DCL(r,n,a) extern r db_##n(a)
#define DB_DCL(n,m...) DB__DCL(DB_##n##_RETV,m##n,DB_##n##_PRMS)

/***************************************************************************
 * This is a function-pointer definition, for a vtable.                    *
 ***************************************************************************/
#define DB__PTR(r,n,a) r (*n)(a)
#define DB_PTR(n,m...) DB__PTR(DB_##n##_RETV,m##n,DB_##n##_PRMS)

/***************************************************************************
 * These are the return type, formal parameters, and actual                *
 * paramters for a function declaration/call.                              *
 ***************************************************************************/

#define DB_open_RETV int
#define DB_open_PRMS
#define DB_open_ARGS

#define DB_close_RETV int
#define DB_close_PRMS
#define DB_close_ARGS

#define DB_get_RETV char**
#define DB_get_PRMS char*name,char*arch,char*cmdsum
#define DB_get_ARGS name,arch,cmdsum

#define DB_put_RETV int
#define DB_put_PRMS char*date,char*time,char*name,char*arch,char*chksum,char*cmdsum,char*cachedir
#define DB_put_ARGS date,time,name,arch,chksum,cmdsum,cachedir

#define DB_log_RETV int
#define DB_log_PRMS char*op,char*date,char*time,char*name,char*cwd,char*result
#define DB_log_ARGS op,date,time,name,cwd,result

/***************************************************************************
 * These are the function declarations.                                    *
 ***************************************************************************/

DB_DCL(open);
DB_DCL(close);
DB_DCL(get);
DB_DCL(put);
DB_DCL(log);

#endif
