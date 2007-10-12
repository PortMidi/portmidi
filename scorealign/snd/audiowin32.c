/* Standard includes */

#include "stdlib.h"
#include "stdio.h"

/* snd includes */

#include "snd.h"
#include "audiowin32.h"

int audio_reset(snd_type snd);

/* for timeBeginPeriod and timeEndPeriod, in case of SND_REALTIME device 
 * handling
 */
int rt_devices_open = 0; 

/* also for RT devices */
#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

/* also (wTimerRes will contain the resolution that we can get */
TIMECAPS tc;
UINT     wTimerRes;


int audio_formatmatch(format_node *demanded, DWORD avail, long *flags);
int test_44(DWORD tested);
int test_22(DWORD tested);
int test_11(DWORD tested);
int test_stereo(DWORD tested); 
int test_mono(DWORD tested); 
int test_16bit(DWORD tested);
int test_8bit(DWORD tested);
MMRESULT win_wave_open(snd_type snd, UINT devtoopen, HWAVE *hptr);
void *audio_descr_build(snd_type snd);
void mm_error_handler(snd_type snd, MMRESULT mmerror, void (*fp)());
int numofbits(long tested);
int audio_dev(snd_type snd, char *name, UINT *device);


long audio_poll(snd_type snd)
{
    long availtemp;
    buffer_state *cur = ((buffer_state*) snd->u.audio.descriptor);
    LPWAVEHDR whdrs = cur->whdr;
    int i, test = 0; 
    int numbuf = cur->numofbuffers;

    for (i=0; i<numbuf; i++){ 
        if (whdrs[i].dwFlags & WHDR_DONE){
            test++;
        }
    }

    if (snd->u.audio.protocol == SND_REALTIME) {
        int last_buf, k;

        if (test < numbuf) { 
            /* there is at least one buffer being processed by the 
             * device (not "done")
               */
            if (test) { /* there is at last one buffer done */
                long bufferbytes;
                if (whdrs[cur->sync_buffer].dwFlags & WHDR_DONE){
                    k = (cur->sync_buffer + 1) % numbuf;
                    while (k != cur->sync_buffer){
                        if (!(whdrs[k].dwFlags & WHDR_DONE)){
                            cur->sync_buffer = k;
                            break;
                        }
                        k = (k + 1) % numbuf;
                    }
                    cur->sync_time = cur->prev_time;
                } else 
                    cur->prev_time = timeGetTime();
                availtemp = (long)
                    (((double)(cur->prev_time - cur->sync_time) *
                     max_srate_dev / 1000 
                     /*+ 2 * snd->u.audio.latency*/) *
                     snd->format.srate);
                last_buf = (cur->sync_buffer + numbuf - 1) % numbuf;
                while (last_buf != cur->pollee) {
                    availtemp += whdrs[last_buf].dwBufferLength;
                    last_buf = (last_buf + numbuf - 1) % numbuf;
                }
                bufferbytes = test * (long) whdrs[0].dwBufferLength;
                if (bufferbytes < availtemp * snd_bytes_per_frame(snd)) {
                    return bufferbytes / snd_bytes_per_frame(snd);
                } else {
                    return availtemp;
                }
            } else { 
                /* all buffer already passed to the device, it 
                 * should happen only just because we "overestimate"
                 * the sample-rate
                  */
                cur->prev_time = timeGetTime();
                cur->sync_time = cur->prev_time;
                return 0;
            }
        } else { //all buffers are done
            cur->prev_time = timeGetTime();
            cur->sync_time = cur->prev_time;
            return ((numbuf - 1) * whdrs[0].dwBufferLength) / 
                   snd_bytes_per_frame(snd);
        }
    } else { /* protocol SND_COPMUTEAHEAD */
        /* here is suppose that all buffer have the same length 
         * (otherwise, sg like "for(i=0;i<numbuf;i++) 
         * available+=whdrs[i].dwBufferLength" should be used)
         */             
        return (test * whdrs[0].dwBufferLength - cur->posinbuffer) / 
               snd_bytes_per_frame(snd);
    }
}


