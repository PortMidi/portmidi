/* see sys/switches.h.template */

/* CHANGE LOG
 * --------------------------------------------------------------------
 * 28Apr03  dm  major reorganization of conditional compilation in Nyquist
 */

#define HAS_STDLIB_H 1
#define HAS_SYS_TYPES_H 1
#define HAS_SYS_STAT_H 1
#undef HAS_STAT_H
#undef HAS_MALLOC_H

#define HAS_GETTIMEOFDAY 1

// I think that READ_LINE prevents user from typing control characters to
// get info during lisp execution. This needs to be tested. Using READ_LINE
// is preventing any character echoing now, maybe due to new "improved"
// command line handling added recently. -RBD

// #define READ_LINE 1

/* this is defined in xlisp.h - RBD
#if i386
#define XL_LITTLE_ENDIAN 1
#elif __i386__
#define XL_LITTLE_ENDIAN 1
#else
#define XL_BIG_ENDIAN 1
#endif
*/

#undef USE_RAND
#define USE_RANDOM 1

/* define this to be printf, or define your own fn of the form
     void nyquist_printf(char *format, ...);
   (for a GUI)
*/
#define nyquist_printf printf

#if __APPLE__ && __GNUC__ /* Mac OS X */
#define NEED_ULONG 1
#else
#include <sys/types.h>
#undef NEED_ULONG
#endif

#undef NEED_USHORT
#define NEED_BYTE 1

#define NEED_ROUND 1

#undef NEED_DEFINE_MALLOC

/* explicitly choose a platform */
#define UNIX 1
#undef WINDOWS
#undef MICROSOFT
#undef DOS
#undef MACINTOSH

#define BUFFERED_SYNCHRONOUS_INPUT 1
#define SPACE_FOR_PLAY 10000
#define MAX_CHANNELS 16

/* this will enable code to read midi files, etc. */
#define CMTSTUFF 1

/* NYQUIST tells some CMT code that we're really in
 * XLISP and NYQUIST
 */
#define NYQUIST 1

#include "swlogic.h"

