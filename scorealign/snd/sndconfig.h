// #include <stdio.h> // for size_t

/* sndconfig.h -- system-specific definitions */
/* 
  NOTES: each implementation must have its own .h and .c files,
  e.g. sndwin32.h and sndwin32.c. These files implement the following:

1) Either LINUX, WIN32, or MACINTOSH should be defined.
(WX can also be defined to override standard file IO with functions
from WX, a graphical user interface library.)

2) The following function declaration:
    void snd_fail(char *msg);
or
    #define snd_fail some_function
(note that snd_fail must be a function pointer)

3) typedef FASTFLOAT to be either a double or a float, whichever
computes faster (PowerPCs are faster at double math than float math)

4) typedef MEMFLOAT to be how you would store a sample in memory
(this should normally be float)

5) min() must be defined (either a macro or a function)

6) max() must be defined (either a macro or a function)

7) the following file IO functions must be defined:
    int snd_file_open(char *fname, int mode);
    int snd_file_creat(char *fname);
    int snd_file_lseek(int file, int offset, int param);
    long snd_file_read(int fp, char *data, long len);
    long snd_file_write(int fp, char *data, long len);
    int snd_file_close(int fp);
    long snd_file_len(int fp);
(some implementations cast a pointer to an int in snd_open,
so it is assumed that int can at least hold a pointer)
NOTE: snd_file_creat and snd_file_open return SND_FILE_FAILURE
if the open fails.

8) The following function to report failure and exit:
    void snd_fail(char *msg);

9) The following memory allocation routines:
    void *snd_alloc(size_t s);
    void snd_free(void *a);

10) snd_string_max -- string length for filenames, etc.

*/

#define snd_string_max 258

void snd_fail(char *msg);
void snd_warn(char *msg);
void *snd_alloc(size_t s);
void snd_free(void *a);

#if defined(__linux__)
  #include "sndlinux.h"

#elif defined(IRIX)
  #include "sndirix.h"

#elif defined(__FreeBSD__)
  #include "sndfreebsd.h"

#elif defined(_WIN32)
  #ifndef WIN32
    #define WIN32
  #endif
  #include "sndwin32.h"

#elif defined(macintosh) || defined(__WXMAC__)
  #ifndef MAC
    #define MAC
  #endif
  #include "sndmac.h"

#elif defined(__MACH__)
  #include "sndmach.h"
#endif
