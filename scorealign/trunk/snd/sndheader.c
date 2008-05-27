/* snd.c -- low-level sound I/O
 *
 * Roger Dannenberg
 * 21 Jun 1997
 *
 * based on sndheader.c:
 *
 * Jim Zelenka, CMU/ITC, 9 Jun 1992 (rewritten from my old sources)
 * Roger Dannenberg, CMU, Mar 1993 (extensive changes and additions)
 *
 * and on sndread.c & sndwrite.c:
 *
 * Roger Dannenberg
 */

/* CHANGE LOG
 * --------------------------------------------------------------------
 * 28Apr03  dm  changes for portability and fix compiler warnings
 */

/* Standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "snd.h"
#include "sndfileio.h"
#include "sndheader.h"
#include "ieeecvt.h"
#include "sndhead.h"
/* for HUGE_VAL: */
#include "math.h"

#ifdef _WIN32
    #define int32_t int
#endif

typedef int32_t int32;

/* #define TRACE 1 */

#ifdef LINUX
#  ifdef WIN32
#    error LINUX and WIN32 both?
#  endif
// #  include "sys/file.h"
// #  include <sys/stat.h>
#  include <netinet/in.h>
#else
#ifndef WIN32
#  include <unistd.h>
// #  include <stat.h>
// #  include <fcntl.h>
#else
// #  include <sys/stat.h>
// #  include "io.h"
// #  include "fcntl.h"
#  ifdef WINNT
#    include <winsock.h>
#  else
#    include <winsock2.h>
#  endif
#endif

#endif


#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif


#define PERMISSION 0644 /* -rw-r--r-- */

/* these macros are for byte ordering -- 
 * doesn't the mac have std macros for this? 
 */

/* u_long PASCAL FAR ntohl(u_long netlong); */

#ifdef MACINTOSH
#define ntohs(x) (x)
#define ntohl(x) (x)
#define htons(x) (x)
#define htonl(x) (x)
#endif

/* AIFF file Marker declaration */
typedef struct {
    short id;
    long position;
} marker_node, *marker_type;

/*  instrument definitions */
typedef short marker_id;
typedef struct {
    short   play_mode;
    marker_id  begin_loop;
    marker_id  end_loop;
} aiff_loop_node, *aiff_loop_type;


typedef struct {
    char        base_note;
    char        detune;
    char        low_note;
    char        high_note;
    char        low_velocity;
    char        high_velocity;
    short       gain;
    aiff_loop_node      sustain_loop;
    aiff_loop_node      release_loop;
} inst_node;


long snd_read_header(snd_type snd, long *flags);
void snd_write_header(snd_type snd, long *flags);

double read_ieee_extended(snd_type snd);
void write_ieee_extended(int file, double x);
void ConvertToIeeeExtended(double num, char *bytes);

/*
 * reset the file to read from the beginning
 */

void resetfile(int file)
{
    snd_file_lseek(file, 0, SND_SEEK_SET);
}


/*
 * reads a an item of type T from F into L, if unsuccessful,
 * resets file to beginning and returns
 */
#define readitem(F,L,T) { \
    int r; \
    r = snd_file_read(F, (char *) L,sizeof(T)); \
    if (r != sizeof(T)) { resetfile(file); return *(L); } \
    *read_in += sizeof(T); \
}


char readchar(int file, long *read_in)
{
    char c = 0;
    readitem(file, &c, char);
    return c;
}


int32 readint32(int file, long *read_in)
{
    long l = 0;
    readitem(file, &l, int32);
    return ntohl(l);
}


short readshort(int file, long *read_in)
{ 
    short s = 0;
    readitem(file, &s, short);
    return ntohs(s);
}


long revint32(int32 l)
{
    return (((l >> 0) & 0xFF) << 24) |
        (((l >> 8) & 0xFF) << 16) |
        (((l >> 16) & 0xFF) << 8) |
        (((l >> 24) & 0xFF) << 0);
}


long readrevint32(int file, long *read_in)
{
    return revint32(readint32(file, read_in));
}


short revshort(short s)
{
    return ((s & 0xFF) << 8) | ((s >> 8) & 0xFF);
}


short readrevshort(int file, long *read_in)
{
    return revshort(readshort(file, read_in));
}


float readfloat(int file, long *read_in)
{
    float f = 0.0F;
    readitem(file, &f, float);
    return f;
}

unsigned char readuchar(int file, long *read_in)
{
    unsigned char c = 0;
    readitem(file, &c, unsigned char);
    return c;
}


/*
 * write a an item of type T to F from L (no error handling yet)
 */
#define writeitem(F,L,T) snd_file_write(F, (char *) L, sizeof(T));


void writeint32(int file, int32 l)
{
    l = htonl(l);
    writeitem(file, &l, int32);
}

void writeshort(int file, short s)
{
    s = htons(s);
    writeitem(file, &s, short);
}

void writerevint32(int file, int32 l)
{
    writeint32(file, revint32(l));
}


void writerevshort(int file, short s)
{
    writeshort(file, revshort(s));
}


void writefloat(int file, float f)
{
    writeitem(file, &f, float);
}


void writeuchar(int file, unsigned char c)
{
    writeitem(file, &c, unsigned char);
}


static int fail(snd_type snd, char *str)
{
    resetfile(snd->u.file.file);
    snd->u.file.header = SND_HEAD_NONE;
    snd_fail(str);
    return !SND_SUCCESS;
}


