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

/* Standard includes */
#include <string.h>
#include <stdio.h>

#include "snd.h"
#include "sndfileio.h"
#include "sndheader.h"

static int snd_initialized = FALSE;

typedef struct {
    char *interf;
    char *device;
    snd_fns_type dictionary;
} descriptor_node, *descriptor_type;


static int descriptor_index = 0;
#define snd_descriptor_max 32
static descriptor_node descriptors[snd_descriptor_max];


/* snd_add_device -- describe interface/device pair to library 
 *
 * This is called at intialization time, once for each 
 * interface (e.g. DirectSound) and device (e.g. SoundBlaster 1)
 * The strings are retained but NOT COPIED, so do not destroy them!
 */
void snd_add_device(char *interf, char *device, snd_fns_type dictionary)
{
    if (descriptor_index >= snd_descriptor_max) {
        snd_fail("snd_add_device: snd_descriptor_max exceeded");
    }
    descriptors[descriptor_index].interf = interf;
    descriptors[descriptor_index].device = device;
    descriptors[descriptor_index].dictionary = dictionary;
    descriptor_index++;
}
    

int snd_device(int n, char *interf, char *device)
{
    if (!snd_initialized) {
        snd_init();
        snd_initialized = TRUE;
    }
    if (n >= 0 && n < descriptor_index) {
        strcpy(interf, descriptors[n].interf);
        strcpy(device, descriptors[n].device);
        return SND_SUCCESS;
    }
    return !SND_SUCCESS;
}


/* failure_fn -- "noop" function pointer */
/**/
static int failure_fn(snd_type snd)
{
    return !SND_SUCCESS;
}


/* success_fn -- "noop" function pointer */
/**/
static int success_fn(snd_type snd)
{
    return SND_SUCCESS;
}


static long file_poll(snd_type snd)
{
    return MAX_FILE_BUF_LEN;

}


static long mem_poll(snd_type snd)
{
    if (snd->write_flag == SND_READ) {
        return (snd->u.mem.buffer_len - snd->u.mem.buffer_pos);
    } else {
        return (snd->u.mem.buffer_max - snd->u.mem.buffer_len);
    }
}


static long none_poll(snd_type snd)
{
    return 0;
}

#if !defined(WIN32) && !defined(IRIX) && !defined(SGI)
static void _swab(char *to, char *from, long length)
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


void change_byte_order(snd_type snd, void *buffer, long length)
{
    int bytes = (snd->format.bits + 7) >> 3;
    switch (bytes) {
      case 1:
        break;
      case 2:
        _swab((char *) buffer, (char *) buffer, length);
        break;
      case 3: {
        char *buff = (char *) buffer;
        while (length > 0) {
            char b = buff[0];
            buff[0] = buff[2];
            buff[2] = b;
            buff += 3;
            length -= 3;
        }
        break;
      }
      case 4: {
        long *buff = (long *) buffer;
        while (length > 0) {
            long data = *buff;
            *buff++ = ((data >> 24) & 0xFF) | ((data >> 8) & 0xFF00) |
                ((data << 8) & 0xFF0000) | (data << 24);
            length -= 4;
        }
        break;
      }
      default: {
        char msg[100];
        sprintf(msg, "Cannot handle %d-byte samples", bytes);
        snd_fail(msg);
        break;
      }
    }
}


static long file_read(snd_type snd, void *buffer, long length)
{
    long togo, cnt;
    togo = snd->u.file.end_offset - snd->u.file.current_offset;
    if (length > togo) length = togo;
    cnt = snd_file_read(snd->u.file.file, (char *) buffer, length);
    snd->u.file.current_offset += cnt;
    if (snd->u.file.swap) change_byte_order(snd, buffer, cnt);
    return cnt;
}

      
static long mem_read(snd_type snd, void *buffer, long length)
{
    /* if there are fewer than length bytes, reduce length: */
    if ((snd->u.mem.buffer_len - snd->u.mem.buffer_pos) < length) {
        length = snd->u.mem.buffer_len - snd->u.mem.buffer_pos;
    }
    memcpy(buffer, snd->u.mem.buffer + snd->u.mem.buffer_pos, 
           length);
    snd->u.mem.buffer_pos += length;
    return length;
}


static long none_read(snd_type snd, void *buffer, long length)
{
    return 0;
}


