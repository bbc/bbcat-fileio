#ifndef __SOUND_FILE_WITH_POSITION__
#define __SOUND_FILE_WITH_POSITION__

#include <vector>

#include <bbcat-base/misc.h>
#include <bbcat-adm/ADMData.h>
#include <bbcat-adm/AudioObjectCursor.h>

#include "SoundFileAttributes.h"

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** Sound samples object which understands positions - syncs position updates
 * with the reading of audio samples
 */
/*--------------------------------------------------------------------------------*/
class SoundObjectFileSamples : public SoundFileSamples
{
public:
  SoundObjectFileSamples();
  SoundObjectFileSamples(const SoundFileSamples *obj);
  SoundObjectFileSamples(const SoundObjectFileSamples *obj);
  virtual ~SoundObjectFileSamples();

  virtual const std::vector<AudioObjectCursor *>& GetCursors() const {return cursors;}

  virtual void SetADMData(const ADMData *_adm) {adm = _adm;}
  const ADMData *GetADMData() const {return adm;}

  /*--------------------------------------------------------------------------------*/
  /** Get list of audio objects active at *current* time
   */
  /*--------------------------------------------------------------------------------*/
  virtual void GetObjectList(ADMAudioObject::LIST& list);

  /*--------------------------------------------------------------------------------*/
  /** Seek to current time on all cursors
   */
  /*--------------------------------------------------------------------------------*/
  virtual void SeekAllCursors();
  
protected:
  const ADMData                    *adm;
  std::vector<AudioObjectCursor *> cursors;
};

BBC_AUDIOTOOLBOX_END

#endif