void snd_open_fail(snd_type snd)
{
    /* char msg[250]; */
    snd->device = SND_DEVICE_NONE;
    snd->dictionary = &snd_none_dictionary;
    /* It's not fatal if the file does not exist...
    sprintf(msg, "snd_open: could not open file %s\n",
        snd->u.file.filename);
    snd_fail(msg); */
    return;
}


int head_to_byteorder[] = { 0, 1, 1, 1, 0 };

/* set_swap -- set the swap flag in snd */
/**/
void set_swap(snd_type snd)
{
    int n = head_to_byteorder[snd->u.file.header];
    if (snd->u.file.header == SND_HEAD_NONE) {
        /* go with whatever caller specified */
    } else if (htonl(0x12345678) == 0x12345678)
        snd->u.file.swap = !n;
    else snd->u.file.swap = n;
}

int snd_open_file(snd_type snd, long *flags)
{
    int file, rslt;
    if (snd->write_flag == SND_READ) {
        snd->u.file.loop_info = FALSE;
        file = snd_file_open(snd->u.file.filename, SND_RDONLY);
        snd->u.file.file = file;
        if (file == SND_FILE_FAILURE) {
            snd_open_fail(snd);
            return file;
        }
        rslt = snd_read_header(snd, flags);
        set_swap(snd);
        return rslt;
    } else if (snd->write_flag == SND_WRITE) {
        if (snd->u.file.header < 0 || snd->u.file.header >= SND_NUM_HEADS) {
            snd_fail("snd_open: invalid header value");
            return !SND_SUCCESS;
        }
        file = snd_file_creat(snd->u.file.filename);
        snd->u.file.file = file;
        if (file == SND_FILE_FAILURE) {
            snd_open_fail(snd);
            return file;
        }
        /* special case: in WAV files, 8-bit PCM is stored as UPCM */
        if (snd->u.file.header == SND_HEAD_WAVE &&
            snd->format.mode == SND_MODE_PCM &&
            snd->format.bits == 8) {
            snd->format.mode = SND_MODE_UPCM;
        }
        snd_write_header(snd, flags);
    } else if (snd->write_flag == SND_OVERWRITE) {
        file = snd_file_open(snd->u.file.filename, SND_RDWR);
        snd->u.file.file = file;
        if (file < 0) {
            snd_open_fail(snd);
            return file;
        }
        snd_file_lseek(file, snd->u.file.byte_offset, SND_SEEK_SET);
        snd->u.file.current_offset = snd->u.file.byte_offset;
        /* special case: in WAV files, 8-bit PCM is stored as UPCM */
        if (snd->u.file.header == SND_HEAD_WAVE &&
            snd->format.mode == SND_MODE_PCM &&
            snd->format.bits == 8) {
            snd->format.mode = SND_MODE_UPCM;
        }
    }
    set_swap(snd);
    return SND_SUCCESS;
}


int snd_seek(snd_type snd, double when)
{
    long offset_in_bytes = ((long) (when * snd->format.srate + 0.5)) *
        snd_bytes_per_frame(snd);
    long new_offset = snd->u.file.byte_offset + offset_in_bytes;
    
    if (new_offset > snd->u.file.end_offset) return !SND_SUCCESS;
    if (new_offset < snd->u.file.byte_offset) return !SND_SUCCESS;
    snd->u.file.current_offset = new_offset;
    if (snd_file_lseek(snd->u.file.file, new_offset, SND_SEEK_SET) == -1L)
        return !SND_SUCCESS;
    return SND_SUCCESS;
}

void readloop(aiff_loop_type loop, int file, long *read_in)
{
    loop->play_mode = readshort(file, read_in);
    loop->begin_loop = readshort(file, read_in);
    loop->end_loop = readshort(file, read_in);
}


/* convert_loop_data -- from AIFF loop structure to snd loop structure */
/**/
void convert_loop_data(loop_type loop, aiff_loop_type aiffloop, 
                       marker_type markers, int nmarkers)
{
    int i;
    
    loop->mode = aiffloop->play_mode;
    
    loop->begin = loop->end = -1; /* initialize to illegal value */
    for (i = 0; i < nmarkers; i++) {
        if (aiffloop->begin_loop == markers[i].id) {
            loop->begin = markers[i].position;
        }
        if (aiffloop->end_loop == markers[i].id) {
            loop->end = markers[i].position;
        }
    }
}


static double StepToHz(double step) 
{
    return exp(step * 0.0577622650466621 + 2.1011784386926213);
}


#define AIFF_SND_MAGIC (*((int32 *) "FORM"))
#define WAVE_SND_MAGIC (*((int32 *) "RIFF"))

