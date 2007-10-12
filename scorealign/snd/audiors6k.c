/* this code is obsolete, and should be converted to be compatible
   with the current snd library. Maybe we should rename it to 
   audionone.c (I never did get direct audio output from RS6K)
   This code should be used for any system that does not have audio
   I/O devices -RBD
 */

#include "snd.h"

int audio_open()
{
    printf("audio_open not implemented\n");
    return SND_SUCCESS;
}


int audio_close()
{
    printf("audio_close not implemented\n");
    return SND_SUCCESS;
}


int audio_flush(snd_type snd)
{
    printf("audio_flush not implemented\n");
    return SND_SUCCESS;
}

long audio_read()
{
    printf("audio_read not implemented\n");
    return 0;
}


long audio_write()
{
    printf("audio_write not implemented\n");
    return 0;
}

int audio_reset()
{
    printf("audio reset not implemented\n");
    return SND_SUCCESS;
}


long audio_poll(snd_type snd)
{
    return 1000000;
}

