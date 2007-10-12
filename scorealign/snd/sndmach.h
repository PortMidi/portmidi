/* sndmach.h -- system-specific definitions for OS X */

#ifndef OSX
#define OSX
#endif

typedef double FASTFLOAT;
typedef float MEMFLOAT;

/* avoid conflicts if already defined: */
#ifndef max

/* min(n, sizeof(long)) doesn't work on RS6K without this: 
 * (I never tracked down what min() was called and what was wrong.)
 */
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif
