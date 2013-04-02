#ifndef DEFAULTS_H
#define DEFAULTS_H

#define DEF_ACCESS_DEBUG         ""
#define DEF_ACCESS_SHLIBS        ""
#define DEF_ACCESS_ARCH          str_arch()
#define DEF_ACCESS_TMP           "/tmp"
#define DEF_ACCESS_PROGRAM       "amake-collect"
#define DEF_ACCESS_LIBRARY       "libaccess.so"
#define DEF_ACCESS_ENVVARS       "PATH LD_LIBRARY_PATH"
#define DEF_ACCESS_CACHE_DAEMON  "tcd"
#define DEF_ACCESS_CACHE_DIR     ""
#define DEF_ACCESS_CACHE_TIMEOUT "10"
#define DEF_ACCESS_CACHE_TRIES   "5"
#define DEF_ACCESS_CACHE_DBMGR   "tid"

#define DEF_ACCESS_DIR           ""
#define DEF_ACCESS_CEXT          "cmd"
#define DEF_ACCESS_DEXT          "dep"
#define DEF_ACCESS_SEXT          "sib"
#define DEF_ACCESS_OEXT          "old"

#define DEF_ACCESS_PASS          ""
#define DEF_ACCESS_FAIL          ""
#define DEF_ACCESS_CHKSUM        ""
#define DEF_ACCESS_LMTIME        ""

#ifndef EVPREFIX
#define EVPREFIX ACCESS
#endif

// cpp tricks!

#define ENV2(v,p) #p "_" #v
#define ENV1(v,p) ENV2(v,p)
#define ENV(v)    ENV1(v,EVPREFIX)

#define DEF1(v,p) DEF_##p##_##v
#define DEF(v,p)  DEF1(v,p)

#define GETVAR(v) getvar(ENV(v),DEF(v,EVPREFIX),file)

#endif
