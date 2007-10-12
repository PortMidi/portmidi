/* sndhead.h -- header info */

/* NeXT sound headers */
#define NEXT_SND_MAGIC (*((long *) ".snd"))  // was: ((int)0x2e736e64)
#define NEXT_SND_FORMAT_UNSPECIFIED		(0)
#define	NEXT_SND_FORMAT_ULAW_8			(1)
#define NEXT_SND_FORMAT_LINEAR_8		(2)
#define NEXT_SND_FORMAT_LINEAR_16		(3)
#define NEXT_SND_FORMAT_LINEAR_24		(4)
#define NEXT_SND_FORMAT_LINEAR_32		(5)
#define	NEXT_SND_FORMAT_FLOAT			(6)
#define	NEXT_SND_FORMAT_DOUBLE			(7)
#define NEXT_SND_FORMAT_INDIRECT		(8)
#define	NEXT_SND_FORMAT_NESTED			(9)
#define NEXT_SND_FORMAT_DSP_CORE		(10)
#define NEXT_SND_FORMAT_DSP_DATA_8		(11)
#define NEXT_SND_FORMAT_DSP_DATA_16		(12)
#define NEXT_SND_FORMAT_DSP_DATA_24		(13)
#define NEXT_SND_FORMAT_DSP_DATA_32		(14)
#define	NEXT_SND_FORMAT_DISPLAY			(16)
#define	NEXT_SND_FORMAT_MULAW_SQUELCH		(17)
#define	NEXT_SND_FORMAT_EMPHASIZED		(18)
#define	NEXT_SND_FORMAT_COMPRESSED		(19)
#define	NEXT_SND_FORMAT_COMPRESSED_EMPHASIZED	(20)
#define	NEXT_SND_FORMAT_DSP_COMMANDS		(21)
#define	NEXT_SND_FORMAT_DSP_COMMANDS_SAMPLES	(22)

/* for soundheaders (IRCAM) */
#define IRCAM_SND_MAGIC 107364L
#define SIZEOF_IRCAM_HEADER 1024
#define IRCAM_SND_CHAR   0x101
#define IRCAM_SND_ALAW   0x102
#define IRCAM_SND_ULAW   0x103
#define IRCAM_SND_SHORT  0x104
#define IRCAM_SND_LONG   0x105
#define IRCAM_SND_FLOAT  0x106
#define	IRCAM_SND_END		0
#define	IRCAM_SND_MAXAMP	1
#define	IRCAM_SND_AUDIOENCODE	2
#define	IRCAM_SND_PVDATA	3
#define	IRCAM_SND_COMMENT	4
#define	IRCAM_SND_CODMAX	4

/* (Microsoft WAV sound headers */
/* purloined from public Microsoft RIFF docs */

#define	WAVE_FORMAT_UNKNOWN		(0x0000)
#ifndef WIN32
// already defined (compatibly) by Microsoft header file:
#define	WAVE_FORMAT_PCM			(0x0001)
#endif
#define	WAVE_FORMAT_ADPCM		(0x0002)
#define	WAVE_FORMAT_ALAW		(0x0006)
#define	WAVE_FORMAT_MULAW		(0x0007)
#define	WAVE_FORMAT_OKI_ADPCM		(0x0010)
#define	WAVE_FORMAT_DIGISTD		(0x0015)
#define	WAVE_FORMAT_DIGIFIX		(0x0016)
#define	IBM_FORMAT_MULAW         	(0x0101)
#define	IBM_FORMAT_ALAW			(0x0102)
#define	IBM_FORMAT_ADPCM         	(0x0103)
