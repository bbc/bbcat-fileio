
#define BBCDEBUG_LEVEL 1
#include "ADMAudioFileSamples.h"

BBC_AUDIOTOOLBOX_START

ADMAudioFileSamples::ADMAudioFileSamples(const SoundFileSamples *isamples, const ADMAudioObject *obj) : SoundObjectFileSamples(isamples)
{
  uint_t i, n = GetChannels();

  // save initial limits of channels and time
  initialclip = GetClip();
  BBCDEBUG2(("Initial clip is %s for %s, tracks %u for %u", StringFrom(initialclip.start).c_str(), StringFrom(initialclip.nsamples).c_str(), initialclip.channel, initialclip.nchannels));

  for (i = 0; i < n; i++)
  {
    cursors.push_back(new ADMTrackCursor(GetStartChannel() + i));
  }

  if (obj) Add(obj);
}

ADMAudioFileSamples::ADMAudioFileSamples(const ADMAudioFileSamples *isamples) : SoundObjectFileSamples(isamples)
{
  uint_t i;

  // save initial limits of channels and time
  initialclip = GetClip();
  BBCDEBUG2(("Initial clip is %s for %s, tracks %u for %u", StringFrom(initialclip.start).c_str(), StringFrom(initialclip.nsamples).c_str(), initialclip.channel, initialclip.nchannels));

  for (i = 0; i < isamples->cursors.size(); i++)
  {
    const ADMTrackCursor *cursor;

    if ((cursor = dynamic_cast<const ADMTrackCursor *>(isamples->cursors[i])) != NULL)
    {
      cursors.push_back(new ADMTrackCursor(*cursor));
    }
  }
}

ADMAudioFileSamples::~ADMAudioFileSamples()
{
}

bool ADMAudioFileSamples::Add(const ADMAudioObject *obj)
{
  bool   added = false;
  uint_t i;

  for (i = 0; i < cursors.size(); i++)
  {
    ADMTrackCursor *cursor;

    if ((cursor = dynamic_cast<ADMTrackCursor *>(cursors[i])) != NULL)
    {
      if (cursor->Add(obj))
      {
        Clip_t   clip      = GetClip();
        uint64_t startTime = cursor->GetStartTime() / timebase;        // convert cursor start time from ns to samples
        uint64_t endTime   = cursor->GetEndTime()   / timebase;        // convert cursor end   time from ns to samples

        if (endTime == startTime)
        {
          startTime = obj->GetStartTime();
          endTime   = startTime + obj->GetDuration();
        }

        BBCDEBUG2(("Add object from %s to %s on track %u...", StringFrom(startTime).c_str(), StringFrom(endTime).c_str(), cursor->GetChannel() + 1));

        if (endTime > startTime)
        {
          if (objects.size() == 0)
          {
            // first object brings the audio time limits INWARDS to that of the object
            startTime = std::max(startTime, initialclip.start);
            endTime   = std::min(endTime,   initialclip.start + initialclip.nsamples);
          }
          else
          {
            // subsequent objects push the audio time limits OUTWARDS to that of the object (keeping within the original clip)
            startTime = std::min(startTime, std::max(clip.start, initialclip.start));
            endTime   = std::max(endTime,   std::min(clip.start + clip.nsamples, initialclip.start + initialclip.nsamples));
          }
        }

        clip.start    = startTime;
        clip.nsamples = endTime - startTime,

        BBCDEBUG2(("...clip now from %s to %s", StringFrom(startTime).c_str(), StringFrom(endTime).c_str()));

        SetClip(clip);

        objects.push_back(obj);

        added = true;
      }
    }
  }

  return added;
}

bool ADMAudioFileSamples::Add(const ADMAudioObject *objs[], uint_t n)
{
  bool   added = false;
  uint_t i;

  for (i = 0; i < n; i++)
  {
    added |= Add(objs[i]);
  }

  return added;
}

bool ADMAudioFileSamples::Add(const std::vector<const ADMAudioObject *>& objs)
{
  bool   added = false;
  uint_t i;

  for (i = 0; i < objs.size(); i++)
  {
    added |= Add(objs[i]);
  }

  return added;
}

BBC_AUDIOTOOLBOX_END
