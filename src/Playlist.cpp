
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BBCDEBUG_LEVEL 1
#include "Playlist.h"

#include "SoundFileAttributes.h"

BBC_AUDIOTOOLBOX_START

Playlist::Playlist() : filestartpos(0),
                       playlistlength(0),
                       fadesamples(100),
                       fadedowncount(0),
                       fadeupcount(0),
                       autoplay(true),
                       loop_all(false),
                       loop_file(false),
                       pause(false),
                       releaseplayback(false),
                       positionchange(false)
{
  it = list.begin();
}

Playlist::~Playlist()
{
  uint_t i;

  for (i = 0; i < list.size(); i++)
  {
    delete list[i];
  }
}

/*--------------------------------------------------------------------------------*/
/** Add file to list
 *
 * @note object will be DELETED on destruction of this object!
 */
/*--------------------------------------------------------------------------------*/
void Playlist::AddFile(SoundFileSamples *file)
{
  list.push_back(file);

  playlistlength += file->GetSampleLength();

  // MUST reset here to ensure 'it' is always valid
  Reset();
}

/*--------------------------------------------------------------------------------*/
/** Clear playlist
 */
/*--------------------------------------------------------------------------------*/
void Playlist::Clear()
{
  while (list.size())
  {
    delete list.back();
    list.pop_back();
  }

  playlistlength = 0;
    
  // MUST reset here to ensure 'it' is always valid
  Reset();
}

/*--------------------------------------------------------------------------------*/
/** Reset to start of playback list
 */
/*--------------------------------------------------------------------------------*/
void Playlist::Reset()
{
  filestartpos    = 0;           // reset to start of playlist
  fadedowncount   = 0;           // stop fade down
  fadeupcount     = fadesamples; // start fade up
  positionchange  = false;       // cancel position change
  pause           = false;       // unpause audio
  releaseplayback = false;       // clear playback release
  
  it = list.begin();
  if (it != list.end()) (*it)->SetSamplePosition(0);
}

/*--------------------------------------------------------------------------------*/
/** Move onto next file (or stop if looping is disabled)
 */
/*--------------------------------------------------------------------------------*/
void Playlist::Next()
{
  if (it != list.end())
  {
    // if looping file, don't need to move playlist, just reset back to start of file
    if (!loop_file)
    {
      // move position on by current file's length
      filestartpos += (*it)->GetSampleLength();

      // advance along list and loop back to start if loop_all enabled 
      if (((++it) == list.end()) && loop_all)
      {
        it = list.begin();
        filestartpos = 0;
      }
    }
    
    // reset position of file if not at end of list
    if (it != list.end()) (*it)->SetSamplePosition(0);
  }
}

/*--------------------------------------------------------------------------------*/
/** Return current file or NULL if the end of the list has been reached
 */
/*--------------------------------------------------------------------------------*/
SoundFileSamples *Playlist::GetFile()
{
  return (it != list.end()) ? *it : NULL;
}

/*--------------------------------------------------------------------------------*/
/** Return max number of audio channels of playlist
 */
/*--------------------------------------------------------------------------------*/
uint_t Playlist::GetMaxOutputChannels() const
{
  uint_t i, channels = 0;

  for (i = 0; i < list.size(); i++)
  {
    uint_t file_channels = list[i]->GetChannels();

    channels = std::max(channels, file_channels);
  }

  return channels;
}

/*--------------------------------------------------------------------------------*/
/** Return current playback position (in samples)
 */
/*--------------------------------------------------------------------------------*/
uint64_t Playlist::GetPlaybackPosition() const
{
  uint64_t pos = filestartpos;
  if (it != list.end()) pos += (*it)->GetSamplePosition();
  return pos;
}

/*--------------------------------------------------------------------------------*/
/** Set current playback position (in samples)
 *
 * @note setting force to true may cause clicks!
 * @note setting force to false causes a fade down *before* and a fade up *after*
 * changing the position which means this doesn't actually change the position straight away!
 */
