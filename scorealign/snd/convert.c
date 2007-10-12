/* Standard includes */

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

/* snd includes */

#include "snd.h"

/* set the length of buffers to be even multiple of Audio_...
 *  to avoid fractional filling of
 * audio output buffers
 */
#define MAXLEN MAX_FILE_BUF_LEN

char *header_string[] = { "none", "AIFF", "IRCAM", "NeXT", "Wave" };

char *header_to_string(long header)
{
    if (header < 0 || header > 4) return "InvalidData";
    else return header_string[header];
}

void snd_print_info(snd_type snd)
{
    if (snd->device == SND_DEVICE_AUDIO) {
        if (snd->write_flag == SND_READ) {
            printf("audio input");
        } else {
            printf("audio output");
        }
    } else if (snd->device == SND_DEVICE_MEM) {
        if (snd->write_flag == SND_READ) {
            printf("from memory buffer");
        } else {
            printf("to memory buffer");
        }
    } else {
        printf("header %s", header_to_string(snd->u.file.header));
    }
    printf(", channels %d, mode %s, bits %d, srate %g",
           snd->format.channels, 
           snd_mode_to_string(snd->format.mode),
           snd->format.bits, snd->format.srate);
    if (snd->device == SND_DEVICE_MEM) {
        if (snd->write_flag == SND_READ) {
            printf("length %d\n", snd->u.mem.buffer_len);
        } else {
            printf("length %d\n", snd->u.mem.buffer_max);
        }
    } else if (snd->device == SND_DEVICE_FILE &&
               snd->write_flag == SND_READ) {
        printf(", length %d\n", (snd->u.file.end_offset - 
                                snd->u.file.byte_offset) / snd_bytes_per_frame(snd));
    } else printf("\n");
}


