/* sndread.c -- read sound files */

/* CHANGELOG
 *
 * 29Jun95  RBD  ULAW fixed problems with signed chars
 * 28Apr03  dm  changes for portability and fix compiler warnings
 */

#include "switches.h"

#include <stdio.h>
#include <string.h>

#if HAS_STDLIB_H
#include <stdlib.h>
#endif

#if HAS_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAS_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAS_STAT_H
#include <stat.h>
#endif

#ifdef WIN32
#include "io.h"
#endif /* WIN32 */

#include "snd.h"
#include "sndhead.h"

#define SCALE_FACTOR_TO_SHORT 32767
#define SCALE_FACTOR_TO_BYTE 127
#define SCALE_FACTOR_TO_24BIT 0x7FFFFF
#define SCALE_FACTOR_TO_LONG 2147483647

enum
{
    BIAS = 0x84, /* define the add-in bias for 16 bit samples */
    CLIP = 32635,
    SIGN_BIT = 0x80, /* sign bit on A-law / mu-law encoded samples */
    ZEROTRAP = 0
};

static int ulaw_table[256] =
{
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
       876,    844,    812,    780,    748,    716,    684,    652,
       620,    588,    556,    524,    492,    460,    428,    396,
       372,    356,    340,    324,    308,    292,    276,    260,
       244,    228,    212,    196,    180,    164,    148,    132,
       120,    112,    104,     96,     88,     80,     72,     64,
       56,     48,     40,     32,     24,     16,      8,      0
};


long snd_convert2(snd_type snd1, void *buffer1,
                  snd_type snd2, void *buffer2, long length);


unsigned char
st_linear_to_alaw (int sample)
{
    int mask = 0xD5, a_val = 0x7F;
    unsigned int segment;
    short seg_aend[8] = {
        0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF
    };

    sample >>= 3;
    if (sample < 0) {
        mask = 0x55;		/* sign bit = 0 */
        sample = -1 - sample;
    }

    /* Convert the scaled magnitude to segment number. */
    for (segment = 0; segment < 8; segment++) {
        if (sample <= seg_aend[segment]) {
            break;
        }
    }

    /* Combine the sign, segment, and quantization bits. */
    if (segment < 8)
    {
    a_val = segment << 4;
    if (segment < 2)
        a_val |= (sample >> 1) & 0x0F;
    else
        a_val |= (sample >> segment) & 0x0F;
    }

    return (unsigned char) (a_val ^ mask);
}


unsigned char st_linear_to_ulaw(int sample)
{
    static int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                   4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
    int sign, exponent, mantissa;
    unsigned char ulawbyte;

    /* Get the sample into sign-magnitude. */
    sign = (sample >> 8) & SIGN_BIT;          /* set aside the sign */
    if (sign != 0) sample = -sample;          /* get magnitude */
    if (sample > CLIP) sample = CLIP;         /* clip the magnitude */

    /* Convert from 16 bit linear to ulaw. */
    sample = sample + BIAS;
    exponent = exp_lut[(sample >> 7) & 0xFF];
    mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F;
    ulawbyte = ~ ( sign | ( exponent << 4 ) | mantissa );
    if (ZEROTRAP && (ulawbyte == 0)) {      /* optional CCITT trap */
    ulawbyte = 0x02;
    }
    return (ulawbyte);
}


char TOOSMALL[] = "snd_convert: destination buffer too small";

/* snd_convert -- copy from buffer2 to buffer1, converting samples if necessary */
/*
 */
