/*
 * snd.h
 */
/*
 * based on sndheader.h, written by
 *      Jim Zelenka, CMU/ITC
 *      rewritten 9 Jun 1992 (from my original sources,
 *       adding many new constants)
 * converted by Roger Dannenberg to snd.h, 21 Jun 97
 */
#ifdef SND_H
error here
#endif
#define SND_H

/* Standard includes */
#include <stdlib.h>

#include "sndconfig.h"
#ifdef PORTAUDIO
#include "pablio.h"
#endif

/* header formats */
#define SND_HEAD_NONE 0
/* LISP-SRC: (setf snd-head-none 0) */
#define SND_HEAD_AIFF 1
/* LISP-SRC: (setf snd-head-AIFF 1) */
#define SND_HEAD_IRCAM 2
/* LISP-SRC: (setf snd-head-IRCAM 2) */
#define SND_HEAD_NEXT 3
/* LISP-SRC: (setf snd-head-NeXT 3) */
#define SND_HEAD_WAVE 4
/* LISP-SRC: (setf snd-head-Wave 4) */
#define SND_NUM_HEADS 5

/* bitfields for soundheaders */
#define SND_HEAD_CHANNELS (1<<0)
/* LISP-SRC: (setf snd-head-channels 1) */
#define SND_HEAD_MODE (1<<1)
/* LISP-SRC: (setf snd-head-mode 2) */
#define SND_HEAD_BITS (1<<2)
/* LISP-SRC: (setf snd-head-bits 4) */
#define SND_HEAD_SRATE (1<<3)
/* LISP-SRC: (setf snd-head-srate 8) */

/* when returned from lisp, len (samples) is converted to time (seconds) */
#define SND_HEAD_LEN (1<<4)
/* LISP-SRC: (setf snd-head-dur 16) */

#define SND_HEAD_LATENCY (1<<5)
/* LISP-SRC: (setf snd-head-latency 32) */
#define SND_HEAD_TYPE (1<<6)
/* LISP-SRC: (setf snd-head-type 64) */

/* modes */
#define SND_MODE_ADPCM 0
/* LISP-SRC: (setf snd-mode-adpcm 0) */
#define SND_MODE_PCM   1
/* LISP-SRC: (setf snd-mode-pcm 1) */
#define SND_MODE_ULAW 2
/* LISP-SRC: (setf snd-mode-ulaw 2) */
#define SND_MODE_ALAW  3
/* LISP-SRC: (setf snd-mode-alaw 3) */
#define SND_MODE_FLOAT 4
/* LISP-SRC: (setf snd-mode-float 4) */
/* unsigned pcm (e.g. Microsoft 8-bit wav format): */
#define SND_MODE_UPCM 5
/* LISP-SRC: (setf snd-mode-upcm 5) */
#define SND_MODE_UNKNOWN 6
/* LISP-SRC: (setf snd-mode-unknown 6) */
#define SND_NUM_MODES  7

/* if snd_open fails, source is set to SND_DEVICE_NONE */
#define SND_DEVICE_NONE 0
#define SND_DEVICE_FILE 1
#define SND_DEVICE_AUDIO 2
#define SND_DEVICE_MEM 3

/* this value is unique for both FILE* and file numbers,
 * either of which may be returned by snd_file_open or
 * snd_file_creat. It is used to indicate "no file"
 */
#define SND_FILE_FAILURE -1


/* write_flag values */
#define SND_READ 0
#define SND_WRITE 1
#define SND_OVERWRITE 2

#define SND_REALTIME 0
#define SND_COMPUTEAHEAD 1

#define SND_LOOP_NONE 0
#define SND_LOOP_FORWARD 1
#define SND_LOOP_FORWARD_BACKWARD 2

typedef struct {
    int mode;
    long begin;
    long end;
} loop_node, *loop_type;

typedef struct {
    long channels;      /* number of channels */
    long mode;          /* ADPCM, PCM, ULAW, ALAW, FLOAT, UPCM */
    long bits;          /* bits per sample */
    double srate;       /* sample rate */
} format_node;


/* notes on audio protocol:
If you are writing audio to a device, there are two things you
might want to optimize: (1) you want to maintain just enough
buffered data to handle the system+application latency. You
might want to output data incrementally in small units to avoid
a long interval of audio computation. If buffers have a
practical minimum length, a solution is to fill system buffers
incrementally. This approach is supported by the SND_REALTIME
protocol; (2) you can afford to compute large buffers in one
step, and for simplicity you want to fill buffers as soon as
they are available.  This approach is supported by the
SND_COMPUTEAHEAD protocol.

In either case, the audio.latency field specifies the estimated
worst case latency.  (For example, the system may be swapping
pages and the application may be garbage collecting.)  Then
buffer space will be at least large enough to hold
audio.latency seconds of audio.

The audio.granularity field specifies the maximum duration of
a block of samples. The application will not compute samples
until there is room for audio.granularity of samples (for
example, it will test by calling snd_poll()). The application
will pass at most audio.granularity of samples to snd_write().
On input, the application will wait until there are at least
audio.granularity of samples and then read no more than that amount.

For sound input, SND_REALTIME and SND_COMPUTEAHEAD serve similar
purposes.  Use SND_REALTIME if you want to read data incrementally
a bit at a time.  Use SND_COMPUTEAHEAD if you want to be informed
(by snd_poll) of all the data as soon as it is available,
regardless of how big the buffers are.

[if I understand correctly: "Use SND_COMPUTEAHEAD if you want to be 
 informed as soon as all the data is available....".   -eub]

*/

