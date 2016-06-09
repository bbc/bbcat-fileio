#ifndef __SIMPLE_PLAYLIST__
#define __SIMPLE_PLAYLIST__

#include <vector>

#include <bbcat-base/ThreadLock.h>

#include "PlaybackTracker.h"

BBC_AUDIOTOOLBOX_START

class Playlist : public PlaybackTracker
{
public:
  Playlist();
  virtual ~Playlist();

  /*--------------------------------------------------------------------------------*/
  /** Return whether the playlist is empty
   */
  /*--------------------------------------------------------------------------------*/
  bool Empty() const {return (list.size() == 0);}

  /*--------------------------------------------------------------------------------*/
  /** Add file to list
   *
   * @note object will be DELETED on destruction of this object!
   */
  /*--------------------------------------------------------------------------------*/
  void AddFile(SoundFileSamples *file);

  /*--------------------------------------------------------------------------------*/
  /** Update playlist length (e.g. after modifying a file)
   */
  /*--------------------------------------------------------------------------------*/
  void UpdatePlaylistLength();

  /*--------------------------------------------------------------------------------*/
  /** Clear playlist
   */
  /*--------------------------------------------------------------------------------*/
  void Clear();

  /*--------------------------------------------------------------------------------*/
  /** Enable/disable looping
   */
  /*--------------------------------------------------------------------------------*/
  void EnableLoop(bool enable = true) {loop_all = enable;}

  /*--------------------------------------------------------------------------------*/
  /** Get whether looping is enabled
   */
  /*--------------------------------------------------------------------------------*/
  bool IsLoopEnabled() const {return loop_all;}

  /*--------------------------------------------------------------------------------*/
  /** Enable/disable looping of each file
   */
  /*--------------------------------------------------------------------------------*/
  void EnableLoopFile(bool enable = true) {loop_file = enable;}

  /*--------------------------------------------------------------------------------*/
  /** Get whether looping of each file is enabled
   */
  /*--------------------------------------------------------------------------------*/
  bool IsLoopFileEnabled() const {return loop_file;}

  /*--------------------------------------------------------------------------------*/
  /** Enable/disable autoplay of each file
   */
  /*--------------------------------------------------------------------------------*/
  void EnableAutoPlay(bool enable) {autoplay = enable;}

  /*--------------------------------------------------------------------------------*/
  /** Get whether autoplay of each file is enabled
   */
  /*--------------------------------------------------------------------------------*/
  bool IsAutoPlayEnabled() const {return autoplay;}

  /*--------------------------------------------------------------------------------*/
  /** Pause/unpause audio
   */
  /*--------------------------------------------------------------------------------*/
  void PauseAudio(bool enable = true);

  /*--------------------------------------------------------------------------------*/
  /** Get whether autoplay of each file is enabled
   */
  /*--------------------------------------------------------------------------------*/
  bool IsAudioPaused() const {return pause;}

  /*--------------------------------------------------------------------------------*/
  /** Release playback of audio that is current held
   */
  /*--------------------------------------------------------------------------------*/
  void ReleasePlayback() {releaseplayback = true;}

  /*--------------------------------------------------------------------------------*/
  /** Reset to start of playback list
   */
  /*--------------------------------------------------------------------------------*/
  void Reset();

  /*--------------------------------------------------------------------------------*/
  /** Move onto next file (or stop if looping is disabled)
   */
  /*--------------------------------------------------------------------------------*/
  void Next();

  /*--------------------------------------------------------------------------------*/
  /** Return whether the end of the playlist has been reached
   */
  /*--------------------------------------------------------------------------------*/
  bool AtEnd() const {return (it == list.end());}

  /*--------------------------------------------------------------------------------*/
  /** Return current file or NULL if the end of the list has been reached
   */
  /*--------------------------------------------------------------------------------*/
  SoundFileSamples *GetFile() const;

  /*--------------------------------------------------------------------------------*/
  /** Return file at specific index or NULL if index is out of range
   */
  /*--------------------------------------------------------------------------------*/
  SoundFileSamples *GetFileAtIndex(uint_t index) const;
  