long snd_convert(snd_type snd1, void *buffer1,
    snd_type snd2, void *buffer2, long length)
{
    /* simplest conversion: none */
    if (snd1->format.channels == snd2->format.channels &&
    snd1->format.mode == snd2->format.mode &&
    snd1->format.bits == snd2->format.bits &&
    snd1->format.srate == snd2->format.srate) {
    memcpy(buffer1, buffer2, length * snd_bytes_per_frame(snd1));
    return length;
    }

    /* reject if sample rates do not match */
    if (snd1->format.srate != snd2->format.srate) {
    snd_fail("snd_convert: cannot convert sample rates");
    return 0L;
    }

    /* convert from anything to float */
    if (snd1->format.mode == SND_MODE_FLOAT /* implies 32-bit */ &&
    snd1->format.channels == snd2->format.channels) {
    cvtfn_type cvtfn = select_cvtfn(snd1, snd2);
    float peak = 0.0F;
    return (*cvtfn)(buffer1, buffer2, length * snd1->format.channels, 
                        1.0F, &peak) / snd1->format.channels;
    }

    /* convert from float to anything */
    if (snd2->format.mode == SND_MODE_FLOAT /* implies 32-bit */ &&
    snd1->format.channels == snd2->format.channels) {
    cvtfn_type cvtfn = select_cvtfn(snd1, snd2);
    float peak = 0.0F;
    return (*cvtfn)(buffer1, buffer2, length * snd1->format.channels,
                1.0F, &peak) / snd1->format.channels;
    }

    /* otherwise, we need to do a full conversion to and from float */
    return snd_convert2(snd1, buffer1, snd2, buffer2, length);
}


long snd_bytes_per_frame(snd_type snd)
{
    return (((snd->format.bits + 7) >> 3) * snd->format.channels);
}


long to_mono(void *buffer, long length, long channels)
{
    float *ptr = (float *) buffer;
    float *out = ptr;
    float inverse = 1.0F / channels;
    long result = length;    
    while (length > 0) {
    float sum = 0.0F;
    int i;
    for (i = 0; i < channels; i++) {
        sum += ptr[i];
    }
    *out++ = sum * inverse;
    ptr += channels;
    length--;
    }
    return result;
}


long from_mono(void *buffer, long length, long channels)
{
    float *outptr = ((float *) buffer) + (length * channels) - 1;
    float *inptr = ((float *) buffer) + length - 1;

    /* copy backwards to avoid overwriting data you need */
    while (inptr >= (float *) buffer) {
    int i;
    float s = *inptr--;
    for (i = 0; i < channels; i++) {
            *outptr-- = s;
    }
    }
    return length;
}


long snd_convert2(snd_type snd1, void *buffer1, 
              snd_type snd2, void *buffer2, long length)
{
    char *buf1 = (char *) buffer1;
    char *buf2 = (char *) buffer2;

/* how many floats in one temporary block? */
#define blocklen 1024
/* how many bytes in temporary block? */
#define blockbytes (blocklen * sizeof(float))
    float temp[blocklen];

    /* how many frames converted via temp block every iteration? */
    long frames_per_iteration = blocklen / max(snd1->format.channels, 
                           snd2->format.channels);

    snd_node temp_snd2;

    temp_snd2.format = snd2->format;
    temp_snd2.format.mode = SND_MODE_FLOAT;
    temp_snd2.format.bits = 32;
    temp_snd2.format.channels = snd2->format.channels; /* must match */

    while (length > 0) {
        long temp_len;
    /* First convert from buffer2 to floats in temp.
     * Channel counts will match even though they may not be what we want
     * in the end.
     */
        temp_snd2.format.channels = snd2->format.channels;
    temp_len = snd_convert(&temp_snd2, temp,
        snd2, buf2, min(length, frames_per_iteration));
        /* now temp contains temp_len frames of floats, same channels as snd2 */

    /* figure out if channels match */
    if (snd1->format.channels != snd2->format.channels) {
        /* no match, so first convert to mono */
        temp_len = to_mono(temp, temp_len, snd2->format.channels);

        /* then, if dest isn't mono, duplicate mono on all channels */
        if (snd1->format.channels > 1) {
        temp_len = from_mono(temp, temp_len, snd1->format.channels);
        }
    }
        /* now temp contains temp_len frames with same channels as snd1 */

    /* convert from temp to buffer1, again ignoring channel count */
    temp_snd2.format.channels = snd1->format.channels;
    temp_len = snd_convert(snd1, buf1,
                   &temp_snd2, temp, temp_len);

    /* adjust pointers etc. */
    buf1 += temp_len * snd_bytes_per_frame(snd1);
    length -= temp_len;
    buf2 += temp_len * snd_bytes_per_frame(snd2);
    }
    return (buf1 - (char *) buffer1) / snd_bytes_per_frame(snd1);
}


