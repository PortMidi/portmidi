#include <Sound.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int                      recording;

  int                      bufferSize;
  int                      frameSize;

  /* Recording */
  long                     refnum;
  SPB                      params;
  char                     *recBuffer;
  int                      recqStart;
  int                      recqEnd;
  int                      starved;
  
  /* Playback */
  SndChannelPtr            chan;
  CmpSoundHeader           header;
  SndCommand               playCmd;
  SndCommand               callCmd;
  char                     *buffer;
  char                     *nextBuffer;
  int                      nextBufferSize;
  int                      curBuffer;
  int                      curSize;
  int                      firstTime;
  int                      finished;
  int                      flushing;
  int                      busy;
  int					   empty;

} buffer_state;
    
#ifdef __cplusplus
}
#endif
