#include "snd.h"

long audio_poll(snd_type snd)
{
    return 0;
}


long audio_read(snd_type snd, void *buffer, long length)
{
    return 0;
}


long audio_write(snd_type snd, void *buffer, long length)
{
  return 0;
}


int audio_open(snd_type snd, long *flags)
{
  return 0;
}


int audio_close(snd_type snd)
{
  return 0;
}


/* audio_flush -- finish audio output */
int audio_flush(snd_type snd)
{
  return 0;
}


int audio_reset(snd_type snd)
{
  return 0;
}

snd_fns_node mmsystem_dictionary = { audio_poll, audio_read, audio_write, 
                       audio_open, audio_close, audio_reset, audio_flush };


void snd_init()
{
    snd_add_device("None", "default", &mmsystem_dictionary);
}
