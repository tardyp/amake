#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>

#include "chksum.h"
#include "fileutils.h"
#include "err.h"

static int initialized=0;

/***************************************************************************
 * This function converts a SHA1 message-digest string to a                *
 * 40-character string. It returns the string, which can be                *
 * free()-ed.                                                              *
 *                                                                         *
 * Arguments:                                                              *
 *     name = name of file to checksum                                     *
 ***************************************************************************/
static char *md_hex(unsigned char *md_string) {
    int len=gcry_md_get_algo_dlen(GCRY_MD_SHA1);
    char *s=(char *)malloc((len<<1)+1);
    if (!s)
      ERR("md_hex(): malloc() failed");
    int i;
    char *p;
    for (i=0, p=s; i<len; i++, p+=2)
        snprintf(p,3,"%02x",md_string[i]);
    *p=0;
    return s;
}

/***************************************************************************
 * This function computes the SHA1 checksum of a string. It                *
 * returns the checksum as a 40-character string, which can be             *
 * free()-ed, or zero for error.                                           *
 *                                                                         *
 * Arguments:                                                              *
 *     s = string to checksum                                              *
 ***************************************************************************/
extern char *chksum(const char *s) {
  if (!initialized) {
    gcry_control(GCRYCTL_DISABLE_SECMEM,0);
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED,0);
    initialized=1;
  }
  gcry_md_hd_t digest;
  gcry_error_t err=gcry_md_open(&digest,GCRY_MD_SHA1,0);
  if (err)
    ERR("chksum(): gcry_md_open() failed %s",gcry_strerror(err));
  gcry_md_write(digest,s,strlen(s));
  unsigned char *md_string=gcry_md_read(digest,0);
  char *t=md_hex(md_string);
  gcry_md_close(digest);
  return t;
}

/***************************************************************************
 * This function computes the SHA1 checksum of the content of a            *
 * file. It returns the checksum as a 40-character string,                 *
 * which can be free()-ed, or zero for error.                              *
 *                                                                         *
 * Arguments:                                                              *
 *     name = name of file to checksum                                     *
 ***************************************************************************/
extern char *chksumfile(const char *name) {
  if (isdir(name))
    return 0;
  FILE *f=fopen(name,"r");
  if (!f)
    return 0;
  if (!initialized) {
    gcry_control(GCRYCTL_DISABLE_SECMEM,0);
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED,0);
    initialized=1;
  }
  gcry_md_hd_t digest;
  gcry_error_t err=gcry_md_open(&digest,GCRY_MD_SHA1,0);
  if (err)
    ERR("chksumfile(): gcry_md_open() failed %s",gcry_strerror(err));
  const size_t bufsize=4096;
  unsigned char buffer[bufsize];
  size_t num;
  while ((num=fread(buffer,1,bufsize,f)))
    gcry_md_write(digest,buffer,num);
  unsigned char *md_string=gcry_md_read(digest,0);
  char *s=md_hex(md_string);
  gcry_md_close(digest);
  fclose(f);
  return s;
}