long audio_read(snd_type snd, void *buffer, long length)
{
    long i = 0, processed = 0, n;       
    buffer_state *cur = (buffer_state *) snd->u.audio.descriptor;
    // int *index = &(cur->pollee);
    unsigned int *posinb = (unsigned int *) &(cur->posinbuffer);
    MMRESULT er;
    byte *in;
    byte *out;
    
    in = (byte *) (cur->whdr[cur->pollee].lpData + cur->posinbuffer);
    out = (byte *) buffer;
    
    while (length) {
        if (cur->whdr[cur->pollee].dwFlags & WHDR_DONE) {

#if WHY_DOESNT_THIS_WORK
            // This seems correct and does work under NT, but sometimes
            // under Win95 and Win98 this generates an error. Although
            // we can't explain why the code is not necessary, everything
            // seems to work if we just comment this out. -RBD
            if (cur->posinbuffer == 0) {
                if (er = waveInUnprepareHeader(cur->u.h_in, 
                                               &(cur->whdr[cur->pollee]), 
                                               sizeof(WAVEINHDR))) {
                    mm_error_handler(snd, er, snd_fail);
                    return processed;
                }
            }
#endif // WHY_DOESNT_THIS_WORK
            n = min((long) (cur->whdr[cur->pollee].dwBufferLength - 
                         cur->posinbuffer), length);
            memcpy(out, in, n);
            processed += n;
            cur->posinbuffer += n;
            length -= n;
            out += n;

            if (cur->posinbuffer >= 
                (int) cur->whdr[cur->pollee].dwBufferLength) {
                if (er = waveInPrepareHeader(cur->u.h_in, 
                                             &(cur->whdr[cur->pollee]), 
                                             sizeof(WAVEHDR))) {
                    mm_error_handler(snd, er, snd_fail);
                    return(processed);
                }
                if (er = waveInAddBuffer(cur->u.h_in, &(cur->whdr[cur->pollee]),
                                         sizeof(WAVEHDR))) {
                    waveInUnprepareHeader(cur->u.h_in, 
                                          &(cur->whdr[cur->pollee]), 
                                          sizeof(WAVEHDR));
                    mm_error_handler(snd, er, snd_fail);
                    return processed;
                }
                //maybe the following line isn't necessary
                cur->posinbuffer = 0;
                if (++(cur->pollee) >= cur->numofbuffers){
                    cur->pollee = 0;
                }
                in = (byte *) (cur->whdr[cur->pollee].lpData);
            }
        }
    }

    return processed;
}


long audio_write(snd_type snd, void *buffer, long length)
{
    long i = 0, processed = 0, n;   
    buffer_state *cur = (buffer_state *)snd->u.audio.descriptor;
    unsigned int *posinb = (unsigned int *) &(cur->posinbuffer);
    MMRESULT er;
    byte *in;
    byte *out;

    in = (byte *) buffer;
    out = (byte *) (cur->whdr[cur->pollee].lpData + cur->posinbuffer);
    
    while (length) {
        if (cur->whdr[cur->pollee].dwFlags & WHDR_DONE) {

#if WHY_DOESNT_THIS_WORK
            // This seems correct and does work under NT, but sometimes
            // under Win95 and Win98 this generates an error. Although
            // we can't explain why the code is not necessary, everything
            // seems to work if we just comment this out. -RBD
            if (cur->posinbuffer == 0) {
                if (er = waveOutUnprepareHeader(cur->h_wave, 
                                                &(cur->whdr[cur->pollee]), 
                                                sizeof(WAVEOUTHDR))) {
                    mm_error_handler(snd, er, snd_fail);
                    return processed;
                }
            }
#endif // WHY_DOESNT_THIS_WORK
            n = min((long)(cur->whdr[cur->pollee].dwBufferLength - 
                    cur->posinbuffer), length);
            memcpy(out, in, n);
            processed += n;
            cur->posinbuffer += n;
            length -= n;
            in += n;

            if (cur->posinbuffer >= 
                (int) cur->whdr[cur->pollee].dwBufferLength) {
                if (er = waveOutPrepareHeader(cur->u.h_out, 
                                              &(cur->whdr[cur->pollee]), 
                                              sizeof(WAVEHDR))) {
                    mm_error_handler(snd, er, snd_fail);
                    return processed;
                }
                if (er = waveOutWrite(cur->u.h_out, &(cur->whdr[cur->pollee]),
                                      sizeof(WAVEHDR))) {
                    waveOutUnprepareHeader(cur->u.h_out,
                                           &(cur->whdr[cur->pollee]), 
                                           sizeof(WAVEHDR));
                    mm_error_handler(snd, er, snd_fail);
                    return processed;
                }
                //maybe the following line isn't necessary
                cur->posinbuffer = 0;
                if (++(cur->pollee) >= cur->numofbuffers) {
                    cur->pollee = 0;
                }
                out = (byte*)(cur->whdr[cur->pollee].lpData);
            }
        }
    }

    return processed;
}