long snd_read_header(snd_type snd, long *flags)
{
    long read_in = 0;
    int32 magic;
    long bytemode, len;
    short type=IRCAM_SND_COMMENT, size=0, encoding;
    unsigned char buf[SIZEOF_IRCAM_HEADER];
    
    snd->u.file.loop_info = FALSE;
    len = snd_file_len(snd->u.file.file);  /* get length of file */
    magic = readint32(snd->u.file.file, &read_in);
    magic = htonl(magic);   /* undo what readint32 did */
    if (magic == IRCAM_SND_MAGIC) {
        snd->u.file.header = SND_HEAD_IRCAM;
        snd->format.srate = (double) readfloat(snd->u.file.file, &read_in);
        snd->format.channels = readint32(snd->u.file.file, &read_in);
        bytemode = readint32(snd->u.file.file, &read_in);
        (*flags) |= SND_HEAD_SRATE | SND_HEAD_CHANNELS;
        /* now SF_ULAW, SF_SHORT, SF_FLOAT,AE_CHAR, AE_ALAW, AE_ULAW, AE_SHORT, AE_LONG, AE_FLOAT or other */
        while (type != IRCAM_SND_END && type != IRCAM_SND_AUDIOENCODE) {
            type = readshort(snd->u.file.file, &read_in);
            size = readshort(snd->u.file.file, &read_in);
            if (size > 2 * sizeof(short)) {
                int rc;
                /* make sure we don't overflow buffer */
                if (size < SIZEOF_IRCAM_HEADER) {
                    rc = snd_file_read(snd->u.file.file, (char *) buf, (size - 2 * sizeof(short)));
                } else {
                    rc = 0; /* force error */
                }
                if (rc != (size - 2 * (int) sizeof(short))) {
                    return fail(snd, "bad IRCAM header");
                }
                read_in += size - 2 * sizeof(short);
            }
        }
        if (type == IRCAM_SND_AUDIOENCODE) {
            /* printf("Got IRCAM sound format\n"); */
            encoding = *((short *)(buf));
            (*flags) |= SND_HEAD_MODE | SND_HEAD_BITS;
            switch (encoding) {
              case IRCAM_SND_CHAR:
                snd->format.mode = SND_MODE_PCM;
                snd->format.bits = 8;
                break;
              case IRCAM_SND_ALAW:
                snd->format.mode = SND_MODE_ALAW;
                snd->format.bits = 8;
                break;
              case IRCAM_SND_ULAW:
                snd->format.mode = SND_MODE_ULAW;
                snd->format.bits = 8;
                break;
              case IRCAM_SND_SHORT:
                snd->format.mode = SND_MODE_PCM;
                snd->format.bits = 16;
                break;
              case IRCAM_SND_LONG:
                snd->format.mode = SND_MODE_PCM;
                snd->format.bits = 32;
                break;
              case IRCAM_SND_FLOAT:
                snd->format.mode = SND_MODE_FLOAT;
                snd->format.bits = 32;
                break;
              default:
                (*flags) &= ~(SND_HEAD_MODE | SND_HEAD_BITS);
                break;
            }
        } else {
            snd->format.bits = bytemode << 3;
            (*flags) |= SND_HEAD_BITS;
            switch (bytemode) {
              case sizeof(char):
                (*flags) |= SND_HEAD_MODE;
                snd->format.mode = SND_MODE_ULAW;
                break;
              case sizeof(short):
                snd->format.mode = SND_MODE_PCM;
                break;
              case sizeof(float):
                snd->format.mode = SND_MODE_FLOAT;
                break;
              default:
                snd->format.mode = SND_MODE_PCM;
                break;
            }
        }
        
        /* seek to end of header */
        snd_file_lseek(snd->u.file.file, SIZEOF_IRCAM_HEADER, SND_SEEK_SET);
        snd->u.file.byte_offset = SIZEOF_IRCAM_HEADER;
        snd->u.file.current_offset = snd->u.file.byte_offset;
        
    } else if (magic == NEXT_SND_MAGIC) {
        long hdr_size, trash, rate;
        snd->u.file.header = SND_HEAD_NEXT;
        hdr_size = readint32(snd->u.file.file, &read_in);        /* dataLocation */
        trash = readint32(snd->u.file.file, &read_in);   /* dataSize */
        bytemode = readint32(snd->u.file.file, &read_in);        /* dataFormat */
        rate = readint32(snd->u.file.file, &read_in);            /* samplingRate */
        snd->format.channels = readint32(snd->u.file.file, &read_in);    /* channelCount */
        
        snd->format.srate = (double) rate;
        (*flags) = SND_HEAD_SRATE | SND_HEAD_CHANNELS;
        switch (bytemode) {
          case NEXT_SND_FORMAT_ULAW_8:
            snd->format.bits = 8;
            snd->format.mode = SND_MODE_ULAW;
            (*flags) |= SND_HEAD_BITS | SND_HEAD_MODE;
            break;
          case NEXT_SND_FORMAT_LINEAR_8:
            snd->format.bits = 8;
            snd->format.mode = SND_MODE_PCM;
            (*flags) |= SND_HEAD_BITS | SND_HEAD_MODE;
            break;
          case NEXT_SND_FORMAT_LINEAR_16:
            snd->format.bits = 16;
            snd->format.mode = SND_MODE_PCM;
            (*flags) |= SND_HEAD_BITS | SND_HEAD_MODE;
            break;
          case NEXT_SND_FORMAT_LINEAR_24:
            snd->format.bits = 24;
            snd->format.mode = SND_MODE_PCM;
            (*flags) |= SND_HEAD_BITS | SND_HEAD_MODE;
            break;
          case NEXT_SND_FORMAT_LINEAR_32:
            snd->format.bits = 32;
            snd->format.mode = SND_MODE_PCM;
            (*flags) |= SND_HEAD_BITS |SND_HEAD_MODE;
            break;
          case NEXT_SND_FORMAT_FLOAT:
            snd->format.bits = sizeof(float) * 8;
            snd->format.mode = SND_MODE_FLOAT;
            (*flags) |= SND_HEAD_BITS | SND_HEAD_MODE;
            break;
          case NEXT_SND_FORMAT_DOUBLE:
            snd->format.bits = sizeof(double) * 8;
            snd->format.mode = SND_MODE_FLOAT;
            (*flags) |= SND_HEAD_BITS | SND_HEAD_MODE;
            break;
          case NEXT_SND_FORMAT_INDIRECT:
            break;
          case NEXT_SND_FORMAT_DSP_DATA_8:
            snd->format.bits = 8;
            (*flags) |= SND_HEAD_BITS;
            break;
          case NEXT_SND_FORMAT_DSP_DATA_16:
            snd->format.bits = 16;
            (*flags) |= SND_HEAD_BITS;
            break;
          case NEXT_SND_FORMAT_DSP_DATA_24:
            snd->format.bits = 24;
            (*flags) |= SND_HEAD_BITS;
            break;
          case NEXT_SND_FORMAT_DSP_DATA_32:
            snd->format.bits = 32;
            (*flags) |= SND_HEAD_BITS;
            break;
          case NEXT_SND_FORMAT_MULAW_SQUELCH:
            snd->format.mode = SND_MODE_ULAW;
            (*flags) |= SND_HEAD_MODE;
            break;
          default:
            break;
        }
        /* position file past header */
        if (read_in < hdr_size) {
            snd_file_lseek(snd->u.file.file, hdr_size, SND_SEEK_SET);
            read_in = hdr_size;
        }
        snd->u.file.byte_offset = read_in;
    } else if (magic == AIFF_SND_MAGIC) {
        unsigned long totalsize;
        unsigned long ssnd_start = 0;
        char buf[4];
        long blocksize=0;
        long offset=0;
        long chunksize;
        long ssnd_chunksize=0;
        inst_node inst;
        short nmarkers=0;
        marker_type markers = NULL;
        int inst_read = FALSE;
        
        snd->u.file.header = SND_HEAD_AIFF;
        totalsize = (unsigned long) readint32(snd->u.file.file, &read_in);
        if (snd_file_read(snd->u.file.file, buf, 4) != 4 || strncmp(buf, "AIFF", 4) != 0) {
            snd->u.file.header = SND_HEAD_NONE;
            (*flags) = 0;
            resetfile(snd->u.file.file);
            read_in = 0;
        } else {
#ifdef TRACE
            printf("snd_read: 'AIFF', totalsize %d, at %d\n",
                   totalsize, snd_file_lseek(snd->u.file.file, (off_t) 0, SND_SEEK_CUR) - 8);
#endif
            /* Skip everything but the COMM, MARK, INST, and SSND chunks,  */

            while (1) {
                if (snd_file_read(snd->u.file.file, buf, 4) != 4) {
                    if (ssnd_start != 0) break; /* found SSND chunk */
                    return fail(snd, "Missing SSND chunk in AIFF file\n");
                }
#ifdef TRACE
                printf("snd_read: '%c%c%c%c' at %d (0%o)\n", 
                       buf[0], buf[1], buf[2], buf[3],
                       snd_file_lseek(snd->u.file.file, (off_t) 0, SND_SEEK_CUR) - 8,
                       snd_file_lseek(snd->u.file.file, (off_t) 0, SND_SEEK_CUR) - 8);
#endif
                if (strncmp(buf, "COMM", 4) == 0) {
                    /* COMM chunk */
                    long chunksize;
                    long frames;
                    chunksize = readint32(snd->u.file.file, &read_in);
                    if (chunksize != 18) {
                        return fail(snd, "AIFF COMM chunk has bad size\n");
                    }
                    snd->format.channels = (long) readshort(snd->u.file.file, &read_in);
                    frames = readint32(snd->u.file.file, &read_in);
                    snd->format.bits = readshort(snd->u.file.file, &read_in);
                    snd->format.mode = SND_MODE_PCM;
                    snd->format.srate = read_ieee_extended(snd);
                    snd->u.file.end_offset = frames * snd_bytes_per_frame(snd) +
                        read_in;
                    (*flags) = SND_HEAD_MODE | SND_HEAD_BITS | SND_HEAD_SRATE |
                        SND_HEAD_CHANNELS | SND_HEAD_LEN;
                } else if (strncmp(buf, "SSND", 4) == 0) {
                    /* SSND chunk */
                    ssnd_chunksize = readint32(snd->u.file.file, &read_in);
                    offset = readint32(snd->u.file.file, &read_in);
                    blocksize = readint32(snd->u.file.file, &read_in);
                    /* remember the file offset, there may be more chunks */
                    ssnd_start = snd_file_lseek(snd->u.file.file, 0, SND_SEEK_CUR);
                    chunksize = ssnd_chunksize;
                    if (chunksize & 1) chunksize++;  /* round up to even number */
                    /* subtract 8, size of offset and blocksize: */
                    snd_file_lseek(snd->u.file.file, chunksize - 8, SND_SEEK_CUR);
                    read_in += chunksize - 8;
                } else if (strncmp(buf, "MARK", 4) == 0) {
                    int i;

                    /*long chunksize = */
                    readint32(snd->u.file.file, &read_in);
                    
                    nmarkers = readshort(snd->u.file.file, &read_in);
                    markers = (marker_type) snd_alloc(nmarkers * sizeof(marker_node));
                    for (i = 0; i < nmarkers; i++) {
                        unsigned char label[256];
                        int len;
                        markers[i].id = readshort(snd->u.file.file, &read_in);
                        markers[i].position = readint32(snd->u.file.file, &read_in);
                        if (snd_file_read(snd->u.file.file, (char *) label, 1L) != 1) 
                            return fail(snd, "problem reading AIFF file\n");
                        len = label[0] | 1;
                        if (snd_file_read(snd->u.file.file, (char *) label, (long) len) != len)
                            return fail(snd, "problam reading AIFF file\n");
                    }
                } else if (strncmp(buf, "INST", 4) == 0) {
                    chunksize = readint32(snd->u.file.file, &read_in);
                    inst.base_note = readchar(snd->u.file.file, &read_in);
                    inst.detune = readchar(snd->u.file.file, &read_in);
                    inst.low_note = readchar(snd->u.file.file, &read_in);
                    inst.high_note = readchar(snd->u.file.file, &read_in);
                    inst.low_velocity = readchar(snd->u.file.file, &read_in);
                    inst.high_velocity = readchar(snd->u.file.file, &read_in);
                    inst.gain = readshort(snd->u.file.file, &read_in);
                    readloop(&inst.sustain_loop, snd->u.file.file, &read_in);
                    readloop(&inst.release_loop, snd->u.file.file, &read_in);
                    
                    /* compute some properties from inst structure:
                     * (marker chunk may not be read in yet, so do that later
                     */
                    
                    /* snd should have a StepToHz function, but it doesn't */
                    snd->u.file.native_hz = StepToHz(inst.base_note + 
                                                     inst.detune/100.0);
                    /* the constant is to convert from db */
                    snd->u.file.gain = (float) exp(((double) inst.gain) * 0.11512925);
                    snd->u.file.low_hz = StepToHz(inst.low_note);
                    snd->u.file.high_hz = StepToHz(inst.high_note);
                    snd->u.file.low_velocity = inst.low_velocity;
                    snd->u.file.high_velocity = inst.high_velocity;
                    inst_read = TRUE;
                } else {
                    long chunksize = readint32(snd->u.file.file, &read_in);
                    if (chunksize & 1) chunksize ++;  /* round up to even number */
                    read_in += chunksize;
                    /* skip the chunk */
                    snd_file_lseek(snd->u.file.file, chunksize, SND_SEEK_CUR);
                }
            }

            snd_file_lseek(snd->u.file.file, ssnd_start, SND_SEEK_SET);
            /* now we're positioned back at SSND chunk samples */
            if (blocksize != 0) {
                return fail(snd, "AIFF header specifies nonzero blocksize.");
            }
            if (markers && inst_read) {
                convert_loop_data(&snd->u.file.sustain_loop, &inst.sustain_loop, 
                                  markers, nmarkers);
                convert_loop_data(&snd->u.file.release_loop, &inst.release_loop, 
                                  markers, nmarkers);
                if (snd->u.file.native_hz <= 0.0) {
                    return fail(snd, "Bad base pitch + detune in AIFF file");
                }
                if (snd->u.file.sustain_loop.mode < 0 ||
                    snd->u.file.sustain_loop.mode > 3) {
                    return fail(snd, "Bad sustain loop mode in AIFF file");
                }
                if (snd->u.file.sustain_loop.mode && 
                    (snd->u.file.sustain_loop.begin < 0 ||
                     snd->u.file.sustain_loop.end <= 
                     snd->u.file.sustain_loop.begin)) {
                    return fail(snd, "Bad sustain loop in AIFF file");
                }
                if (snd->u.file.release_loop.mode < 0 ||
                    snd->u.file.release_loop.mode > 3) {
                    return fail(snd, "Bad release loop mode in AIFF file");
                }
                if (snd->u.file.release_loop.mode && 
                    (snd->u.file.release_loop.begin < 0 ||
                     snd->u.file.release_loop.end <= 
                     snd->u.file.release_loop.begin)) {
                    return fail(snd, "Bad release loop in AIFF file");
                }
                snd->u.file.loop_info = TRUE;
            } 
            if (markers) snd_free(markers);
            /* byte_offset is where the file pointer ends up after SND_SEEK_CUR */
            snd->u.file.byte_offset = snd_file_lseek(snd->u.file.file, offset, SND_SEEK_CUR);
            snd->u.file.current_offset = snd->u.file.byte_offset;
            snd->u.file.end_offset = snd->u.file.byte_offset + ssnd_chunksize - 8;
        }
    } else if (magic == WAVE_SND_MAGIC) {
        long size;
        char buf[4];
        short format;
        
        snd->u.file.header = SND_HEAD_WAVE;
        /* RIFF WAVE uses little-endian format -- be careful! */
        size = readrevint32(snd->u.file.file, &read_in);
        if (snd_file_read(snd->u.file.file, buf,  4) != 4 || strncmp(buf, "WAVE", 4) != 0) {
            return fail(snd, 
                        "RIFF file does not specify 'WAVE' as type\n");
        }
        
        /* Skip to the next "fmt " or end of file */
        while (1) {
            long siz;
            if (snd_file_read(snd->u.file.file, buf, 4) != 4) {
                return fail(snd, "WAVE file missing fmt spec");
            }
            if (strncmp("fmt ", buf, 4) == 0) break;
            siz = readrevint32(snd->u.file.file, &read_in);
            while (siz > 0) {
                snd_file_read(snd->u.file.file, buf, 1);
                siz--;
            }
        }
        size = readrevint32(snd->u.file.file, &read_in);
        format = readrevshort(snd->u.file.file, &read_in);
        switch (format) {
          case WAVE_FORMAT_UNKNOWN:
            return fail(snd, "file in Microsoft Official Unknown format");
          case WAVE_FORMAT_PCM:
            break;    
          case WAVE_FORMAT_ADPCM:
            return fail(snd, "file in ADPCM format");
          case WAVE_FORMAT_ALAW:
            return fail(snd, "file in ALAW format");
          case WAVE_FORMAT_MULAW:
            return fail(snd, "file in ULAW format");
          case WAVE_FORMAT_OKI_ADPCM:
            return fail(snd, "file in OKI ADPCM format");
          case WAVE_FORMAT_DIGISTD:
            return fail(snd, "file in Digistd format");
          case WAVE_FORMAT_DIGIFIX:
            return fail(snd, "file in Digifix format");
          case IBM_FORMAT_MULAW:
            return fail(snd, "file in IBM U-law format");
          case IBM_FORMAT_ALAW:
            return fail(snd, "file in IBM A-law format");
          case IBM_FORMAT_ADPCM:
            return fail(snd, "file in IBM ADPCM format");
          default:
            return fail(snd, "file in unknown format");
        }       
        snd->format.channels = readrevshort(snd->u.file.file, &read_in);
        snd->format.srate = (double) readrevint32(snd->u.file.file, &read_in);
        readrevint32(snd->u.file.file, &read_in);  /* Average bytes/second */
        readrevshort(snd->u.file.file, &read_in); /* Block align */
        snd->format.bits = readrevshort(snd->u.file.file, &read_in);
        snd->format.mode = SND_MODE_PCM;
        if (snd->format.bits == 8) snd->format.mode = SND_MODE_UPCM;    /* unsigned */
        *flags = SND_HEAD_MODE | SND_HEAD_BITS | SND_HEAD_SRATE |
            SND_HEAD_CHANNELS | SND_HEAD_LEN;
        /* size is size of "fmt " section; we've read 16 bytes, so
         * advance file pointer over any remaining bytes
         */
        snd->u.file.byte_offset = 
            snd_file_lseek(snd->u.file.file, size - 16, SND_SEEK_CUR);
        /* skip over any other forms until you find "data" */
        while (1) {
            long n = snd_file_read(snd->u.file.file, buf, 4);
            if (n != 4) {
                return fail(snd, "missing data portion");
            }
            if (strncmp("data", buf, 4) == 0) break;
            n = readrevint32(snd->u.file.file, &read_in);    /* length of form */
            snd->u.file.byte_offset = 
                snd_file_lseek(snd->u.file.file, n, SND_SEEK_CUR);
        }
        snd->u.file.byte_offset += 8; /* "data" and length use 8 bytes */
        snd->u.file.current_offset = snd->u.file.byte_offset;
        snd->u.file.end_offset = snd->u.file.byte_offset + 
            readrevint32(snd->u.file.file, &read_in);
    } else {
        snd->u.file.header = SND_HEAD_NONE;
        (*flags) = 0;
        resetfile(snd->u.file.file);
        read_in = 0;
    }
    
    /* If not already determined from file header, assume remainder of file
       is sound data */
    if (!(*flags & SND_HEAD_LEN)) {
        snd->u.file.end_offset = snd->u.file.byte_offset = read_in;
        snd->u.file.current_offset = snd->u.file.byte_offset;
        snd->u.file.end_offset = len;
        *flags |= SND_HEAD_LEN;
    }
    return SND_SUCCESS;
}