int main(int argc, char *argv[])
{
    char *fromfile = NULL;
    char *tofile = NULL;
    int i;
    long flags = 0;
    long frames;
    long len;
    long checksum = 0;
    long count = 0;

    int format = SND_HEAD_AIFF;
    int mode = SND_MODE_PCM;
    int channels = 1;
    int bits = 16;
    double srate = 44100.0;
    snd_node from_snd, to_snd;
    char from_buf[MAXLEN];
    char to_buf[MAXLEN];
    long frames_from_len;
    long frames_to_len;
    long from_len = 0;
    long to_len = 0;

    for (i = 1; i < argc; i++) {
        if (*(argv[i]) != '-') {
            if (!fromfile) fromfile = argv[i];
            else if (!tofile) tofile = argv[i];
            else {
                printf("%s: don't know what to do with 3rd file\n", argv[i]);
                return 1;
            }
        } else {
            char *flag = argv[i] + 1;
            if (*flag == '?') {
                printf("convert -- sound file conversion utility\n\n");
                printf("usage: convert fromfile tofile -flag1 -flag2 ...\n");
                printf("  -fa -- AIFF file format\n");
                printf("  -fi -- IRCAM file format\n");
                printf("  -fn -- NeXT/Sun file format\n");
                printf("  -fw -- Wave file format\n");
                printf("  -ep -- PCM\n");
                printf("  -em -- u-Law\n");
                printf("  -ef -- float\n");
                printf("  -eu -- unsigned PCM\n");
                printf("  -b1 -- 8-bit\n");
                printf("  -b2 -- 16-bit\n");
                printf("  -b4 -- 32-bit\n");
                printf("  -c1 -- 1 channel, etc.\n");
                printf("use 'ada' to get audio input or output\n");
            } else if (*flag == 'f') {
                switch (flag[1]) {
                  case 'a': format = SND_HEAD_AIFF; break;
                  case 'i': format = SND_HEAD_IRCAM; break;
                  case 'n': format = SND_HEAD_NEXT; break;
                  case 'w': format = SND_HEAD_WAVE; break;
                  default: printf("flag %s ignored\n", argv[i]); break;
                }
            } else if (*flag == 'e') {
                switch (flag[1]) {
                  case 'p': mode = SND_MODE_PCM; break;
                  case 'm': mode = SND_MODE_ULAW; break;
                  case 'f': mode = SND_MODE_FLOAT; break;
                  case 'u': mode = SND_MODE_UPCM; break;
                  default: printf("flag %s ignored\n", argv[i]); break;
                } 
            } else if (*flag == 'b') {
                switch (flag[1]) {
                  case '1': bits = 8; break;
                  case '2': bits = 16; break;
                  case '4': bits = 32; break;
                  default: printf("flag %s ignored\n", argv[i]); break;
                }
            } else if (*flag == 'r') {
                switch (flag[1]) {
                  case '4': srate = 44100.0; break;
                  case '2': srate = 22050.0; break;
                  case '1': srate = 11025.0; break;
                  case '8': srate = 8000.0; break;
                  default: printf("flag %s ignored\n", argv[i]); break;
                }
            } else if (*flag == 'c') {
                channels = atoi(flag+1);
                if (channels < 1 || channels > 16) {
                    channels = 1;
                    printf("flag %s ignored, using 1 channel\n",
                           argv[i]); 
                }
            }
        }
    }
    if (!tofile) {
        printf("2 files not found, use -? for help\n");
        return 1;
    }
    
    from_snd.device = SND_DEVICE_FILE;
    from_snd.write_flag = SND_READ;
    from_snd.u.file.byte_offset = 0;
    from_snd.format.channels = channels;
    from_snd.format.mode = mode;
    from_snd.format.bits = bits;
    from_snd.format.srate = srate;

    if (strcmp(fromfile, "ada") == 0) {
        from_snd.device = SND_DEVICE_AUDIO;
        from_snd.u.audio.protocol = SND_COMPUTEAHEAD;
        from_snd.u.audio.latency = 1.0;
        from_snd.u.audio.granularity = 0.1;
        strcpy(from_snd.u.file.filename, "");
    } else {
        strcpy(from_snd.u.file.filename, fromfile);
    }
    if (snd_open(&from_snd, &flags) != SND_SUCCESS) {
        printf("error opening %s for input\n", fromfile);
        exit(1);
    }
    printf("Opened %s for input: ", from_snd.u.file.filename);
    snd_print_info(&from_snd);
    
    to_snd.device = SND_DEVICE_FILE;
    to_snd.write_flag = SND_WRITE;
    to_snd.format.channels = channels;
    to_snd.format.mode = mode;
    to_snd.format.bits = bits;
    to_snd.format.srate = from_snd.format.srate;
    to_snd.u.file.header = format;	/* header format, e.g. AIFF */
    if (to_snd.u.file.header == SND_HEAD_WAVE && to_snd.format.mode == SND_MODE_PCM &&
        to_snd.format.bits == 8) to_snd.format.mode = SND_MODE_UPCM;
    if (strcmp(tofile, "ada") == 0) {
        to_snd.device = SND_DEVICE_AUDIO;
        to_snd.u.audio.protocol = SND_COMPUTEAHEAD;
        to_snd.u.audio.latency = 0.0;
        to_snd.u.audio.granularity = 0.1;
        strcpy(to_snd.u.audio.interfacename, "");
        strcpy(to_snd.u.audio.devicename, "");
    } else {
        strcpy(to_snd.u.file.filename, tofile);
    }
    if (snd_open(&to_snd, &flags) != SND_SUCCESS) {
        printf("error opening %s for output\n", tofile);
        exit(1);
    }
    printf("Opened %s for output: ", to_snd.u.file.filename);
    snd_print_info(&to_snd);

    /* figure out how much to convert on each iteration */

    /* first, compute how many frames could fit in each buffer */
    from_len = MAXLEN / snd_bytes_per_frame(&from_snd);
    to_len = MAXLEN / snd_bytes_per_frame(&to_snd);

    /* then compute the minimum of the two frame counts */
    frames = min(from_len, to_len);

    while (1) {
        /* then convert back from frame counts to byte counts: */
        while ((frames_from_len = snd_poll(&from_snd)) <=0);
        frames_from_len = min(frames_from_len, frames);
        while ((frames_to_len = snd_poll(&to_snd)) <= 0);
        frames_to_len = min(frames_to_len, frames);
        len = min(frames_to_len, frames_from_len);
        len = snd_read(&from_snd, from_buf, len);
        if (((from_snd.device == SND_DEVICE_AUDIO) && 
             (count > from_snd.format.srate * 10)) || (!len)) {
                break;
        }
        for (i = 0; i < len * snd_bytes_per_frame(&from_snd); i++) {
            checksum += from_buf[i];
            count++;
        }
        len = snd_convert(&to_snd, to_buf, &from_snd, from_buf, len);
        len = snd_write(&to_snd, to_buf, len);
        printf("#");
        fflush(stdout);
    }
    snd_close(&from_snd);
    if (to_snd.device == SND_DEVICE_AUDIO) {
        while (snd_flush(&to_snd) != SND_SUCCESS) ;
    }
    snd_close(&to_snd);

    printf("Read %ld frames, checksum = %ld\n", count, checksum);
    exit(0);
}