int audio_open(snd_type snd, long *flags)
{
    UINT devtoopen;
    WAVEOUTCAPS outspec;
    WAVEINCAPS inspec;
    MMRESULT er;
    HWAVE h;

    if (snd->u.audio.devicename[0] != '\000'){
        if (audio_dev(snd, snd->u.audio.devicename, &devtoopen)){
            return(!SND_SUCCESS);
        }
    } else {
        devtoopen = WAVE_MAPPER; //open what you find
    }
    er = win_wave_open(snd, devtoopen, &h);
    if (er != (MMRESULT) MMSYSERR_NOERROR) {
        if (er != WAVERR_BADFORMAT) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        // The specified format isn't supported around here. 
        // Let's try to use another one.
        if (devtoopen == WAVE_MAPPER) {
            UINT i, n, bestdev;
            format_node tempformat, bestformat = snd->format;
            long bestmatch = ((SND_HEAD_CHANNELS | 
                               SND_HEAD_MODE |
                               SND_HEAD_BITS |
                               SND_HEAD_SRATE) << 1) + 1;
            if (snd->write_flag == SND_READ) {
                n = waveInGetNumDevs();
                for (i = 0; i < n; i++) {
                    tempformat = snd->format;
                    // non-zero means an error occurred:
                    if (er = waveInGetDevCaps(i, &inspec,
                                              sizeof(WAVEINCAPS))) {
                        char msg[64];
                        sprintf(msg, "Error at the %i device: %d\n", 
                                                    i,        er);
                        snd_fail(msg);
                    } else {
                        if (audio_formatmatch(&tempformat, inspec.dwFormats, 
                                flags) != SND_SUCCESS) {
                            char msg[64];
                            sprintf(msg, 
                             "Problem with FormatMatching at device %i\n", i);
                        } else if (numofbits(*flags) < numofbits(bestmatch)) {
                            bestmatch = *flags;
                            bestformat = tempformat;
                            bestdev = i;
                        }
                    }
                }
            } else {
                n = waveOutGetNumDevs();
                for (i = 0; i < n; i++) {
                    tempformat = snd->format;
                    // non-zero means an error occurred:
                    if (er = waveOutGetDevCaps(i, &outspec,
                                               sizeof(WAVEOUTCAPS))) {
                        char msg[64];
                        sprintf(msg, "Error at the %i device: %d\n", 
                                                    i,        er);
                        snd_fail(msg);
                    } else {
                        if (audio_formatmatch(&tempformat, outspec.dwFormats, 
                                flags) != SND_SUCCESS) {
                            char msg[64];
                            sprintf(msg, 
                             "Problem with FormatMatching at device %i\n", i);
                        } else if (numofbits(*flags) < numofbits(bestmatch)) {
                            bestmatch = *flags;
                            bestformat = tempformat;
                            bestdev = i;
                        }
                    }
                }
            }
            if (bestmatch == ((long)-1)) {
                // We weren't able to pick up a device. Let's quit.
                return !SND_SUCCESS;
            }
            snd->format = bestformat;
            *flags = bestmatch;
            devtoopen = bestdev;
        // not devtoopen == WAVE_MAPPER, so open specified device, get format:
        } else if (snd->write_flag == SND_READ) {
            if (er = waveInGetDevCaps(devtoopen, &inspec, 
                                      sizeof(WAVEINCAPS))) {
                mm_error_handler(snd, er, snd_fail);
                return !SND_SUCCESS;
            }
            if (audio_formatmatch(&snd->format, inspec.dwFormats, flags)) {
                snd_fail("Something went wrong with FormatMatching");
                return !SND_SUCCESS;
            }
        } else { // snd->write_flag != SND_READ
            if (er = waveOutGetDevCaps(devtoopen, &outspec, 
                                      sizeof(WAVEOUTCAPS))) {
                mm_error_handler(snd, er, snd_fail);
                return !SND_SUCCESS;
            }
            if (audio_formatmatch(&snd->format, outspec.dwFormats, flags)) {
                snd_fail("Something went wrong with FormatMatching");
                return !SND_SUCCESS;
            }
        }
        if (er = win_wave_open(snd, devtoopen, &h)) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
    }

    if (snd->u.audio.protocol == SND_REALTIME && !rt_devices_open) {
        if (er = timeGetDevCaps(&tc, sizeof(TIMECAPS))) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        wTimerRes = min(max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);
        
        if (er = timeBeginPeriod(wTimerRes)) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        rt_devices_open++;
    }        

    snd->u.audio.descriptor = audio_descr_build(snd);
    if (snd->u.audio.descriptor == NULL) {
        snd_fail("Unable to create Device-state descriptor");
        return !SND_SUCCESS;
    }
    ((buffer_state *) snd->u.audio.descriptor)->u.h_in = (HWAVEIN) h;

    if (snd->write_flag == SND_READ) {
        audio_reset(snd);
        // audio buffers should be passed to the device here, 
        // like in the Reset() function
    }

    return SND_SUCCESS;
}


