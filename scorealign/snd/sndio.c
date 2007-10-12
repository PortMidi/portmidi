/* sndio.c -- read/write sound file data */

/* CHANGELOG
 *
 * 29Jun95  RBD  ULAW fixed problems with signed chars
 */

/* Standard includes */
#include "stdio.h"
#include "memory.h"

#ifndef mips
#include "stdlib.h"
#endif
#include "snd.h"
#ifdef LINUX
#include "sys/file.h"
#else
/* #include <unistd.h> */
#ifdef WIN32
#include <sys/stat.h>
#include "io.h"
#elif defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/stat.h>
#else
#include <stat.h>
#endif
#endif
//#include "audio.h"

/* was "#ifndef WIN32" */
#if !defined(WIN32) && !defined(IRIX) && !defined(SGI)
void _swab(char *to, char *from, long length)
{
    short *to16 = (short *) to;
    short *from16 = (short *) from;
    int i = 1;
    while (i < length) {
        short data = *from16++;
        *to16++ = (data << 8) | ((data >> 8) & 0xFF);
        i += 2;
    }
}
#endif




