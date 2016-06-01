
#define BBCDEBUG_LEVEL 1
#include "ADMRIFFFile.h"
#include "ADMAudioFileSamples.h"

BBC_AUDIOTOOLBOX_START

ADMAudioFileSamples::ADMAudioFileSamples(const ADMRIFFFile& file) : SoundFileSamples(file.GetSamples()),
                                                                    adm(file.GetADM())
{
  uint_t i, n = GetChannels();
  
  // save initial limits of channels and time
  initialclip = GetClip();
  BBCDEBUG2(("Initial clip is %s for %s, tracks %u for %u", StringFrom(initialclip.start).c_str(), StringFrom(initialclip.nsamples).c_str(), initialclip.channel, initialclip.nchannels));

  for (i = 0; i < n; i++)
  {
    cursors.push_back(new ADMTrackCursor(GetStartChannel() + i));
  }
}

ADMAudioFileSamples::ADMAudioFileSamples(const SoundFileSamples *isamples, const ADMData *_adm) : SoundFileSamples(isamples),
                                                                                                  adm(_adm)
{
  uint_t i, n = GetChannels();
  
  // save initial limits of channels and time
  initialclip = GetClip();
  BBCDEBUG2(("Initial clip is %s for %s, tracks %u for %u", StringFrom(initialclip.start).c_str(), StringFrom(initialclip.nsamples).c_str(), initialclip.channel, initialclip.nchannels));

  for (i = 0; i < n; i++)
  {
    cursors.push_back(new ADMTrackCursor(GetStartChannel() + i));
  }
}

ADMAudioFileSamples::ADMAudioFileSamples(const ADMAudioFileSamples *isamples) : SoundFileSamples(isamples),
                                                                                adm(isamples->adm)
{
  uint_t i;

  // save initial limits of channels and time
  initialclip = GetClip();
  BBCDEBUG2(("Initial clip is %s for %s, tracks %u for %u", StringFrom(initialclip.start).c_str(), StringFrom(initialclip.nsamples).c_str(), initialclip.channel, initialclip.nchannels));

  for (i = 0; i < isamples->cursors.size(); i++)
  {
    cursors.push_back(new ADMTrackCursor(*isamples->cursors[i]));
  }
}

ADMAudioFileSamples::~ADMAudioFileSamples()
{
  uint_t i;
  for (i = 0; i < cursors.size(); i++)
  {
    delete cursors[i];
  }
}

bool ADMAudioFileSamples::Add(const ADMAudioObject *obj)
{
  bool   added = false;
  uint_t i;

  if (!adm) adm = &obj->GetOwner();
  else if (adm != &obj->GetOwner())
  {
    // MUST not allow objects from different ADM to be added (their audio will be in a different file!)
    BBCERROR("Attempting to add %s from *different* ADM (%s vs %s) to ADMAudioFileSamples<%s>", obj->ToString().c_str(), StringFrom(&obj->GetOwner()).c_str(), StringFrom(adm).c_str(), StringFrom(this).c_str());
    return false;
  }

  ThreadLock lock(*GetADM());
  for (i = 0; i < cursors.size(); i++)
  {
    ADMTrackCursor *cursor = cursors[i];

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

/*--------------------------------------------------------------------------------*/
/** Get list of audio objects active at *current* time
 */
/*--------------------------------------------------------------------------------*/
void ADMAudioFileSamples::GetObjectList(ADMAudioObject::LIST& list)
{
  // MUST make sure ADM is valid before using its lock
  // ADM should never be invalidated during the lifetime of this object
  if (adm)
  {
    ThreadLock lock(*adm);
    ADMAudioObject *lastobj = NULL;
    uint_t i;

    for (i = 0; i < cursors.size(); i++)
    {
      ADMAudioObject *obj = cursors[i]->GetAudioObject();

      if (obj && (obj != lastobj)) list.push_back(obj);
    
      lastobj = obj;
    }  
  }
}

/*--------------------------------------------------------------------------------*/
/** Seek to specified time on all cursors
 */
/*--------------------------------------------------------------------------------*/
void ADMAudioFileSamples::SeekAllCursors(uint64_t t)
{
  // MUST make sure ADM is valid before using its lock
  // ADM should never be invalidated during the lifetime of this object
  if (adm)
  {
    ThreadLock lock(*adm);
    uint_t i;
  
    for (i = 0; i < cursors.size(); i++)
    {
      cursors[i]->Seek(t);
    }
  }
}

BBC_AUDIOTOOLBOX_END
