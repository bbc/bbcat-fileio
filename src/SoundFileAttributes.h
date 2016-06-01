#ifndef __SOUND_FILE_ATTRIBUTES__
#define __SOUND_FILE_ATTRIBUTES__

#include <string>

#include <bbcat-base/misc.h>
#include <bbcat-base/EnhancedFile.h>
#include <bbcat-base/UniversalTime.h>
#include <bbcat-base/RefCount.h>

#include <bbcat-dsp/SoundFormatConversions.h>

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** Sound format object - manages the format of audio data within a file
 */
/*--------------------------------------------------------------------------------*/
class SoundFormat
{
public:
  SoundFormat();
  virtual ~SoundFormat();

  virtual uint32_t       GetSampleRate()       const {return samplerate;}
  virtual uint_t         GetChannels()         const {return channels;}
  virtual uint8_t        GetBytesPerSample()   const {return bytespersample;}
  virtual uint8_t        GetBitsPerSample()    const {return bbcat::GetBitsPerSample(format);}
  virtual uint_t         GetBytesPerFrame()    const {return channels * bytespersample;}
  virtual SampleFormat_t GetSampleFormat()     const {return format;}
  virtual bool           GetSamplesBigEndian() const {return bigendian;}

  virtual void           SetSampleRate(uint32_t val)         {samplerate = val; timebase.SetDenominator(samplerate); timebase.Reset();}
  virtual void           SetChannels(uint_t val)             {channels = val;}
  virtual void           SetSampleFormat(SampleFormat_t val) {format = val; bytespersample = bbcat::GetBytesPerSample(format);}
  virtual void           SetSamplesBigEndian(bool val)       {bigendian = val;}

  const UniversalTime&   GetTimeBase() const {return timebase;}

protected:
  uint32_t       samplerate;
  uint_t         channels;
  uint8_t        bytespersample;
  SampleFormat_t format;
  bool           bigendian;
  UniversalTime  timebase;
};

/*--------------------------------------------------------------------------------*/
/** Sound smaples object - manages the audio data within a file
 */
/*--------------------------------------------------------------------------------*/
class SoundFileSamples
{
public:
  SoundFileSamples();
  SoundFileSamples(const SoundFileSamples *obj);
  virtual ~SoundFileSamples();

  virtual void SetSampleBufferSize(uint_t samples = 256) {samplebufferframes = samples; UpdateData();}

  virtual void SetFormat(const SoundFormat *format);
  const SoundFormat *GetFormat() const {return format;}
  virtual void SetFile(const RefCount<EnhancedFile>& file, uint64_t pos, uint64_t bytes, bool readonly = true);

  /*--------------------------------------------------------------------------------*/
  /** Return whether read or write error has occurred
   */
  /*--------------------------------------------------------------------------------*/
  bool     InError() const               {return inerror;}
  
  uint_t   GetStartChannel()             const {return clip.channel;}
  uint_t   GetChannels()                 const {return clip.nchannels;}

  uint64_t GetSamplePosition()           const {return samplepos;}
  uint64_t GetSampleLength()             const {return clip.nsamples;}
  uint64_t GetAbsoluteSamplePosition()   const {return clip.start + samplepos;}
  uint64_t GetAbsoluteSampleLength()     const {return clip.start + clip.nsamples;}

  void     SetSamplePosition(uint64_t pos)         {samplepos = std::min(pos, clip.nsamples); inerror = false; UpdatePosition();}
  void     SetAbsoluteSamplePosition(uint64_t pos) {samplepos = limited::limit(pos, clip.start, clip.start + clip.nsamples) - clip.start; inerror = false; UpdatePosition();}

  uint64_t GetPositionNS()              const {return timebase.Calc(GetSamplePosition());}
  double   GetPositionSeconds()         const {return timebase.CalcSeconds(GetSamplePosition());}

  uint64_t GetAbsolutePositionNS()      const {return timebase.GetTime();}
  double   GetAbsolutePositionSeconds() const {return timebase.GetTimeSeconds();}

  uint64_t GetLengthNS()                const {return timebase.Calc(GetSampleLength());}
  double   GetLengthSeconds()           const {return timebase.CalcSeconds(GetSampleLength());}