static long file_write(snd_type snd, void *buffer, long length)
{
    long cnt;
    if (snd->u.file.swap) change_byte_order(snd, buffer, length);
    cnt = snd_file_write(snd->u.file.file, (char *) buffer, length);
    snd->u.file.end_offset += cnt;	/* keep track of how much data */
    return cnt;
}

    
static long mem_write(snd_type snd, void *buffer, long length)
{
    /* if there are fewer than length bytes, reduce length: */
    if ((snd->u.mem.buffer_max - snd->u.mem.buffer_len) < length) {
        length = snd->u.mem.buffer_max - snd->u.mem.buffer_len;
    }
    memcpy(snd->u.mem.buffer + snd->u.mem.buffer_len, buffer, length);
    snd->u.mem.buffer_len += length;
    return length;
}


static long none_write(snd_type snd, void *buffer, long length)
{
    return 0;
}


static int mem_open(snd_type snd, long *flags)
{
    return SND_SUCCESS;
}


static int none_open(snd_type snd, long *flags)
{
    return !SND_SUCCESS;
}


static int file_close(snd_type snd)
{
    if (snd->write_flag == SND_WRITE) {
        write_sndheader_finish(snd);
    }
    return snd_file_close(snd->u.file.file);
}


#define mem_close success_fn

#define none_close failure_fn

#define file_reset success_fn


static int mem_reset(snd_type snd)
{
    snd->u.mem.buffer_pos = 0;
    if (snd->write_flag != SND_READ) {
        snd->u.mem.buffer_len = 0;
    }
    return SND_SUCCESS;
}


#define none_reset failure_fn


#define file_flush success_fn

#define mem_flush success_fn

#define none_flush failure_fn


char *snd_mode_string[] = { "ADPCM", "PCM", "ULAW", "ALAW", "Float", "UPCM", "Unknown" };

char *snd_mode_to_string(long mode)
{
    if (mode < 0 || mode >= SND_NUM_MODES) return "InvalidData";
    else return snd_mode_string[mode];
}


static snd_fns_node mem_dictionary = { 
    mem_poll, mem_read, mem_write, mem_open,
    mem_close, mem_reset, mem_flush };

static snd_fns_node file_dictionary = {
    file_poll, file_read, file_write, snd_open_file,
        file_close, file_reset, file_flush };

snd_fns_node snd_none_dictionary = {
    none_poll, none_read, none_write, none_open,
        none_close, none_reset, none_flush };

long snd_poll(snd_type snd)
{
    return (*snd->dictionary->poll)(snd);
}


/* snd_read -- read up to length frames from source into buffer */
/*
 * returns number of frames actually read
 */
long snd_read(snd_type snd, void *buffer, long length)
{
    int bpf = snd_bytes_per_frame(snd);
    return ((*snd->dictionary->read)(snd, buffer, length * bpf)) / bpf;
}


long snd_write(snd_type snd, void *buffer, long length)
/* note: length is in frames, returns number of frames actually written */
{
    int bpf = snd_bytes_per_frame(snd);
    return (*snd->dictionary->write)(snd, buffer, length * bpf) / bpf;
}



int snd_open(snd_type snd, long *flags)
{
    if (!snd_initialized) {
        snd_init();
        snd_initialized = TRUE;
    }
    *flags = 0;
    if (snd->device == SND_DEVICE_FILE) {
        snd->dictionary = &file_dictionary;
    } else if (snd->device == SND_DEVICE_AUDIO) {
        int i;
        for (i = 0; i < descriptor_index; i++) {
            /* take the first descriptor that matches both interface 
               and device, where empty string matches anything: */
            if ((snd->u.audio.interfacename[0] == 0 ||
                 strcmp(snd->u.audio.interfacename, 
                        descriptors[i].interf) == 0) &&
                                /*&& commented out by dmazzoni - don't want to compare dev 
                  && uncommented by rbd - please see previous comment/specification */
                (snd->u.audio.devicename[0] == 0 ||
                 strcmp(snd->u.audio.devicename, 
                                 descriptors[i].device) == 0)) {
                snd->dictionary = descriptors[i].dictionary;
                break;
            }
        }
        if (i >= descriptor_index) {
            return !SND_SUCCESS;
        }
    } else if (snd->device == SND_DEVICE_MEM) {
        snd->dictionary = &mem_dictionary;
    }
    return (*snd->dictionary->open)(snd, flags);
}


int snd_close(snd_type snd)
{
    return (*snd->dictionary->close)(snd);
}


int snd_reset(snd_type snd)
{
    return (*snd->dictionary->reset)(snd);
}


/* snd_flush -- allows client to finish audio output before closing */
/**/
int snd_flush(snd_type snd)
{
    if (snd->write_flag != SND_READ) {
        return (*snd->dictionary->flush)(snd);
    }
    return SND_SUCCESS;
}
