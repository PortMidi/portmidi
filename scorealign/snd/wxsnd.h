/*
 * Header file for wxWindows port of snd library
 *
 * Dominic Mazzoni
 */

#ifndef _WX_SND_
#define _WX_SND_

#undef WIN32

#ifdef __WXGTK__
    #include <netinet/in.h>
#endif

#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define PROTECTION 
#define off_t int

#define open(X,Y) wxsnd_open(X,Y)
#define creat(X,Y) wxsnd_creat(X,Y)
#define lseek(X,Y,Z) wxsnd_lseek(X,Y,Z)
#define read(X,Y,Z) wxsnd_read(X,Y,Z)
#define write(X,Y,Z) wxsnd_write(X,Y,Z)
#define close(X) wxsnd_close(X)
#define getfilelen(X) wxsnd_getfilelen(X)

enum {O_RDONLY, O_RDWR};

#ifdef __cplusplus
extern "C" {
#endif

int wxsnd_open(char *fname, int mode);
int wxsnd_creat(char *fname, int perms);
int wxsnd_lseek(int file, int offset, int param);
int wxsnd_read(int fp, char *data, int len);
int wxsnd_write(int fp, char *data, int len);
int wxsnd_close(int fp);
long wxsnd_getfilelen(int fp);

#ifdef __cplusplus
}
#endif

#endif