  uint64_t GetAbsoluteLengthNS()        const {return timebase.GetTime();}
  double   GetAbsoluteLengthSeconds()   const {return timebase.GetTimeSeconds();}

  const UniversalTime& GetTimeBase()    const {return timebase;}

  typedef struct
  {
    uint64_t start;
    uint64_t nsamples;
    uint_t   channel;
    uint_t   nchannels;
  } Clip_t;
  const Clip_t& GetClip() const {return clip;}
  void SetClip(const Clip_t& newclip);

  virtual uint_t ReadSamples(uint8_t  *buffer, SampleFormat_t type, uint_t dstchannel, uint_t ndstchannels, uint_t frames, uint_t firstchannel = 0, uint_t nchannels = ~0);
  virtual uint_t ReadSamples(sint16_t *dst, uint_t dstchannel, uint_t ndstchannels, uint_t frames, uint_t firstchannel = 0, uint_t nchannels = ~0) {return ReadSamples((uint8_t *)dst, SampleFormatOf(dst), dstchannel, ndstchannels, frames, firstchannel, nchannels);}
  virtual uint_t ReadSamples(sint32_t *dst, uint_t dstchannel, uint_t ndstchannels, uint_t frames, uint_t firstchannel = 0, uint_t nchannels = ~0) {return ReadSamples((uint8_t *)dst, SampleFormatOf(dst), dstchannel, ndstchannels, frames, firstchannel, nchannels);}
  virtual uint_t ReadSamples(float    *dst, uint_t dstchannel, uint_t ndstchannels, uint_t frames, uint_t firstchannel = 0, uint_t nchannels = ~0) {return ReadSamples((uint8_t *)dst, SampleFormatOf(dst), dstchannel, ndstchannels, frames, firstchannel, nchannels);}
  virtual uint_t ReadSamples(double   *dst, uint_t dstchannel, uint_t ndstchannels, uint_t frames, uint_t firstchannel = 0, uint_t nchannels = ~0) {return ReadSamples((uint8_t *)dst, SampleFormatOf(dst), dstchannel, ndstchannels, frames, firstchannel, nchannels);}

  virtual uint_t WriteSamples(const uint8_t  *buffer, SampleFormat_t type, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1, uint_t firstchannel = 0, uint_t nchannels = ~0);
  virtual uint_t WriteSamples(const sint16_t *src, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1, uint_t firstchannel = 0, uint_t nchannels = ~0) {return WriteSamples((const uint8_t *)src, SampleFormatOf(src), srcchannel, nsrcchannels, nsrcframes, firstchannel, nchannels);}
  virtual uint_t WriteSamples(const sint32_t *src, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1, uint_t firstchannel = 0, uint_t nchannels = ~0) {return WriteSamples((const uint8_t *)src, SampleFormatOf(src), srcchannel, nsrcchannels, nsrcframes, firstchannel, nchannels);}
  virtual uint_t WriteSamples(const float    *src, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1, uint_t firstchannel = 0, uint_t nchannels = ~0) {return WriteSamples((const uint8_t *)src, SampleFormatOf(src), srcchannel, nsrcchannels, nsrcframes, firstchannel, nchannels);}
  virtual uint_t WriteSamples(const double   *src, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1, uint_t firstchannel = 0, uint_t nchannels = ~0) {return WriteSamples((const uint8_t *)src, SampleFormatOf(src), srcchannel, nsrcchannels, nsrcframes, firstchannel, nchannels);}

protected:
  virtual void UpdateData();
  virtual void UpdatePosition() {timebase.Set(GetAbsoluteSamplePosition());}

protected:
  const SoundFormat      *format;
  UniversalTime          timebase;
  RefCount<EnhancedFile> fileref;
  Clip_t                 clip;
  uint64_t               filepos;
  uint64_t               samplepos;
  uint64_t               totalsamples;
  uint64_t               totalbytes;
  uint8_t                *samplebuffer;
  uint_t                 samplebufferframes;
  bool                   readonly;
  bool                   inerror;
};

BBC_AUDIOTOOLBOX_END

#endif