long cvt_from_alaw_8 (void *buf1, void *buf2, long len2, 
                      float scale, float *peak)
{
    float *floats = (float *) buf1;
    unsigned char *bytes = (unsigned char *) buf2;
    long i;
    int pcm_val, a_val;
    unsigned segment;

    scale /= SCALE_FACTOR_TO_SHORT;
    for (i = 0; i < len2; i++) {
    a_val = bytes[i] ^ 0x55;

    pcm_val = ((a_val & 0x0F) << 4) + 8;
    segment = (a_val & 0x70) >> 4;
    if (segment != 0)
        pcm_val = (pcm_val + 0x100) << (segment - 1);

    *floats++ = ((a_val & 0x80) ? pcm_val : -pcm_val) * scale;
    }

    return len2;;
}


long cvt_from_ulaw_8(void *buf1, void *buf2, long len2, 
                     float scale, float *peak)
{
    float *floats = (float *) buf1;
    unsigned char *bytes = (unsigned char *) buf2;
    long i;

    scale /= SCALE_FACTOR_TO_SHORT;
    for (i = 0; i < len2; i++) {
    *floats++ = ulaw_table[((int) (*bytes++))] * scale;
    }
    return len2;
}


long cvt_from_upcm_8(void *buf1, void *buf2, long len2, 
                     float scale, float *peak)
{
    float *floats = (float *) buf1;
    unsigned char *bytes = (unsigned char *) buf2;
    int i;
    scale /= SCALE_FACTOR_TO_BYTE;
    for (i = 0; i < len2; i++) {
    *floats++ = (((int) (*bytes++)) - 128) * scale;
    }
    return len2;
}


long cvt_from_pcm_8(void *buf1, void *buf2, long len2, 
                    float scale, float *peak)
{
    float *floats = (float *) buf1;
    unsigned char *bytes = (unsigned char *) buf2;
    int i;
    scale /= SCALE_FACTOR_TO_BYTE;
    for (i = 0; i < len2; i++) {
    *floats++ = (((int) (*bytes++) ^ 128) - 128) * scale;
    }
    return len2;
}


long cvt_from_pcm_16(void *buf1, void *buf2, long len2, 
                     float scale, float *peak)
{
    float *floats = (float *) buf1;
    short *shorts = (short *) buf2;
    int i;
    scale /= SCALE_FACTOR_TO_SHORT;
    for (i = 0; i < len2; i++) {
    *floats++ = *shorts++ * scale;
    }
    return len2;
}


long cvt_from_pcm_24(void *buf1, void *buf2, long len2,
                     float scale, float *peak)
{
    float *floats = (float *) buf1;
    unsigned char *bytes = (unsigned char *) buf2;
    long temp;
    unsigned char *temp_bytes = (unsigned char *) &temp;
    int i;
    scale /= SCALE_FACTOR_TO_24BIT;
    for (i = 0; i < len2; i++) {
// since there are no natural 24 bit data types, our scheme of writing
// ints or shorts "natively" and taking care of swapping elsewhere does
// not really work with 24-bit data. To be consistent, we copy the data
// maintaining byte-order, but with little-endian, we need to load the
// data into the beginning of the word, and with big-endian we need to
// load the data into the end of the word. That way, the data ends up
// in the low-order bits of a long. Getting the bytes in the correct
// order is taken care of by the swap code that is run elsewhere.
#ifdef __LITTLE_ENDIAN__
        // pack bytes into higher bytes so that sign bit is set
        temp_bytes[1] = *bytes++;
        temp_bytes[2] = *bytes++;
        temp_bytes[3] = *bytes++;
        temp >>= 8; // shift down to low order bits, extending sign
#else
        temp_bytes[0] = *bytes++;
        temp_bytes[1] = *bytes++;
        temp_bytes[2] = *bytes++;
        temp >>= 8; // shift down to low order bits, extending sign
#endif
        *floats++ = temp * scale;
    }
    return len2;
}


