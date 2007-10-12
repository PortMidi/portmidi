#include "memory.h"
#include "stdio.h" 
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

