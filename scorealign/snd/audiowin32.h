#include "windows.h"
#include "mmsystem.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
#define audio_read audio_process

#define audio_write audio_process
*/
#define Audio_out_min_buffersize 2 * 4096 //in samples
#define max_srate_dev 1.1 //maximum deviation from the expected nominal sample_rate. this is some
        // device dependent number, the device won't play back faster than max_srate_dev*srate

//there may be a need for minimums in separate units, like in samples (as above), in ms, ...

typedef struct {
    union {
        HWAVEIN h_in;
        HWAVEOUT h_out;		
    } u;
    LPWAVEHDR whdr;
    int numofbuffers;
    int pollee;	
    int posinbuffer;

    long sync_time;		
    int sync_buffer;
    long prev_time;	
} buffer_state;
        
/*	
#define waveGetDevCaps(a,b,c,d) ((a!=SND_READ) ? waveOutGetDevCaps((b),(c),(d)) : waveInGetDevCaps((b), (WAVEINCAPS *) (c),(d)))
#define waveGetNumDevs(a) ((a!=SND_READ) ? waveOutGetNumDevs() : waveInGetNumDevs())
#define waveOpen(a,b,c,d,e,f,g) ((a!=SND_READ) ? waveOutOpen((b),(c),(d),(e),(f),(g)) : waveInOpen((b),(c),(d),(e),(f),(g)))
#define waveReset(a,b) ((a!=SND_READ) ? waveOutReset((b)) : waveInReset((b)))
#define waveClose(a,b) ((a!=SND_READ) ? waveOutClose((b)) : waveInClose((b)))
#define waveUnprepareHeader(a,b,c,d) ((a!=SND_READ) ? waveOutUnprepareHeader((b),(c),(d)) : waveInUnprepareHeader((b),(c),(d)))
#define wavePrepareHeader(a,b,c,d) ((a!=SND_READ) ? waveOutPrepareHeader((b),(c),(d)) : waveInPrepareHeader((b),(c),(d)))
#define waveProcess(a,b,c,d) ((a!=SND_READ) ? waveOutWrite((b),(c),(d)) : waveInAddBuffer((b),(c),(d)))
#define waveGetErrorText(a,b,c,d) ((a!=SND_READ) ? waveInGetErrorText((b),(c),(d)) : waveOutGetErrorText((b),(c),(d)))
*/
#ifdef __cplusplus
}
#endif
