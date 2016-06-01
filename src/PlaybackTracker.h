#ifndef __PLAYBACK_TRACKER__
#define __PLAYBACK_TRACKER__

#include <bbcat-base/misc.h>

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** An interface for gaining access to current file, file position and absolute position
 */
/*--------------------------------------------------------------------------------*/
class SoundFileSamples;
class PlaybackTracker
{
public:
  typedef struct
  {
    SoundFileSamples *file;             // current file being played
    uint64_t         absoluteposition;  // absolute position in playlist in samples
    uint64_t         fileposition;      // position within current file in samples
    uint_t           fileindex;         // index of file in playlist
  } PLAYBACKPROGRESS;
  
  /*--------------------------------------------------------------------------------*/
  /** Return playback progress object
   */
  /*--------------------------------------------------------------------------------*/
  virtual PLAYBACKPROGRESS GetPlaybackProgress() const = 0;
};

BBC_AUDIOTOOLBOX_END

#endif