/* write_zeros -- add zeros to end of file */
/**/
void write_zeros(int fout, long n)
{
    long zero = 0;
    while (n > 0) {
        /* don't put min() in the arg list of write on an RS6K */
        /* there seems to be a compiler bug */
        long len = min(n, sizeof(long));
        snd_file_write(fout, (char *) &zero, len);
        n -= sizeof(long);
    }
}


/* write_ircam_start -- write an IRCAM header at current file position */
/*
 * length is the total length of the header to be written; this is normally
 * SIZEOF_IRCAM_HEADER, but in the case of the hybrid CMIX headers, length
 * will be shorter to accommodate the NeXT header prefix
 */
void write_ircam_start(snd_type snd, long length)
{
    short encoding=0;
    
    writeint32(snd->u.file.file, IRCAM_SND_MAGIC);
    writefloat(snd->u.file.file, (float) snd->format.srate);
    writeint32(snd->u.file.file, snd->format.channels);
    writeint32(snd->u.file.file, snd->format.bits >> 3);
    /* now write the "CODE" section */
    writeshort(snd->u.file.file, IRCAM_SND_AUDIOENCODE);
    writeshort(snd->u.file.file, 3 * sizeof(short));    /* size of this record */
    if (snd->format.bits == 8) {
        encoding = IRCAM_SND_CHAR;
        if (snd->format.mode == SND_MODE_ULAW) encoding = IRCAM_SND_ULAW;
        if (snd->format.mode == SND_MODE_ALAW) {
            snd_warn("ALAW not implemented, writing 8-bit PCM\n");
        }
    } else if (snd->format.bits == 16) {
        encoding = IRCAM_SND_SHORT;
    } else if (snd->format.bits == 32) {
        encoding = IRCAM_SND_FLOAT;
        if (snd->format.mode == SND_MODE_PCM) encoding = IRCAM_SND_LONG;
    }
    writeshort(snd->u.file.file, encoding);
    
    /* end the "CODE" section */
    writeshort(snd->u.file.file, IRCAM_SND_END);
    writeshort(snd->u.file.file, 2 * sizeof(short));
    
    /* write filler */
    length -= ( 16 /* head */ + 6 /* AudioEncode */ + 4 /* End */ );
    
    write_zeros(snd->u.file.file, length);
}


