/*
 * audiomac.c
 *
 * Written by Dominic Mazzoni
 *
 * sndlib by Roger Dannenberg
 *
 * I rewrote this code in January 2001 to be compatible with both the old Mac OS
 * and the new CarbonLib required to run under Mac OS X.
 *
 * Brief description of algorithm: we keep two buffers.  After the first one is filled
 * we pass it to the Sound Manager and ask it to call our callback function as soon as
 * it has finished playing.  In the meantime we start filling the second buffer.
 * Whenever the first buffer completes, we copy the second buffer to the first buffer
 * and start playback on that buffer, unless the second buffer is empty, in which case
 * we play a small amount of silence, hoping the underlying process will catch up.
 *
 * Good references:
 *
 * http://devworld.apple.com/samplecode/Sample_Code/Sound/SndPlayDoubleBuffer.htm
 * http://devworld.apple.com/technotes/tn/tn1198.htm
 *
 */

#include <MacTypes.h>
#include <Sound.h>
#include "memory.h"
#include "stdio.h"

#include "snd.h"
#include "audiomac.h"

#include <FixMath.h>

#ifdef __cplusplus
extern "C" {
#endif

long audio_write(snd_node *n, void *buffer, long length);


pascal void playbackCallback(SndChannelPtr channel, SndCommand *theCallBackCmd)
{
  buffer_state *data = (buffer_state *)(theCallBackCmd->param2);
  
  if (data->busy) {
    SndDoCommand(channel, &data->callCmd, true);
    return;
  }
  
  data->busy = 1;

  // If there's data in the second buffer, copy it into the playback buffer
  // and mark the second buffer as empty again.

  if (data->curBuffer == 1 && data->curSize>0) {
    int bytes = data->curSize;

    if (bytes > data->bufferSize)
      bytes = data->bufferSize;

    BlockMove((Ptr)data->nextBuffer, (Ptr)data->buffer, bytes); 
    if (bytes != data->curSize)
      BlockMove((Ptr)&data->nextBuffer[bytes], (Ptr)data->nextBuffer, data->curSize - bytes);
       
    data->header.numFrames = bytes / data->frameSize;
    
    data->curSize -= bytes;
  }
  else {
    // even when we get here, the sound output is not completed, so we count
    // how many empty buffers we've sent. The flush command can return true
    // when this number is high enough.
      data->empty += 1; 
  
    // Otherwise, either we're finished playing or we're stalling
  
    if (!data->flushing) {
      // Send some silence through the speaker while we wait for
      // the program to catch up
      
      int waittime = 4096;
      int i;
      
      if (waittime > data->bufferSize)
        waittime = data->bufferSize;
      
      for(i=0; i<waittime / 2; i++)
          ((short *)data->buffer)[i] = 0;
      data->header.numFrames = waittime / data->frameSize;
    }
  }
  
  data->busy = 0;

  if (!data->flushing) {
    SndDoCommand(channel, &data->playCmd, true);
    SndDoCommand(channel, &data->callCmd, true);
  }
}

pascal void recordingCallback(SPBPtr params, Ptr buffer, short peakAmplitude, long numBytes)
{
  buffer_state *data = (buffer_state *)params->userLong;
  int bytesAvail;
  int bufferBytesUsed;
  int i;
  
  if (data->recqEnd >= data->recqStart)
    bufferBytesUsed = data->recqEnd - data->recqStart;
  else
    bufferBytesUsed = (data->recqEnd + (data->bufferSize - data->recqStart));
  
  bytesAvail = data->bufferSize - data->frameSize - bufferBytesUsed;
  
  if (numBytes > bytesAvail) {
    numBytes = bytesAvail;
    data->starved++;
  }
  
  for(i=0; i<numBytes; i++)
    data->recBuffer[(data->recqEnd+i) % data->bufferSize] = ((char *)buffer)[i];

  if (numBytes > 0)
    data->recqEnd = ((data->recqEnd + numBytes) % data->bufferSize);
}

int audio_open(snd_node *n, long *f)
{
  OSErr	err;
  Fixed sampleRateFixed;
  int i;
  buffer_state *data = (buffer_state *)malloc(sizeof(buffer_state));
  n->u.audio.descriptor = (void *)data;

  data->frameSize = snd_bytes_per_frame(n);

  data->bufferSize = (int) (n->format.srate * (double)data->frameSize);
  if (n->u.audio.latency > 0.0)
    data->bufferSize = (int)(n->format.srate * n->u.audio.latency) * data->frameSize;

  /* Calculate sample rate as an unsigned fixed-point number */
  if (n->format.srate > 65535.0 ||
    n->format.srate < 1.0)
    sampleRateFixed = 0xAC440000; /* Fixed for 44100 */
  else {
    unsigned short numerator = (unsigned short)n->format.srate;
    unsigned short denominator = (unsigned short)(65536.0*(n->format.srate - numerator));
    sampleRateFixed = (numerator << 16) | denominator;
  }

  /* Open device for recording or playback, depending on mode selected */

  if (n->write_flag == SND_READ) {
    /* recording */
    short gainControl = 0; /* off */
    short numChannels = n->format.channels;
    short continuousRecording = 1; /* on */
    short playthroughVolume = 0; /* off */
    OSType quality = 'cd  ';
    short sampleSize = 16;
    short twos = 0; /* i.e. signed */
    OSType compression = 'NONE';
    OSErr err;
  
    data->recording = 1;  
    
    err = SPBOpenDevice("\p", siWritePermission, &data->refnum);
    if (err)
      return !SND_SUCCESS;
    
    err = SPBSetDeviceInfo(data->refnum, 'qual', &quality);
    if (err)
      return !SND_SUCCESS;

    err = SPBSetDeviceInfo(data->refnum, 'agc ', &gainControl);
    if (err)
      return !SND_SUCCESS;
    
    err = SPBSetDeviceInfo(data->refnum, 'srat', &sampleRateFixed);
    if (err)
      return !SND_SUCCESS;

    err = SPBSetDeviceInfo(data->refnum, 'ssiz', &sampleSize);
    if (err)
      return !SND_SUCCESS;

    err = SPBSetDeviceInfo(data->refnum, 'chan', &numChannels);
    if (err)
      return !SND_SUCCESS;
  
    err = SPBSetDeviceInfo(data->refnum, 'cont', &continuousRecording);
    if (err)
      return !SND_SUCCESS;
  
    err = SPBSetDeviceInfo(data->refnum, 'plth', &playthroughVolume);
    if (err)
      return !SND_SUCCESS;
    
    err = SPBSetDeviceInfo(data->refnum, 'twos', &twos);
    if (err)
      return !SND_SUCCESS;

    err = SPBSetDeviceInfo(data->refnum, 'comp', &compression);
    if (err)
      return !SND_SUCCESS;
      
    data->recBuffer = (char *)malloc(data->bufferSize);
    data->recqStart = 0;
    data->recqEnd = 0;
    data->starved = 0;
    
    data->params.inRefNum = data->refnum;
    data->params.count = 0; /* data->bufferSize; /* bytes to record */
    data->params.milliseconds = 0; /* param will be ignored; use count */
    data->params.bufferLength = 0; /* ignore buffer */
    data->params.bufferPtr = NULL; /* ignore buffer */
    data->params.completionRoutine = NULL;
    data->params.userLong = (long)data;
    data->params.unused1 = 0;
    /* NOTE: This was NewSIInterruptUPP, but this isn't defined in my compiler -RBD */
    data->params.interruptRoutine = NewSIInterruptProc(recordingCallback);

    err = SPBRecord(&data->params, true);
    if (err)
      return !SND_SUCCESS;
    
  }
  else {
    /* playback */

    data->recording = 0;
    data->chan = NULL;
    err = SndNewChannel(&data->chan, sampledSynth, 0, NULL);
      
    if (err)
      return !SND_SUCCESS;
        
    data->buffer = (char *)malloc(data->bufferSize);
    
    data->nextBufferSize = data->bufferSize * 3;
    data->nextBuffer = (char *)malloc(data->nextBufferSize);   

    if (!data->buffer || !data->nextBuffer)
      return !SND_SUCCESS;
    
    data->chan->callBack = NewSndCallBackProc(playbackCallback);
    
    data->header.samplePtr = data->buffer;
    data->header.numChannels = n->format.channels;
    data->header.sampleRate = sampleRateFixed;
    data->header.loopStart = 0;
    data->header.loopEnd = 0;
    data->header.encode = cmpSH;
    data->header.baseFrequency = kMiddleC;
    // data->header.AIFFSampleRate = 0;  -- this parameter is unused
    data->header.markerChunk = NULL;
    data->header.format = kSoundNotCompressed;
    data->header.futureUse2 = NULL;
    data->header.stateVars = NULL;
    data->header.leftOverSamples = NULL;
    data->header.compressionID = 0;
    data->header.packetSize = 0;
    data->header.snthID = 0;
    data->header.sampleSize = 16;
    data->header.sampleArea[0] = 0;
    
    data->playCmd.cmd = bufferCmd;
    data->playCmd.param1 = 0; //unused
    data->playCmd.param2 = (long)&data->header;
    
    data->callCmd.cmd = callBackCmd;
    data->callCmd.param1 = 0;
    data->callCmd.param2 = (long)data;
    
    data->curBuffer = 0;
    data->curSize = 0;
    data->firstTime = 1;
    data->finished = 0;
    data->busy = 0;
    data->flushing = 0;
    data->empty = 1;
    
    // to avoid the glitch at the beginning of playback, write some zeros to prime the pump
    // use nextBuffer because it's convenient. We just need a buffer full of zeros.
    for (i = 0; i < data->bufferSize; i++) {
        data->nextBuffer[i] = 0;
    }
    audio_write(n, data->nextBuffer, data->bufferSize);
  }
  
  return SND_SUCCESS;
}


int audio_close(snd_node *n)
{
  buffer_state *data = (buffer_state *)n->u.audio.descriptor;
  OSErr err;
  
  if (data->recording) {
    SPBStopRecording(data->refnum);
    SPBCloseDevice(data->refnum);
    
    if (data->starved) {
      data->starved = 0;
    }
    
    #ifndef TARGET_CARBON
    DisposeRoutineDescriptor(data->params.interruptRoutine);
    #endif

    free((void *)data->recBuffer);
  }
  else {
      SndCallBackUPP callBack;
    data->finished = 1;
    
    callBack = data->chan->callBack;
    
    err = SndDisposeChannel(data->chan,
                            true         // quiets the channel now
                            );

    #ifndef TARGET_CARBON
    DisposeRoutineDescriptor(callBack);
    #endif
    
    free((void *)data->buffer);  
    free((void *)data->nextBuffer);
  }
  
  free((void *)data);

  return SND_SUCCESS;
}


int audio_flush(snd_type snd)
// return SND_SUCCESS when sound has been delivered to output
{
  buffer_state *data = (buffer_state *)snd->u.audio.descriptor;

  if (data->recording) {
    SPBStopRecording(data->refnum);
  }
  else {
    SCStatus status;
    OSErr err;

    //data->flushing = 1;
    
    /* Start playback if we haven't already */
/*
    if (data->firstTime) {
      data->header.numFrames = data->curSize / data->frameSize;
      
      SndDoCommand(data->chan, &data->playCmd, true);
      
      data->firstTime = 0;
    }

    do {
      err = SndChannelStatus(data->chan, sizeof(status), &status);
    } while (!err && status.scChannelBusy);
*/

    //data->flushing = 0;

  }

  // I don't know how to figure out when the last sample has been
  // written to the DAC. The following waits until 8 buffers of
  // zeros have been written. This seems to work well, giving a
  // fraction of a second margin of extra silence at the end.
  return (data->empty >= 8 ? SND_SUCCESS : !SND_SUCCESS);
}


long audio_read(snd_node *n, void *buffer, long length)
{
  buffer_state *data = (buffer_state *)n->u.audio.descriptor;

  if (data->recording) {

    int bufferBytesUsed;
    int i;
    
    if (data->recqEnd >= data->recqStart)
      bufferBytesUsed = data->recqEnd - data->recqStart;
    else
      bufferBytesUsed = (data->recqEnd + (data->bufferSize - data->recqStart));

    if (length > bufferBytesUsed)
      length = bufferBytesUsed;
    
    for(i=0; i<length; i++)
      ((char *)buffer)[i] = data->recBuffer[(data->recqStart+i) % data->bufferSize];
    
    data->recqStart = ((data->recqStart + length) % data->bufferSize);
    
    return length;
  }
  else {
    /* This shouldn't happen */
    return -1;
  }
}


long audio_write(snd_node *n, void *buffer, long length)
{
  long written = 0;
  long block;
  buffer_state *data = (buffer_state *)n->u.audio.descriptor;

  while(data->busy)
    ;
  
  data->busy = 1;

  
  if (data->curBuffer==0 && length>0) {
    data->empty = 0;
    block = min(length, data->bufferSize - data->curSize);
    
    if (block>0) {
    
      Ptr dest = (Ptr)&data->buffer[data->curSize];
      BlockMove((Ptr)buffer, dest, block);
      
      length -= block;
      written += block;
      data->curSize += block;
      buffer = &((char *)buffer)[block];
      
      if (data->curSize == data->bufferSize) {
        data->curSize = 0;
        data->curBuffer = 1;
      }
    }
  }
  
  // Copy into the second buffer (the one we don't pass to the Sound Manager directly)

  if (data->curBuffer == 1 && length>0) {
    data->empty = 0;
    block = min(length, data->nextBufferSize - data->curSize);
    
    if (block > 0) {
    
      Ptr dest = (Ptr)&data->nextBuffer[data->curSize];
      BlockMove((Ptr)buffer, dest, block);
      
      length -= block;
      written += block;
      data->curSize += block;
    }
  }

  // start playback immediately

  if (data->firstTime) {
    data->firstTime = 0;
    
    if (data->curBuffer==1) {
      data->header.numFrames = data->bufferSize / data->frameSize;
    }
    else {
      data->header.numFrames = data->curSize / data->frameSize;
      data->curBuffer = 1;
      data->curSize = 0;
    }
    
    data->busy = 0;

    SndDoCommand(data->chan, &data->playCmd, true);
    SndDoCommand(data->chan, &data->callCmd, true);    
  }
  
  data->busy = 0;
  
  return written;
}

int audio_reset(snd_node *n)
{
  /* audio reset not implemented */
  return !SND_SUCCESS;
}

long audio_poll(snd_type snd)
{
  buffer_state *data = (buffer_state *)snd->u.audio.descriptor;
  
  if (data->recording) {
  
    int bufferBytesUsed;

    if (data->recqEnd >= data->recqStart)
      bufferBytesUsed = data->recqEnd - data->recqStart;
    else
      bufferBytesUsed = (data->recqEnd + (data->bufferSize - data->recqStart));

    return (bufferBytesUsed / data->frameSize);
  
  }
  else {
    long avail = data->bufferSize - data->curSize;

    if (data->curBuffer == 0)
      avail += data->bufferSize;  

    /* Is this a bug in snd that I have to return frames here,
        and bytes everywhere else?
    */
    
    return avail / data->frameSize;
  }
}

snd_fns_node mac_dictionary = { audio_poll, audio_read, audio_write, 
                       audio_open, audio_close, audio_reset, audio_flush };


void snd_init()
{
  snd_add_device((char *)"Macintosh", (char *)"default", &mac_dictionary);
}

#ifdef __cplusplus
} // extern "C"
#endif