long cvt_from_float_32(void *buf1, void *buf2, long len2, float scale, float *peak)
{
    float *floats1 = (float *) buf1;
    float *floats2 = (float *) buf2;
    int i;
    for (i = 0; i < len2; i++) {
    *floats1++ = *floats2++ * scale;
    }
    return len2;
}


long cvt_from_pcm_32(void *buf1, void *buf2, long len2, 
                     float scale, float *peak)
{
    float *floats = (float *) buf1;
    long *longs = (long *) buf2;
    int i;
    scale /= SCALE_FACTOR_TO_LONG;
    for (i = 0; i < len2; i++) {
    *floats++ = *longs++ * scale;
    }
    return len2;
}


long cvt_from_unknown(void *buf1, void *buf2, long len2, 
                      float scale, float *peak)
{
    snd_fail("snd_convert: unknown conversion");
    return 0;
}

/* snd.h says: #define cvt_to_unknown cvt_from_unknown */


long cvt_to_alaw_8 (void *buf1, void *buf2, long len2,
                    float scale, float *peak)
{
    unsigned char *bytes = (unsigned char *) buf1;
    float *floats = (float *) buf2;
    long i;
    FASTFLOAT s, max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_SHORT;

    for (i = 0; i < len2; i++) {
    s = scale * floats[i];
    if (s > 0) {
        if (s > max_sample)	{
        max_sample = s;
        }
        s += 0.5;
    } else {
        if (-s > max_sample) {
        max_sample = -s;
        }
        s -= 0.5;
    }
    bytes[i] = st_linear_to_ulaw ((int) s);
    }

    *peak = (float) (max_sample / SCALE_FACTOR_TO_SHORT);
    return len2;
}


long cvt_to_ulaw_8(void *buf1, void *buf2, long len2, 
                   float scale, float *peak)
{
    unsigned char *bytes = (unsigned char *) buf1;
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_SHORT;
    
    for (i = 0; i < len2; i++) {
    FASTFLOAT s = *floats++ * scale;
    /* round down, e.g. -4.5 => -5: */
    if (s > 0) {
        if (s > max_sample) max_sample = s;
        s += 0.5;
    } else {
        if (-s > max_sample) max_sample = -s;
        s -= 0.5;
    }
    *bytes++ = st_linear_to_ulaw((int) s);
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_SHORT);
    return len2;
}


long cvt_to_upcm_8(void *buf1, void *buf2, long len2, 
                   float scale, float *peak)
{
    unsigned char *bytes = (unsigned char *) buf1;
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_BYTE;
    
    for (i = 0; i < len2; i++) {
    FASTFLOAT s = *floats++ * scale;
    /* round down, e.g. -4.5 => -5: */
    if (s > 0) {
        if (s > max_sample) max_sample = s;
        s += 0.5;
    } else {
        if (-s > max_sample) max_sample = -s;
        s -= 0.5;
    }
    *bytes++ = ((int) s) ^ 128;
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_BYTE);
    return len2;
}


long cvt_to_pcm_8(void *buf1, void *buf2, long len2, 
                  float scale, float *peak)
{
    unsigned char *bytes = (unsigned char *) buf1;
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    len2 = len2;
    scale *= SCALE_FACTOR_TO_BYTE;
    
    for (i = 0; i < len2; i++) {
    FASTFLOAT s = *floats++ * scale;
    /* round down, e.g. -4.5 => -5: */
    if (s > 0) {
        if (s > max_sample) max_sample = s;
        s += 0.5;
    } else {
        if (-s > max_sample) max_sample = -s;
        s -= 0.5;
    }
    *bytes++ = (int) s;
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_BYTE);
    return len2;
}