/* write_next_start -- write a NeXT header */
/*
 * Note: uses length for Length field, but does not fill with zeros.
 * Instead, this routine writes only the NeXT 24 byte header.
 */
void write_next_start(snd_type snd, long length)
{
    short encoding=0;
    
    writeint32(snd->u.file.file, NEXT_SND_MAGIC);
    /* header size matches cmix's bsd format */
    writeint32(snd->u.file.file, length);
    writeint32(snd->u.file.file, 0); /* data size, 0 -> unspecified */
    if (snd->format.bits == 8) {
        encoding = NEXT_SND_FORMAT_LINEAR_8;
        if (snd->format.mode == SND_MODE_ULAW)
            encoding = NEXT_SND_FORMAT_ULAW_8;
        if (snd->format.mode == SND_MODE_ALAW) {
            snd_warn("ALAW not implemented, writing 8-bit PCM\n");
        }
    } else if (snd->format.bits == 16) {
        encoding = NEXT_SND_FORMAT_LINEAR_16;
    } else if (snd->format.bits == 32) {
        encoding = NEXT_SND_FORMAT_FLOAT;
        if (snd->format.mode == SND_MODE_PCM)
            encoding = NEXT_SND_FORMAT_LINEAR_32;
    }
    writeint32(snd->u.file.file, encoding);
    writeint32(snd->u.file.file, (long) (snd->format.srate + 0.5));
    writeint32(snd->u.file.file, snd->format.channels);
}


