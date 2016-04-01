
#define BBCDEBUG_LEVEL 1
#include "SoundObjectFile.h"

BBC_AUDIOTOOLBOX_START

SoundObjectFileSamples::SoundObjectFileSamples() : SoundFileSamples(),
                                                   adm(NULL)
{
}

SoundObjectFileSamples::SoundObjectFileSamples(const SoundFileSamples *obj) : SoundFileSamples(obj),
                                                                              adm(NULL)
{
}

SoundObjectFileSamples::SoundObjectFileSamples(const SoundObjectFileSamples *obj) : SoundFileSamples(obj),
                                                                                    adm(obj->adm)
{
}

SoundObjectFileSamples::~SoundObjectFileSamples()
{
  uint_t i;

  for (i = 0; i < cursors.size(); i++)
  {
    delete cursors[i];
  }
}   

/*--------------------------------------------------------------------------------*/
/** Get list of audio objects active at *current* time
 */
/*--------------------------------------------------------------------------------*/
void SoundObjectFileSamples::GetObjectList(ADMAudioObject::LIST& list)
{
  ADMAudioObject *lastobj = NULL;
  uint_t i;

  for (i = 0; i < cursors.size(); i++)
  {
    ADMAudioObject *obj = cursors[i]->GetAudioObject();

    if (obj && (obj != lastobj)) list.push_back(obj);
    
    lastobj = obj;
  }  
}

/*--------------------------------------------------------------------------------*/
/** Seek to specified time on all cursors
 */
/*--------------------------------------------------------------------------------*/
void SoundObjectFileSamples::SeekAllCursors(uint64_t t)
{
  uint_t i;
  
  for (i = 0; i < cursors.size(); i++)
  {
    cursors[i]->Seek(t);
  }
}

BBC_AUDIOTOOLBOX_END