int audio_close(snd_type snd)
{
    int i;
    MMRESULT er;
    buffer_state *bs = (buffer_state *) snd->u.audio.descriptor;
    if (snd->write_flag == SND_READ) {
        if (er = waveInReset(bs->u.h_in)) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        if (er = waveInClose(bs->u.h_in)) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
    } else { // SND_WRITE
        if (er = waveOutReset(bs->u.h_out)) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        if (er = waveOutClose(bs->u.h_out)) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
    }
    for (i = 0; i < bs->numofbuffers; i++) {
        snd_free(bs->whdr[i].lpData);
    }
    snd_free(bs->whdr);
    snd_free(bs);
    if (rt_devices_open) {
        timeEndPeriod(1);
        rt_devices_open--;
    }
    return SND_SUCCESS;
}


/* audio_flush -- finish audio output */
/* 
 * if any buffer is non-empty, send it out
 * return success when all buffers have been returned empty
 */
int audio_flush(snd_type snd)
{
    buffer_state *cur = (buffer_state *) snd->u.audio.descriptor;
    int *index = &(cur->pollee);
    LPWAVEHDR whdrs = cur->whdr;
    int i = 0;
    int test = 0;
    int processed = 0; 
    int numbuf = cur->numofbuffers;
    MMRESULT er;
    
    if (snd->write_flag != SND_WRITE) return SND_SUCCESS;

    if (cur->posinbuffer > 0) { /* flush final partial buffer of data */
        cur->whdr[*index].dwBufferLength = cur->posinbuffer;
        if (er = waveOutPrepareHeader(cur->u.h_out, &(cur->whdr[*index]), sizeof(WAVEHDR))) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        if (er = waveOutWrite(cur->u.h_out, &(cur->whdr[*index]), sizeof(WAVEHDR))) {
            waveOutUnprepareHeader(cur->u.h_out, &(cur->whdr[*index]), sizeof(WAVEHDR));
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        cur->posinbuffer = 0;
        if (++(*index) >= cur->numofbuffers){
            *index = 0;
        }
    }

    for (i = 0; i < numbuf; i++){ 
        if (whdrs[i].dwFlags & WHDR_DONE) {
            test++;
        }
    }

    if (test < numbuf) return !SND_SUCCESS;
    else return SND_SUCCESS;
}


int audio_reset(snd_type snd)
{
    MMRESULT er;
    int i;
    buffer_state *cur = (buffer_state *)snd->u.audio.descriptor;

    cur->pollee = 0;
    cur->posinbuffer = 0;
    if (snd->write_flag == SND_READ){
        if (er = waveInReset(cur->u.h_in)) {
            mm_error_handler(snd, er, snd_fail);
            return (!SND_SUCCESS);
        }
        for (i=0; i<cur->numofbuffers; i++){
            if (er = waveInPrepareHeader (cur->u.h_in, &(cur->whdr[i]), sizeof(WAVEHDR))) {
                mm_error_handler(snd, er, snd_fail);
//this could be changed, as it's not necessery to return with error if at least one buffer didn't fail
                snd_fail("Not all of the buffers was initializable");
                return(!SND_SUCCESS);
            }
            if (er = waveInAddBuffer (cur->u.h_in, &(cur->whdr[i]), sizeof(WAVEHDR))) {
                waveInUnprepareHeader (cur->u.h_in, &(cur->whdr[i]), sizeof(WAVEHDR));
                mm_error_handler(snd, er, snd_fail);
                snd_fail("Not all of the buffers was initializable");
                return(!SND_SUCCESS);
            }
        }
        if (er = waveInStart(cur->u.h_in)) {
        mm_error_handler(snd, er, snd_fail);
        return (!SND_SUCCESS);
        }
    } else {
        if (er = waveOutReset(cur->u.h_out)) {
            mm_error_handler(snd, er, snd_fail);
            return (!SND_SUCCESS);
        }
    }
    if (snd->u.audio.protocol == SND_REALTIME){
        cur->sync_buffer = 0;
        cur->prev_time = timeGetTime();
        cur->sync_time = cur->prev_time - (long)(2 * 1000 * snd->u.audio.latency + .5);
    }
    return(SND_SUCCESS);
}



//to choose that format (of those the device can support) that is equal or highest and
//closest tp the one that the user required. Also, write out those parameters which were changed
int audio_formatmatch(format_node *demanded, DWORD avail, long *flags)
{
    *flags = 0;
    if (demanded->srate <= 11025.0){
        if (test_11(avail)){
            if (demanded->srate < 11025.0){
                *flags = *flags | SND_HEAD_SRATE;
                demanded->srate = 11025.0;
            }
        }else { 
            *flags = *flags | SND_HEAD_SRATE;
            if (test_22(avail))
                demanded->srate = 22050.0;
            else if (test_44(avail))
                demanded->srate = 44100.0;
            else {
                snd_fail("No available sample-rate (internal error)");
                return(!SND_SUCCESS);
            }
        }
    }else if (demanded->srate <= 22050.0) {
        if (test_22(avail)){
            if (demanded->srate < 22050.0){
                *flags = *flags | SND_HEAD_SRATE;
                demanded->srate = 22050.0;
            }
        }else { 
            *flags = *flags | SND_HEAD_SRATE;
            if (test_44(avail))
                demanded->srate = 44100.0;
            else if (test_11(avail))
                demanded->srate = 11025.0;
            else {
                snd_fail("No available sample-rate (internal error)");
                return(!SND_SUCCESS);
            }
        }
    }else if (demanded->srate <= 44100.0) {
        if (test_44(avail)){
            if (demanded->srate < 44100.0){
                *flags = *flags | SND_HEAD_SRATE;
                demanded->srate = 44100.0;
            }
        }else { 
            *flags = *flags | SND_HEAD_SRATE;
            if (test_22(avail))
                demanded->srate = 22050.0;
            else if (test_11(avail))
                demanded->srate = 11025.0;
            else {
                snd_fail("No available sample-rate (internal error)");
                return(!SND_SUCCESS);
            }
        }
    }else { /* srate > 44100 */
        *flags = *flags | SND_HEAD_SRATE;
        if (test_44(avail))
            demanded->srate = 44100.0;
        else if (test_22(avail))
            demanded->srate = 22050.0;
        else if (test_11(avail))
            demanded->srate = 11025.0;
        else {
            snd_fail("No available sample-rate (internal error)");
            return(!SND_SUCCESS);
        }
    }

    if (demanded->bits <= 8){
        if (test_8bit(avail)){
            if (demanded->bits < 8){
                *flags = *flags | SND_HEAD_BITS;
                demanded->bits = 8;
            }
        }else { 
            *flags = *flags | SND_HEAD_BITS;
            if (test_16bit(avail))
                demanded->bits = 16;
            else {
                snd_fail("No available bit number (internal error)");
                return(!SND_SUCCESS);
            }
        }
    }else { /* either between 8 and 16 bit, or higher than 16 bit -> use 16 bit */
        if (test_16bit(avail)){
            if (demanded->bits != 16){
                *flags = *flags | SND_HEAD_BITS;
                demanded->bits = 16;
            }
        }else { 
            *flags = *flags | SND_HEAD_BITS;
            if (test_8bit(avail))
                demanded->bits = 8;
            else {
                snd_fail("No available bit number (internal error)");
                return(!SND_SUCCESS);
            }
        }
    }

    if (demanded->channels == 1){
        if (!(test_mono(avail))){
            *flags = *flags | SND_HEAD_CHANNELS;
            if (test_stereo(avail))
                demanded->channels = 2;
            else {
                snd_fail("No available channels number (internal error)");
                return(!SND_SUCCESS);
            }
        }
    } else { /* otherwise, use stereo (for channels >= 2, or for invalid number, that is <1) */
        if (test_stereo(avail)){
            if (demanded->channels != 2){
                *flags = *flags | SND_HEAD_CHANNELS;
                demanded->channels = 2;
            }
        } else {
            *flags = *flags | SND_HEAD_CHANNELS;
            if (test_mono(avail))
                demanded->channels = 1;
            else {
                snd_fail("No available channels number (internal error)");
                return(!SND_SUCCESS);
            }
        }
    }

    if (demanded->mode != SND_MODE_PCM){
        *flags = *flags | SND_HEAD_MODE;
        demanded->mode = SND_MODE_PCM;
    }

    return (SND_SUCCESS);
}


//tests (based on dwFormats, see at WAVEOUTCAPS) that sr=44,1kHz is supported or not
int test_44(DWORD tested) 
{
    return(tested & (WAVE_FORMAT_4M08 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4S16));
}
 
int test_22(DWORD tested) 
{
    return(tested & (WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16));
}

int test_11(DWORD tested) 
{
    return(tested & (WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16));
}

int test_stereo(DWORD tested)
{
    return(tested & (    WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16 | 
                        WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16 | 
                        WAVE_FORMAT_4S08 | WAVE_FORMAT_4S16));
}
 
int test_mono(DWORD tested)
{
    return(tested & (    WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16 | 
                        WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16 | 
                        WAVE_FORMAT_4M08 | WAVE_FORMAT_4M16));
}
 
int test_16bit(DWORD tested)
{
    return(tested & (    WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 | 
                        WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 | 
                        WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16));
}
 
int test_8bit(DWORD tested)
{
    return(tested & (    WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | 
                        WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | 
                        WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08));
}

MMRESULT win_wave_open(snd_type snd, UINT devtoopen, HWAVE *hptr)
{
    WAVEFORMATEX wfmt;
    int bperf;

    switch (snd->format.mode) {
      case SND_MODE_UNKNOWN:
      case SND_MODE_PCM:
        /* note: Windows uses signed PCM (PCM) for 16-bit data, and
         * unsigned PCM (UPCM) for 8-bit data
         */
        if (snd->format.bits != 8) {
            wfmt.wFormatTag = WAVE_FORMAT_PCM;
            break;
        } else return WAVERR_BADFORMAT;    
      case SND_MODE_ADPCM:
      case SND_MODE_ALAW:
      case SND_MODE_ULAW:
      case SND_MODE_FLOAT:
      case SND_MODE_UPCM:
        if (snd->format.bits == 8) {
            wfmt.wFormatTag = WAVE_FORMAT_PCM;
            break;
        } else return(WAVERR_BADFORMAT);
      default:
        return(MMSYSERR_INVALPARAM);
    }    

    wfmt.nChannels       =  (unsigned short) snd->format.channels;
    wfmt.nSamplesPerSec  =  (long) (snd->format.srate + 0.5);
    bperf = snd_bytes_per_frame(snd);
    wfmt.nAvgBytesPerSec =  (long) (bperf * snd->format.srate + 0.5);
    wfmt.nBlockAlign     =  (unsigned short) bperf;
    wfmt.wBitsPerSample     =  (unsigned short) snd->format.bits;
    wfmt.cbSize            =  0;    
    if (snd->write_flag != SND_READ) {
        /* Under Windows2000 on an IBM A21p Thinkpad, waveOutOpen will 
           CRASH (!) if snd->format.srate is 110250, but it actually 
           worked with 4000, and it worked with 96000, so I will assume
           sample rates above 96000 are bad. -RBD 24Jul02
         */
        if (wfmt.nSamplesPerSec > 96000) {
            return WAVERR_BADFORMAT;
        }
        /* otherwise, we're ready to open the device */
        return waveOutOpen((LPHWAVEOUT) hptr, devtoopen, &wfmt, 0L, 0L, 
                           (DWORD) CALLBACK_NULL);
    } else {
        return waveInOpen((LPHWAVEIN) hptr, devtoopen, &wfmt, 0L, 0L,
                          (DWORD) CALLBACK_NULL);
    }
}

 
void *audio_descr_build(snd_type snd)
{    
    int i, j;
    buffer_state *bufstate;
    double bufsize_in_frames;

    bufstate = (buffer_state *) snd_alloc(sizeof(buffer_state));

    if (snd->u.audio.protocol == SND_REALTIME){
        bufstate->numofbuffers = 3;        
    }
    else { 
        if (snd->u.audio.protocol != SND_COMPUTEAHEAD)
            snd_fail("Invalid protocol identifier. COMPUTEAHEAD method used instead");
        bufstate->numofbuffers = 2;    
    }
    

    if (NULL == (bufstate->whdr = 
                (LPWAVEHDR)snd_alloc(bufstate->numofbuffers * sizeof(WAVEHDR)))){
        snd_fail("Not enough memory, or similar");
        snd_free(bufstate);
        return NULL;
    }
    bufsize_in_frames = max(Audio_out_min_buffersize, 
                            snd->format.srate * snd->u.audio.latency);
    if (bufsize_in_frames > snd->format.srate * snd->u.audio.latency)
        snd->u.audio.latency = bufsize_in_frames / snd->format.srate;
    for (i=0; i<bufstate->numofbuffers; i++){
        bufstate->whdr[i].dwBufferLength = (long)(bufsize_in_frames + 0.5) * snd_bytes_per_frame(snd); 
        if (NULL == (bufstate->whdr[i].lpData = (char *) snd_alloc(bufstate->whdr[i].dwBufferLength))){
            snd_fail("Not enough memory, or similar");
            for (j=0; j<i-1; j++){
                snd_free(bufstate->whdr[j].lpData);
            }
            snd_free(bufstate);
            return NULL;
        }
        bufstate->whdr[i].dwUser = bufstate->whdr[i].dwLoops = 0;    //idrk, what's that
        bufstate->whdr[i].dwFlags = WHDR_DONE;
    }

    bufstate->pollee = 0;
    bufstate->posinbuffer = 0;

    if (snd->u.audio.protocol == SND_REALTIME){
        bufstate->sync_buffer = 0;
        bufstate->prev_time = timeGetTime();
        bufstate->sync_time = bufstate->prev_time - (long)(2 * 1000 * snd->u.audio.latency + .5);
    }


    return (void *) bufstate;
}



void mm_error_handler(snd_type snd, MMRESULT mmerror, void (*fp)())
{
    char err[MAXERRORLENGTH];
    MMRESULT errnum;
    if (snd->write_flag == SND_READ) {
        errnum = waveInGetErrorText(mmerror, err, MAXERRORLENGTH);
    } else {
        errnum = waveOutGetErrorText(mmerror, err, MAXERRORLENGTH);
    }
    if (MMSYSERR_NOERROR != errnum) {
        switch (errnum) {
        case MMSYSERR_BADERRNUM:
            (*fp)("Specified error number is out of range.");
        case MMSYSERR_NODRIVER:
            (*fp)("No device driver is present.");
        case MMSYSERR_NOMEM:
            (*fp)("Unable to allocate or lock memory.");
        }
    }
    else
        (*fp)(err);
}



int numofbits(long tested)
{
    int r = 0;

    for (; tested; tested = tested >> 1){
        r = r + (tested & 1);
    }
    return r;
}

int audio_dev(snd_type snd, char *name, UINT *device)
{   
    UINT i, numofdev;
    MMRESULT er;
    WAVEOUTCAPS outspec;
    WAVEINCAPS inspec;
    char *devname;

    numofdev = (snd->write_flag == SND_READ ? waveInGetNumDevs() :
                                              waveOutGetNumDevs());
    if (numofdev <= 0) {
        snd_fail("There is no device at all.");
        return !SND_SUCCESS;
    }
    for (i = 0; i < numofdev; i++) {
        if (snd->write_flag == SND_READ) { 
              er = waveInGetDevCaps(i, &inspec, sizeof(WAVEINCAPS));
              devname = inspec.szPname;
        } else {
              er = waveOutGetDevCaps(i, &outspec, sizeof(WAVEOUTCAPS));
              devname = outspec.szPname;
        }
        if (er) {
            mm_error_handler(snd, er, snd_fail);
            return !SND_SUCCESS;
        }
        if (strcmp(name, devname) == 0) {
            /* printf("Device found: %s\n", devicespec.szPname); */
            *device = i;
            return SND_SUCCESS;
        }
    }
    snd_warn("There is no matching device, let's use an available one.");
    *device = WAVE_MAPPER;
    return SND_SUCCESS;
}


snd_fns_node mmsystem_dictionary = { audio_poll, audio_read, audio_write, 
                       audio_open, audio_close, audio_reset, audio_flush };


void snd_init()
{
    snd_add_device("MMSystem", "default", &mmsystem_dictionary);
}