/* snd_write_header -- write header, position file for sample data */
/**/
void snd_write_header(snd_type snd, long *flags)
{
    long nframes = 0x7f000000;
    long bytes_per_frame = snd_bytes_per_frame(snd);
    
    /* end_offset will keep track of how much data written so far */
    snd->u.file.end_offset = 0;
    
    switch (snd->u.file.header) {
      case SND_HEAD_NONE:
        break;
      case SND_HEAD_AIFF: {
        int hsize =
            12 /* bytes at beginning */ + 
            8 /*COMM hdr*/ + 18 /*COMM chunk*/ +
            8 /*SSND hdr*/ + 8 /*SSND chunk*/;
        if (snd->format.bits != 8 && snd->format.bits != 16 && snd->format.bits != 24) {
            char msg[64];
            sprintf(msg, "Warning: using 16 bits per sample instead of %d\n",
                    (int)snd->format.bits);
            snd_warn(msg);
            snd->format.bits = 16;
            bytes_per_frame = snd_bytes_per_frame(snd);
        }
        snd_file_write(snd->u.file.file, "FORM", 4); /* IFF header */
        /* (bogus) file size: */
        writeint32(snd->u.file.file, 
                  hsize + nframes * bytes_per_frame);
        snd_file_write(snd->u.file.file, "AIFF", 4); /* File type */
        
        /* COMM chunk -- describes encoding (and #frames) */
        snd_file_write(snd->u.file.file, "COMM", 4);
        writeint32(snd->u.file.file, 18); /* COMM chunk size */
        writeshort(snd->u.file.file, (short) snd->format.channels); /* nchannels */
        writeint32(snd->u.file.file, nframes); /* number of frames */
        writeshort(snd->u.file.file, (short) snd->format.bits); /* sample width, in bits */
        write_ieee_extended(snd->u.file.file, snd->format.srate);
        /* SSND chunk -- describes data */
        snd_file_write(snd->u.file.file, "SSND", 4);
        writeint32(snd->u.file.file, 
                  8 + nframes * bytes_per_frame); /* chunk size */
        writeint32(snd->u.file.file, 0); /* offset */
        writeint32(snd->u.file.file, 0); /* block size */
        snd->u.file.byte_offset = hsize;
        /*      printf("snd_write_header AIFF, byte_offset = %ld\n",
                snd_lseek(snd->u.file.file, (off_t) 0, SND_SEEK_CUR)); */
        break; }
      case SND_HEAD_IRCAM: 
        write_ircam_start(snd, SIZEOF_IRCAM_HEADER);
        snd->u.file.byte_offset = SIZEOF_IRCAM_HEADER;
        break;
      case SND_HEAD_NEXT:
        /* for compatibility with CMIX, we will always write an IRCAM
         * header after the NeXT header, and use 1024 bytes of header.
         */
        write_next_start(snd, SIZEOF_IRCAM_HEADER);
        write_ircam_start(snd, SIZEOF_IRCAM_HEADER - 24);
        snd->u.file.byte_offset = SIZEOF_IRCAM_HEADER;
        break;
      case SND_HEAD_WAVE:
        snd_file_write(snd->u.file.file, "RIFF", 4);
        writerevint32(snd->u.file.file, 12+18+8 + nframes * bytes_per_frame);
        snd_file_write(snd->u.file.file, "WAVE", 4);
        snd_file_write(snd->u.file.file, "fmt ", 4);
        writerevint32(snd->u.file.file, 18L);
        switch (snd->format.mode) /* Format type */
        {
            case SND_MODE_ADPCM:
                writerevshort(snd->u.file.file, 2);
                break;
            case SND_MODE_PCM:
            case SND_MODE_UPCM:
                writerevshort(snd->u.file.file, 1);
                break;
            case SND_MODE_ULAW:
                writerevshort(snd->u.file.file, 7);
                break;
            case SND_MODE_ALAW:
                writerevshort(snd->u.file.file, 6);
                break;
            case SND_MODE_UNKNOWN:
            case SND_MODE_FLOAT:
            default:
                writerevshort(snd->u.file.file, 0);
                break;
        }
        writerevshort(snd->u.file.file, (short) snd->format.channels); /* Number of channels */
        writerevint32(snd->u.file.file, (int32) (snd->format.srate + 0.5)); /* Samples per second */
        writerevint32(snd->u.file.file,  (((int32) snd->format.srate) * bytes_per_frame)); /* Bytes per second*/
        writerevshort(snd->u.file.file, (short) bytes_per_frame); /* Block alignment */
        writerevshort(snd->u.file.file, (short) snd->format.bits); /* Bits per sample */
        writerevshort(snd->u.file.file, (short) 0); /* Size of needed extra data */
        snd_file_write(snd->u.file.file, "data", 4);
        writerevint32(snd->u.file.file, (int32) (nframes * bytes_per_frame));
        snd->u.file.byte_offset = 46;
        break;
      default:
        break;
    }
    snd->u.file.end_offset = snd->u.file.byte_offset;
}