long cvt_to_upcm_16(void *buf1, void *buf2, long len2,
                    float scale, float *peak)
{
    short *shorts = (short *) buf1;
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_SHORT;
    
    for (i = 0; i < len2; i++) {
    FASTFLOAT s = *floats++ * scale;
    /* round down, e.g. -4.5 => -5: */
    if (s > 0) {
        if (s > max_sample) max_sample = s;
        s += 0.5;
    } else {
        if (-s > max_sample) max_sample = -s;
        s -= 0.5;
    }
    *shorts++ = ((short) s) ^ 32768;       // dmazzoni
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_SHORT);
    return len2;
}


long cvt_to_pcm_16(void *buf1, void *buf2, long len2, 
                   float scale, float *peak)
{
    short *shorts = (short *) buf1;
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_SHORT;
    
    for (i = 0; i < len2; i++) {
        FASTFLOAT s = *floats++ * scale;
        /* round down, e.g. -4.5 => -5: */
        if (s > 0) {
            if (s > max_sample) max_sample = s;
            s += 0.5;
        } else {
            if (-s > max_sample) max_sample = -s;
            s -= 0.5;
        }
        *shorts++ = (short) s;
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_SHORT);
    return len2;
}


long cvt_to_upcm_24(void *buf1, void *buf2, long len2,
                    float scale, float *peak)
{
    unsigned char *bytes = (unsigned char *) buf1;
    float *floats = (float *) buf2;
    long samp;
    unsigned char *samp_bytes = (unsigned char *) &samp;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_24BIT;
    
    for (i = 0; i < len2; i++) {
        FASTFLOAT s = *floats++ * scale;
        /* round down, e.g. -4.5 => -5: */
        if (s > 0) {
            if (s > max_sample) max_sample = s;
            s += 0.5;
        } else {
            if (-s > max_sample) max_sample = -s;
            s -= 0.5;
        }
        samp = ((long) s) ^ 0x800000;
#ifdef __LITTLE_ENDIAN__
        *bytes++ = samp_bytes[0];
        *bytes++ = samp_bytes[1];
        *bytes++ = samp_bytes[2];
#else
        *bytes++ = samp_bytes[1];
        *bytes++ = samp_bytes[2];
        *bytes++ = samp_bytes[3];
#endif
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_SHORT);
    return len2;
}

long cvt_to_pcm_24(void *buf1, void *buf2, long len2,
                    float scale, float *peak)
{
    unsigned char *bytes = (unsigned char *) buf1;
    float *floats = (float *) buf2;
    long samp;
    unsigned char *samp_bytes = (unsigned char *) &samp;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_24BIT;
    
    for (i = 0; i < len2; i++) {
        FASTFLOAT s = *floats++ * scale;
        /* round down, e.g. -4.5 => -5: */
        if (s > 0) {
            if (s > max_sample) max_sample = s;
            s += 0.5;
        } else {
            if (-s > max_sample) max_sample = -s;
            s -= 0.5;
        }
        samp = (long) s;
#ifdef __LITTLE_ENDIAN__
        *bytes++ = samp_bytes[0];
        *bytes++ = samp_bytes[1];
        *bytes++ = samp_bytes[2];
#else
        *bytes++ = samp_bytes[1];
        *bytes++ = samp_bytes[2];
        *bytes++ = samp_bytes[3];
#endif
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_SHORT);
    return len2;
}



long cvt_to_float_32(void *buf1, void *buf2, long len2, float scale, float *peak)
{
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    
    for (i = 0; i < len2; i++) {
        FASTFLOAT s = *floats++ * scale;
        if (s > max_sample)
            max_sample = s;
        if (-s > max_sample)
            max_sample = -s;
        *floats++ = (float) s;
    }
    *peak = (float) max_sample;

    return len2;
}


long cvt_to_upcm_32(void *buf1, void *buf2, long len2,
                    float scale, float *peak)
{
    long *longs = (long *) buf1;
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_LONG;
    
    for (i = 0; i < len2; i++) {
    FASTFLOAT s = *floats++ * scale;
    /* round down, e.g. -4.5 => -5: */
    if (s > 0) {
        if (s > max_sample) max_sample = s;
        s += 0.5;
    } else {
        if (-s > max_sample) max_sample = -s;
        s -= 0.5;
    }
    *longs++ = ((long) s) ^ (1 << 31); // dmazzoni
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_LONG);
    return len2;
}


