#ifndef __ADM_AUDIO_OBJECT_FILE__
#define __ADM_AUDIO_OBJECT_FILE__

#include <bbcat-adm/ADMData.h>
#include "SoundObjectFile.h"

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** A version of SoundObjectFileSamples specifically for ADM based files
 */
/*--------------------------------------------------------------------------------*/
class ADMAudioFileSamples : public SoundObjectFileSamples
{
public:
  ADMAudioFileSamples(const SoundFileSamples *isamples, const ADMAudioObject *obj = NULL);
  ADMAudioFileSamples(const ADMAudioFileSamples *isamples);
  virtual ~ADMAudioFileSamples();

  virtual bool Add(const ADMAudioObject *obj);
  virtual bool Add(const ADMAudioObject *objs[], uint_t n);
  virtual bool Add(const std::vector<const ADMAudioObject *>& objs);

  virtual ADMAudioFileSamples *Duplicate() const {return new ADMAudioFileSamples(this);}

protected:
  Clip_t                              initialclip;
  std::vector<const ADMAudioObject *> objects;
};

BBC_AUDIOTOOLBOX_END

#endif
