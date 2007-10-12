/* Standard includes */

#include "stdlib.h"
#include "stdio.h"
#include <sys/asoundlib.h>
#include <linux/asoundid.h>

/* snd includes */

#include "snd.h"

typedef struct {
    int portfd;
    snd_pcm_t *sound_handle;
} descr_t, *descr_type;


const char* snd_card_type_name (unsigned int card_type)
{
  switch (card_type)
    {
      /* Gravis UltraSound */
    case SND_CARD_TYPE_GUS_CLASSIC:
        return "GUS Classic";
    case SND_CARD_TYPE_GUS_EXTREME:
        return "GUS Extreme";
    case SND_CARD_TYPE_GUS_ACE:
        return "GUS ACE";
    case SND_CARD_TYPE_GUS_MAX:
        return "GUS MAX";
    case SND_CARD_TYPE_AMD_INTERWAVE:
        return "AMD Interwave";
      /* Sound Blaster */
    case SND_CARD_TYPE_SB_10:
        return "Sound Blaster 10";
    case SND_CARD_TYPE_SB_20:
        return "Sound Blaster 20";
    case SND_CARD_TYPE_SB_PRO:
        return "Sound Blaster Pro";
    case SND_CARD_TYPE_SB_16:
        return "Sound Blaster 16";
    case SND_CARD_TYPE_SB_AWE:
        return "Sound Blaster AWE";
      /* Various */
    case SND_CARD_TYPE_ESS_ES1688:
        return "ESS AudioDrive ESx688";
    case SND_CARD_TYPE_OPL3_SA:
        return "Yamaha OPL3 SA";
    case SND_CARD_TYPE_MOZART:
        return "OAK Mozart";
    case SND_CARD_TYPE_S3_SONICVIBES:
        return "S3 SonicVibes";
    case SND_CARD_TYPE_ENS1370:
        return "Ensoniq ES1370";
    case SND_CARD_TYPE_ENS1371:
        return "Ensoniq ES1371";
    case SND_CARD_TYPE_CS4232:
        return "CS4232/CS4232A";
    case SND_CARD_TYPE_CS4236:
        return "CS4235/CS4236B/CS4237B/CS4238B/CS4239";
    case SND_CARD_TYPE_AMD_INTERWAVE_STB:
        return "AMD InterWave + TEA6330T";
    case SND_CARD_TYPE_ESS_ES1938:
        return "ESS Solo-1 ES1938";
    case SND_CARD_TYPE_ESS_ES18XX:
        return "ESS AudioDrive ES18XX";
    case SND_CARD_TYPE_CS4231:
        return "CS4231";
    case SND_CARD_TYPE_SERIAL:
        return "Serial MIDI driver";
    case SND_CARD_TYPE_AD1848:
        return "Generic AD1848 driver";
    case SND_CARD_TYPE_TRID4DWAVEDX:
        return "Trident 4DWave DX";
    case SND_CARD_TYPE_TRID4DWAVENX:
        return "Trident 4DWave NX";
    case SND_CARD_TYPE_SGALAXY:
        return "Aztech Sound Galaxy";
    case SND_CARD_TYPE_CS461X:
        return "Sound Fusion CS4610/12/15";
      /* Turtle Beach WaveFront series */
    case SND_CARD_TYPE_WAVEFRONT:
        return "TB WaveFront generic";
    case SND_CARD_TYPE_TROPEZ:
        return "TB Tropez";
    case SND_CARD_TYPE_TROPEZPLUS:
        return "TB Tropez+";
    case SND_CARD_TYPE_MAUI:
        return "TB Maui";
    case SND_CARD_TYPE_CMI8330:
        return "C-Media CMI8330";
    case SND_CARD_TYPE_DUMMY:
        return "Dummy Soundcard";
      /* --- */
    case SND_CARD_TYPE_ALS100:
        return "Avance Logic ALS100";
      /* --- */
    default:
      if (card_type < SND_CARD_TYPE_LAST)
        return "Unknown";
      return "Invalid";
    }
}


descr_type cast_descrp(snd_type snd)
{
    return (descr_type) snd->u.audio.descriptor;
}


int audio_reset(snd_type snd);


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
    /* Not implemented by PL -RBD */
    /* I have no idea how Aura works without audio_poll -RBD */
    exit(0);
    return 0;
}


long audio_read(snd_type snd, void *buffer, long length)
{
    /* Not implemented by PL -RBD */
    return 0;
}


long audio_write(snd_type snd, void *buffer, long length)
{
  descr_type dp = cast_descrp(snd);
  snd_pcm_write(dp->sound_handle, buffer, (int) length);
  return length;
}


int audio_open(snd_type snd, long *flags)
{
    snd_pcm_info_t pcm_info = { 0, };
    int perrno, fd;
    snd_pcm_t *sound_handle;

    perrno = snd_pcm_open(&sound_handle, 0, 0, SND_PCM_OPEN_PLAYBACK);

    if (perrno >= 0) {
        fd = snd_pcm_file_descriptor(sound_handle);
        if (fd < 0) {
            snd_pcm_close(sound_handle);
            perrno = fd;
        }
    } else {
        /* This printf should not be here -RBD */
        printf("Sound device error: %d - %s \n", perrno, snd_strerror(perrno));
        return !SND_SUCCESS;
    };

    if (snd_pcm_info (sound_handle, &pcm_info) < 0)
        return !SND_SUCCESS;

    /* These printfs should not be here -RBD */
    printf("Card name: %s", snd_card_type_name (pcm_info.type));
    printf("Card ID: %d\n", pcm_info.id);

    return SND_SUCCESS;
}


int audio_close(snd_type snd)
{
    snd_pcm_close(cast_descrp(snd)->sound_handle);
    return SND_SUCCESS;
}


/* audio_flush -- finish audio output */
/* 
 * if any buffer is non-empty, send it out
 * return success when all buffers have been returned empty
 */
int audio_flush(snd_type snd)
{
    exit(0);
    return SND_SUCCESS;
}


int audio_reset(snd_type snd)
{
    return SND_SUCCESS;
}


snd_fns_node mmsystem_dictionary = { audio_poll, audio_read, audio_write, 
                       audio_open, audio_close, audio_reset, audio_flush };


void snd_init()
{
    snd_add_device("ALSA", "default", &mmsystem_dictionary);
}