/* Prevent compiler errors */
struct snd_struct;

/* these do not use snd_type because it is not defined yet... */
typedef int (*snd_reset_fn)(struct snd_struct *snd);
typedef long (*snd_poll_fn)(struct snd_struct *snd);
typedef long (*snd_read_fn)(struct snd_struct *snd, void *buffer, long length);
typedef long (*snd_write_fn)(struct snd_struct *snd, void *buffer, long length);
typedef int (*snd_open_fn)(struct snd_struct *snd, long *flags);
typedef int (*snd_close_fn)(struct snd_struct *snd);
//typedef int (*snd_reset_fn)(struct snd_struct *snd);
typedef int (*snd_flush_fn)(struct snd_struct *snd);

typedef struct {
    snd_poll_fn poll;
    snd_read_fn read;
    snd_write_fn write;
    snd_open_fn open;
    snd_close_fn close;
    snd_reset_fn reset;
    snd_flush_fn flush;
} snd_fns_node, *snd_fns_type;


typedef struct snd_struct {
    short device;       /* file, audio, or memory */
    short write_flag;   /* SND_READ, SND_WRITE, SND_OVERWRITE */
    format_node format; /* sample format: channels, mode, bits, srate */
    snd_fns_type dictionary;    /* for internal use only */
    union {
      struct {
        char filename[snd_string_max];  /* file name */
        char filetype[snd_string_max];  /* file type/format description */
        int file;               /* OS file number */
        long header; /* None, AIFF, IRCAM, NEXT, WAVE */
        long byte_offset;       /* file offset of first sample */
        long end_offset; /* byte_offset of last byte + 1 */
        long current_offset;  /* current (computed) file offset */
        int swap;       /* flag to swap bytes on input/output */
        /* fields from AIFF sample files: */
        int loop_info; /* Boolean: is this loop info valid? */
        double native_hz; /* original pitch in hz */
        float gain; /* gain: scale factor */
        double low_hz;
        double high_hz;
        char low_velocity;
        char high_velocity;
        loop_node sustain_loop;
        loop_node release_loop;
      } file;
      struct {
        char interfacename[snd_string_max]; /* (optional) to specify interface */
        char devicename[snd_string_max]; /* (optional) to specify device */
        void *descriptor;
        long protocol;  /* SND_REALTIME or SND_COMPUTEAHEAD */
        double latency; /* app + os worst case latency (seconds) */
        double granularity;     /* expected period of app computation (s) */
        /* note: pass 0.0 for default latency and granularity */
      } audio;
      struct {
        long buffer_max;        /* size of buffer memory */
        char *buffer;   /* memory buffer */
        long buffer_len;        /* length of data in buffer */
        long buffer_pos;        /* current location in buffer */
      } mem;
    } u;
} snd_node, *snd_type;

typedef long (*cvtfn_type)(void *buf1, void *buf2, long len2, 
               float scale, float *peak);

/* when open fails, the dictionary gets this set of functions: */
extern snd_fns_node snd_none_dictionary;

/* snd_open, snd_close, snd_convert return SND_SUCESS if successful */
#define SND_SUCCESS 0

#ifdef __cplusplus
extern "C" {
#endif

/* these two are for internal use only (and should really be declared
 * in another .h file not included by clients):
 */
void snd_init();
void snd_add_device(char *interf, char *device, snd_fns_type dictionary);

long snd_bytes_per_frame(snd_type snd);

int snd_open(snd_type snd, long *flags);

int snd_seek(snd_type snd, double skip); /* set current location in file */
int snd_reset(snd_type snd);    /* flush non-file buffers */

int snd_close(snd_type snd);

int snd_flush(snd_type snd);

long snd_read(snd_type snd, void *buffer, long length);

long snd_write(snd_type snd, void *buffer, long length);

long snd_convert(snd_type snd1, void *buffer1,
    snd_type snd2, void *buffer2, long length);

void change_byte_order(snd_type snd, void *buffer, long length);

long snd_poll(snd_type snd);

cvtfn_type select_cvtfn(snd_type snd1, snd_type snd2);


/* some conversion function lookup tables: */

extern cvtfn_type cvt_from_8[];
extern cvtfn_type cvt_from_16[];
extern cvtfn_type cvt_from_24[];
extern cvtfn_type cvt_from_32[];
extern cvtfn_type cvt_to_8[];
extern cvtfn_type cvt_to_16[];
extern cvtfn_type cvt_to_24[];
extern cvtfn_type cvt_to_32[];

long cvt_from_unknown(void *buf1, void *buf2, long len2, 
                      float scale, float *peak);
#define cvt_to_unknown cvt_from_unknown

char *snd_mode_to_string(long mode);

/* this number is returned by snd_poll if you pass it a file device */
#define MAX_FILE_BUF_LEN 20000

/* this is necessary for SGI */
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#ifdef __cplusplus
}
#endif