void write_sndheader_finish(snd_type snd)
{
    long n;
    switch (snd->u.file.header) {
      case SND_HEAD_NONE:
        break;
      case SND_HEAD_AIFF: {
        int hsize =
            8 /*COMM hdr*/ + 18 /*COMM chunk*/ +
            8 /*SSND hdr*/ + 12 /*SSND chunk*/;
        int datasize = snd->u.file.end_offset - hsize - 8;
        
        /* get the current position = file size */
        n = snd_file_lseek(snd->u.file.file, 0, SND_SEEK_CUR);
        if (n != snd->u.file.end_offset) {
            char str[80];
            sprintf(str,
                    "Actual file size %ld does not match predicted size %ld\n",
                    n, snd->u.file.end_offset);
            fail(snd, str);
        }
        /* write filesize in the header */
        snd_file_lseek(snd->u.file.file, 4, SND_SEEK_SET);
        writeint32(snd->u.file.file, n - 8 /* 'FORM'+size do not count */);
        
        /* write number of frames in COMM chunk */
        snd_file_lseek(snd->u.file.file, (4 /* 'AIFF' */ + 4 /* 'COMM' */ + 
                                         4 /* size */ + 2 /* channels */), 
              SND_SEEK_CUR);
        writeint32(snd->u.file.file, 
                  (n - (hsize + 8)) / snd_bytes_per_frame(snd));
        
        /* write size in SSND chunk */
        snd_file_lseek(snd->u.file.file, (2 /* snd->format.bits */ + 
                                         10 /* sr */ + 4 /* 'SSND' */), 
              SND_SEEK_CUR);
        writeint32(snd->u.file.file, 8 + datasize); /* chunk size */
        break; }
      case SND_HEAD_IRCAM:
        break;
      case SND_HEAD_NEXT:
        break;
      case SND_HEAD_WAVE:
        /* get the current position = file size */
        n = snd_file_lseek(snd->u.file.file, 0, SND_SEEK_CUR);
        /* back to the top */
        snd_file_lseek(snd->u.file.file, 4, SND_SEEK_SET);
        writerevint32(snd->u.file.file, n - 8);  /* file size - ['RIFF', len] */
        snd_file_lseek(snd->u.file.file, 42, SND_SEEK_SET);
        writerevint32(snd->u.file.file, n - 46); /* data size */
        break;
      default:
        break;
    }
}


double read_ieee_extended(snd_type snd)
{
    unsigned char buf[10];
    if (snd_file_read(snd->u.file.file, (char *) buf, 10L) != 10)
        fail(snd, "EOF while reading IEEE extended number");
    return ConvertFromIeeeExtended(buf);
}

void write_ieee_extended(int file, double x)
{
    char buf[10];
    ConvertToIeeeExtended(x, buf);
    /*
      report("converted %g to %o %o %o %o %o %o %o %o %o %o",
      x,
      buf[0], buf[1], buf[2], buf[3], buf[4],
      buf[5], buf[6], buf[7], buf[8], buf[9]);
      */
    snd_file_write(file, buf, 10);
}




char *mode_string[] = { "ADPCM", "PCM", "ULAW", "ALAW", "Float", "UPCM", "Unknown" };

char *mode_to_string(long mode)
{
    if (mode < 0 || mode >= SND_NUM_MODES) return "InvalidData";
    else return mode_string[mode];
}