  /*--------------------------------------------------------------------------------*/
  /** Return last file in list (or NULL)
   */
  /*--------------------------------------------------------------------------------*/
  SoundFileSamples *GetLastFile() const;

  /*--------------------------------------------------------------------------------*/
  /** Return index of specified file (or -1)
   */
  /*--------------------------------------------------------------------------------*/
  int GetIndexOf(SoundFileSamples *file) const;

  /*--------------------------------------------------------------------------------*/
  /** Return max number of audio channels of playlist
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetMaxOutputChannels() const;

  /*--------------------------------------------------------------------------------*/
  /** Return number of objects in playlist
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetCount() const {return (uint_t)list.size();}

  /*--------------------------------------------------------------------------------*/
  /** Return current playback position (in samples)
   */
  /*--------------------------------------------------------------------------------*/
  uint64_t GetPlaybackPosition() const;

  /*--------------------------------------------------------------------------------*/
  /** Return length of playlist in samples
   */
  /*--------------------------------------------------------------------------------*/
  uint64_t GetPlaybackLength() const {return playlistlength;}

  /*--------------------------------------------------------------------------------*/
  /** Set current playback position (in samples)
   *
   * @note setting force to true may cause clicks!
   * @note setting force to false causes a fade down *before* and a fade up *after*
   * changing the position which means this doesn't actually change the position straight away!
   */
  /*--------------------------------------------------------------------------------*/
  bool SetPlaybackPosition(uint64_t pos, bool force);

  /*--------------------------------------------------------------------------------*/
  /** Return current index in playlist
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetPlaybackIndex() const;

  /*--------------------------------------------------------------------------------*/
  /** Return maximum index in playlist
   *
   * @note this is ONE LESS than GetCount() 
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetPlaybackCount() const;
  
  /*--------------------------------------------------------------------------------*/
  /** Move to specified playlist index
   *
   * @param index new playback index
   * @param fromstart true to play new item from start, false to start playing new item from the *same* place as currently being played
   * @param force true to force position change immediately (see below)
   *
   * @note setting force to true may cause clicks!
   * @note setting force to false causes a fade down *before* and a fade up *after*
   * changing the position which means this doesn't actually change the position straight away!
   */
  /*--------------------------------------------------------------------------------*/
  bool SetPlaybackIndex(uint_t index, bool fromstart, bool force);

  /*--------------------------------------------------------------------------------*/
  /** Return number of samples used during fade up or down
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetFadeSamples() const {return fadesamples;}

  /*--------------------------------------------------------------------------------*/
  /** Set number of samples used during fade up or down
   */
  /*--------------------------------------------------------------------------------*/
  void SetFadeSamples(uint_t n) {fadesamples = n;}
  
  /*--------------------------------------------------------------------------------*/
  /** Return playback progress object
   */
  /*--------------------------------------------------------------------------------*/
  virtual PLAYBACKPROGRESS GetPlaybackProgress() const;

  /*--------------------------------------------------------------------------------*/
  /** Read samples into buffer
   *
   * @param dst destination sample buffer
   * @param channel offset channel to read from
   * @param channels number of channels to read
   * @param frames maximum number of frames to read
   *
   * @return actual number of frames read
   */
  /*--------------------------------------------------------------------------------*/
  uint_t ReadSamples(Sample_t *dst, uint_t channel, uint_t channels, uint_t frames);

protected:
  /*--------------------------------------------------------------------------------*/
  /** Set current playback position (in samples)
   */
  /*--------------------------------------------------------------------------------*/
  bool SetPlaybackPositionEx(uint64_t pos);

protected:
  ThreadLockObject                tlock;
  std::vector<SoundFileSamples *> list;
  std::vector<SoundFileSamples *>::iterator it;
  uint64_t                        filestartpos, playlistlength;
  uint64_t                        newposition;
  uint_t                          fadesamples;
  uint_t                          fadedowncount;
  uint_t                          fadeupcount;
  bool                            autoplay;
  bool                            loop_all;
  bool                            loop_file;
  bool                            pause;
  bool                            releaseplayback;
  bool                            positionchange;
};

BBC_AUDIOTOOLBOX_END

#endif
