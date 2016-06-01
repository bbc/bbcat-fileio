#ifndef __ADM_AUDIO_OBJECT_FILE__
#define __ADM_AUDIO_OBJECT_FILE__

#include <bbcat-adm/ADMData.h>
#include "SoundFileAttributes.h"

BBC_AUDIOTOOLBOX_START

class ADMRIFFFile;
/*--------------------------------------------------------------------------------*/
/** A version of SoundFileSamples specifically for ADM based files
 */
/*--------------------------------------------------------------------------------*/
class ADMAudioFileSamples : public SoundFileSamples
{
public:
  ADMAudioFileSamples(const ADMRIFFFile&        file);
  ADMAudioFileSamples(const SoundFileSamples    *isamples, const ADMData *_adm);
  ADMAudioFileSamples(const ADMAudioFileSamples *isamples);
  virtual ~ADMAudioFileSamples();

  virtual const std::vector<ADMTrackCursor *>& GetCursors() const {return cursors;}

  const ADMData *GetADM() const {return adm;}

  virtual bool Add(const ADMAudioObject *obj);
  virtual bool Add(const ADMAudioObject *objs[], uint_t n);
  virtual bool Add(const std::vector<const ADMAudioObject *>& objs);

  /*--------------------------------------------------------------------------------*/
  /** Get list of audio objects active at *current* time
   */
  /*--------------------------------------------------------------------------------*/
  virtual void GetObjectList(ADMAudioObject::LIST& list);

  /*--------------------------------------------------------------------------------*/
  /** Seek to specified time on all cursors
   */
  /*--------------------------------------------------------------------------------*/
  virtual void SeekAllCursors(uint64_t t);

  virtual ADMAudioFileSamples *Duplicate() const {return new ADMAudioFileSamples(this);}

protected:
  const ADMData                       *adm;
  std::vector<ADMTrackCursor *>       cursors;
  std::vector<const ADMAudioObject *> objects;
  Clip_t                              initialclip;
};

BBC_AUDIOTOOLBOX_END

#endif