/*--------------------------------------------------------------------------------*/
bool Playlist::SetPlaybackPosition(uint64_t pos, bool force)
{
  ThreadLock lock(tlock);
  bool success = false;

  pos = std::min(pos, playlistlength);

  if (!Empty())
  {
    if (force || pause)
    {
      BBCDEBUG2(("Warning: forcing move to %s", StringFrom(pos).c_str()));
      
      // clear fade down
      fadedowncount = 0;
      // clear any position change request
      positionchange = false;

      // set position immediately and to hell with fading down
      if (SetPlaybackPositionEx(pos))
      {
        // start fade up
        fadeupcount = fadesamples;
        success     = true;
      }
    }
    else
    {
      // fade down first
      BBCDEBUG3(("Set new position request for position %s samples", StringFrom(pos).c_str()));

      // start fade down
      fadedowncount  = fadesamples;

      // set up for position change
      newposition    = pos;
      positionchange = true;
      success        = true;
    }
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Set current playback position (in samples)
 */
/*--------------------------------------------------------------------------------*/
bool Playlist::SetPlaybackPositionEx(uint64_t pos)
{
  bool success = false;

  // if looping is enabled, do not allow movement beyond last sample of playlist
  uint64_t len = loop_all ? limited::subz(playlistlength, (uint64_t)1) : playlistlength;

  // limit position
  pos = std::min(pos, len); 
            
  // move back if necessary
  while ((it != list.begin()) && (pos < filestartpos))
  {
    // move back
    --it;
    // move position back by new file's length
    filestartpos -= (*it)->GetSampleLength();
  }
  // move forward if necessary
  while ((it != list.end()) && (pos >= (filestartpos + (*it)->GetSampleLength())))
  {
    // move position back by existing file's length
    filestartpos += (*it)->GetSampleLength();
    // move forward
    ++it;
  }

  if ((it != list.end()) && (pos >= filestartpos) && (pos < (filestartpos + (*it)->GetSampleLength())))
  {
    (*it)->SetSamplePosition(pos - filestartpos);
    success = true;
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Return current index in playlist
 */
/*--------------------------------------------------------------------------------*/
uint_t Playlist::GetPlaybackIndex() const
{
  return (uint_t)(it - list.begin());
}

/*--------------------------------------------------------------------------------*/
/** Return maximum index in playlist
 *
 * @note this is ONE LESS than GetCount() 
 */
/*--------------------------------------------------------------------------------*/
uint_t Playlist::GetPlaybackCount() const
{
  return limited::subz((uint_t)list.size(), 1U);
}
  
/*--------------------------------------------------------------------------------*/
/** Move to specified playlist index
 *
 * @note setting force to true may cause clicks!
 * @note setting force to false causes a fade down *before* and a fade up *after*
 * changing the position which means this doesn't actually change the position straight away!
 */
/*--------------------------------------------------------------------------------*/
bool Playlist::SetPlaybackIndex(uint_t index, bool force)
{
  // NO NEED to lock thread here
  bool success = false;

  if (!Empty())
  {
    uint64_t pos = 0;
    uint_t   i;

    index = std::min(index, GetPlaybackCount());

    // find position of start of specified file
    for (i = 0; i < index; i++)
    {
      pos += list[i]->GetSampleLength();
    }

    BBCDEBUG2(("SetPlaybackIndex %u pos %s", index, StringFrom(pos).c_str()));
  
    // now just request change of position
    success = SetPlaybackPosition(pos, force);

    releaseplayback = true;
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Pause/unpause audio
 */
/*--------------------------------------------------------------------------------*/
void Playlist::PauseAudio(bool enable)
{
  if (enable != pause)
  {
    ThreadLock lock(tlock);

    // either fade up or down
    if (pause) fadeupcount   = fadesamples;
    else       fadedowncount = fadesamples;

    pause = enable;
  }
}

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
uint_t Playlist::ReadSamples(Sample_t *dst, uint_t channel, uint_t channels, uint_t frames)
{
  ThreadLock lock(tlock);
  uint_t nframes = 0;

  while (!AtEnd() && frames)
  {
    SoundFileSamples *file = GetFile();
    uint_t nread = 0;
    bool   playallowed = (autoplay || file->GetSamplePosition() || positionchange);

    if (!playallowed && !releaseplayback)
    {
      // file playback cannot be started yet -> fill remainder of buffer with zeros
      nread = frames;
      memset(dst, 0, nread * channels * sizeof(*dst));
    }
    else
    {
      // clear trigger play flag if it was used to release playback
      releaseplayback &= (playallowed || pause);
      
      if (fadedowncount)
      {
        uint_t i, j;

        BBCDEBUG3(("Fading down: %u frames left, coeff %0.3f", fadedowncount, (Sample_t)(fadedowncount - 1) / (Sample_t)fadesamples));

        // fading down, limit to fadedowncount and fade after reading
        nread = file->ReadSamples(dst, channel, channels, std::min(frames, fadedowncount));
        BBCDEBUG3(("Read %u/%u/%u frames from file (fadedown)", nread, frames, fadedowncount));

        // fade read audio
        for (i = 0; i < nread; i++, fadedowncount--)
        {
          Sample_t mul = (Sample_t)(fadedowncount - 1) / (Sample_t)fadesamples;
          for (j = 0; j < channels; j++) dst[i * channels + j] *= mul;
        }
      }
      else if (pause)
      {
        // audio is paused, fill remainder of buffer with zeros
        nread = frames;
        memset(dst, 0, nread * channels * sizeof(*dst));
      }
      else if (positionchange)
      {
        BBCDEBUG3(("Changing position to %s samples", StringFrom(newposition).c_str()));

        // actually set the new position now that audio is faded down
        SetPlaybackPositionEx(newposition);

        // clear change request
        positionchange = false;

        // start fadeup
        fadeupcount = fadesamples;

        // force loop around
        continue;
      }
      else if (fadeupcount)
      {
        uint_t i, j;

        BBCDEBUG3(("Fading up: %u frames left, coeff %0.3f", fadeupcount, (Sample_t)(fadesamples - fadeupcount) / (Sample_t)fadesamples));

        // fading up, limit to fadeupcount and fade after reading
        nread = file->ReadSamples(dst, channel, channels, std::min(frames, fadeupcount));
        BBCDEBUG3(("Read %u/%u/%u frames from file (fadeup)", nread, frames, fadeupcount));

        // fade read audio
        for (i = 0; i < nread; i++, fadeupcount--)
        {
          Sample_t mul = (Sample_t)(fadesamples - fadeupcount) / (Sample_t)fadesamples;
          for (j = 0; j < channels; j++) dst[i * channels + j] *= mul;
        }
      }
      else
      {
        BBCDEBUG3(("Reading %u frames from file", frames));
        // no position changes pending, simple read
        nread = file->ReadSamples(dst, channel, channels, frames);
        BBCDEBUG3(("Read %u/%u frames from file", nread, frames));
      }
    }
    
    // if reached the end of the file, move onto next one
    if (!nread) Next();
      
    dst     += nread * channels;
    frames  -= nread;
    nframes += nread;
  }

  BBCDEBUG3(("Total %u frames returned", nframes));
  
  return nframes;
}

BBC_AUDIOTOOLBOX_END