long cvt_to_pcm_32(void *buf1, void *buf2, long len2, 
                   float scale, float *peak)
{
    long *longs = (long *) buf1;
    float *floats = (float *) buf2;
    int i;
    FASTFLOAT max_sample = 0.0;
    scale *= SCALE_FACTOR_TO_LONG;
    
    for (i = 0; i < len2; i++) {
    FASTFLOAT s = *floats++ * scale;
    /* round down, e.g. -4.5 => -5: */
    if (s > 0) {
        if (s > max_sample) max_sample = s;
        s += 0.5;
    } else {
        if (-s > max_sample) max_sample = -s;
        s -= 0.5;
    }
    *longs++ = (long) s;
    }
    *peak = (float) (max_sample / SCALE_FACTOR_TO_LONG);
    return len2;
}


cvtfn_type cvt_from_8[] = { cvt_from_unknown, cvt_from_pcm_8, cvt_from_ulaw_8, cvt_from_alaw_8, cvt_from_unknown, cvt_from_upcm_8, cvt_from_unknown };

cvtfn_type cvt_from_16[] = { cvt_from_unknown, cvt_from_pcm_16, cvt_from_unknown, cvt_from_unknown, cvt_from_unknown, cvt_from_unknown, cvt_from_unknown };

cvtfn_type cvt_from_24[] = { cvt_from_unknown, cvt_from_pcm_24, cvt_from_unknown, cvt_from_unknown, cvt_from_unknown, cvt_from_unknown, cvt_from_unknown };

cvtfn_type cvt_from_32[] = { cvt_from_unknown, cvt_from_pcm_32, cvt_from_unknown, cvt_from_unknown, cvt_from_float_32, cvt_from_unknown, cvt_from_unknown };

cvtfn_type cvt_to_8[] = { cvt_to_unknown, cvt_to_pcm_8, cvt_to_ulaw_8, cvt_to_alaw_8, cvt_to_unknown, cvt_to_upcm_8, cvt_to_unknown };

cvtfn_type cvt_to_16[] = { cvt_to_unknown, cvt_to_pcm_16, cvt_to_unknown, cvt_to_unknown, cvt_to_unknown, cvt_to_upcm_16, cvt_to_unknown };

cvtfn_type cvt_to_24[] = { cvt_to_unknown, cvt_to_pcm_24, cvt_to_unknown, cvt_to_unknown, cvt_to_unknown, cvt_to_upcm_24, cvt_to_unknown };

cvtfn_type cvt_to_32[] = { cvt_to_unknown, cvt_to_pcm_32, cvt_to_unknown, cvt_to_unknown, cvt_to_float_32, cvt_to_upcm_32, cvt_to_unknown };


/* select_cvtfn -- find conversion from snd2 to snd1 */
/*
 * conversion is based on bits and mode (channels and srate
 * are ignored). Either snd1 or snd2 must be of SND_MODE_FLOAT.
 */
cvtfn_type select_cvtfn(snd_type snd1, snd_type snd2)
{
    long mode1 = snd1->format.mode;
    long mode2 = snd2->format.mode;
    if (mode1 == SND_MODE_FLOAT) {
    long bits2 = snd2->format.bits;
    /* convert_from functions */
    if (bits2 == 8) return cvt_from_8[mode2];
    else if (bits2 == 16) return cvt_from_16[mode2];
    else if (bits2 == 24) return cvt_from_24[mode2];
    else if (bits2 == 32) return cvt_from_32[mode2];
    else return &cvt_from_unknown;
    } else if (mode2 == SND_MODE_FLOAT) {
    long bits1 = snd1->format.bits;
    /* convert_to functions */
    if (bits1 == 8) return cvt_to_8[mode1];
    else if (bits1 == 16) return cvt_to_16[mode1];
    else if (bits1 == 24) return cvt_to_24[mode1];
    else if (bits1 == 32) return cvt_to_32[mode1];
    else return &cvt_to_unknown;
    } else return &cvt_from_unknown;
}
