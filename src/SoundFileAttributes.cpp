
#include <string.h>

#define BBCDEBUG_LEVEL 1
#include "SoundFileAttributes.h"

BBC_AUDIOTOOLBOX_START

SoundFormat::SoundFormat() : samplerate(0),
                             channels(0),
                             bytespersample(0),
                             format(SampleFormat_Unknown),
                             bigendian(false)
{
}

SoundFormat::~SoundFormat()
{
}

/*----------------------------------------------------------------------------------------------------*/

SoundFileSamples::SoundFileSamples() :
  format(NULL),
  filepos(0),
  samplepos(0),
  totalsamples(0),
  totalbytes(0),
  samplebuffer(NULL),
  samplebufferframes(256),
  readonly(true)
{
  memset(&clip, 0, sizeof(clip));
}

SoundFileSamples::SoundFileSamples(const SoundFileSamples *obj) :
  format(NULL),
  filepos(0),
  samplepos(0),
  totalsamples(0),
  totalbytes(0),
  samplebuffer(NULL),
  samplebufferframes(256),
  readonly(true)
{
  memset(&clip, 0, sizeof(clip));

  SetFormat(obj->GetFormat());
  SetFile(obj->fileref, obj->filepos, obj->totalbytes);
  SetClip(obj->GetClip());
}

SoundFileSamples::~SoundFileSamples()
{
  if (samplebuffer) delete[] samplebuffer;

  EnhancedFile *file;
  if ((file = fileref.Obj()) != NULL)
  {
    std::string filename = file->getfilename();

    file->fclose();

    // DON'T delete file object here, it will be done by the fileref object on destruction
  }
}

void SoundFileSamples::SetFormat(const SoundFormat *format)
{
  this->format = format;
  UpdateData();
}

void SoundFileSamples::SetFile(const RefCount<EnhancedFile>& file, uint64_t pos, uint64_t bytes, bool readonly)
{
  // use file reference to control deletion
  fileref    = file;

  filepos    = pos;
  totalbytes = bytes;

  this->readonly = readonly;

  UpdateData();
}

void SoundFileSamples::SetClip(const Clip_t& newclip)
{
  clip = newclip;
  clip.start     = std::min(clip.start,     totalsamples);
  clip.nsamples  = std::min(clip.nsamples,  totalsamples - clip.start);
  clip.channel   = std::min(clip.channel,   format->GetChannels());
  clip.nchannels = std::min(clip.nchannels, format->GetChannels() - clip.channel);

  samplepos = std::min(samplepos, clip.nsamples);
  UpdatePosition();
}

uint_t SoundFileSamples::ReadSamples(uint8_t *buffer, SampleFormat_t type, uint_t dstchannel, uint_t ndstchannels, uint_t frames, uint_t firstchannel, uint_t nchannels)
{
  EnhancedFile *file = fileref;
  uint_t n = 0;

  if (file && file->isopen() && samplebuffer)
  {
    frames = (uint_t)std::min((uint64_t)frames, clip.nsamples - samplepos);

    if (!frames)
    {
      BBCDEBUG3(("No sample data left (pos = %s, nsamples = %s)!", StringFrom(samplepos).c_str(), StringFrom(clip.nsamples).c_str()));
    }

    firstchannel = std::min(firstchannel, clip.nchannels);
    nchannels    = std::min(nchannels,    clip.nchannels - firstchannel);

    dstchannel   = std::min(dstchannel,   ndstchannels);
    nchannels    = std::min(nchannels,    ndstchannels - dstchannel);

    n = 0;
    if (nchannels)
    {
      while (frames)
      {
        uint_t nframes = std::min(frames, samplebufferframes);
        size_t res;

        BBCDEBUG4(("Seeking to %s", StringFrom(filepos + (clip.start + samplepos) * format->GetBytesPerFrame()).c_str()));
        if (file->fseek(filepos + samplepos * format->GetBytesPerFrame(), SEEK_SET) == 0)
        {
          BBCDEBUG4(("Reading %u x %u bytes", nframes, format->GetBytesPerFrame()));

          if ((res = file->fread(samplebuffer, format->GetBytesPerFrame(), nframes)) > 0)
          {
            nframes = (uint_t)res;

            BBCDEBUG4(("Read %u frames, extracting channels %u-%u (from 0-%u), converting and copying to destination", nframes, clip.channel + firstchannel, clip.channel + firstchannel + nchannels, format->GetChannels()));

            // de-interleave, convert and transfer samples
            TransferSamples(samplebuffer, format->GetSampleFormat(), format->GetSamplesBigEndian(), clip.channel + firstchannel, format->GetChannels(),
                            buffer, type, MACHINE_IS_BIG_ENDIAN, dstchannel, ndstchannels,
                            nchannels,
                            nframes);

            n         += nframes;
            buffer    += nframes * ndstchannels * GetBytesPerSample(type);
            frames    -= nframes;
            samplepos += nframes;
          }
          else if (res <= 0)
          {
            BBCERROR("Failed to read %u frames (%u bytes) from file, error %s", nframes, nframes * format->GetBytesPerFrame(), strerror(file->ferror()));
            break;
          }
          else
          {
            BBCDEBUG3(("No data left!"));
            break;
          }
        }
        else
        {
          BBCERROR("Failed to seek to correct position in file, error %s", strerror(file->ferror()));
          n = 0;
          break;
        }
      }
    }
    else
    {
      // no channels to transfer, just increment position and return number of requested frames
      n          = frames;
      samplepos += n;
    }

    UpdatePosition();
  }
  else BBCERROR("No file or sample buffer");

  return n;
}

uint_t SoundFileSamples::WriteSamples(const uint8_t *buffer, SampleFormat_t type, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes, uint_t firstchannel, uint_t nchannels)
{
  EnhancedFile *file = fileref;
  uint_t n = 0;

  if (file && file->isopen() && samplebuffer && !readonly)
  {
    uint_t bpf = format->GetBytesPerFrame();

    firstchannel = std::min(firstchannel, clip.nchannels);
    nchannels    = std::min(nchannels,    clip.nchannels - firstchannel);

    srcchannel   = std::min(srcchannel,   nsrcchannels);
    nchannels    = std::min(nchannels,    nsrcchannels - srcchannel);

    n = 0;
    if (nchannels)
    {
      while (nsrcframes)
      {
        uint_t nframes = std::min(nsrcframes, samplebufferframes);
        size_t res;

        if (nchannels < format->GetChannels())
        {
          // read existing sample data to allow overwriting of channels
          res = file->fread(samplebuffer, bpf, nframes);

          // clear rest of buffer
          if (res < (bpf * nframes)) memset(samplebuffer + res * bpf, 0, (nframes - res) * bpf);

          // move back in file for write
          if (res) file->fseek(-(long)(res * bpf), SEEK_CUR);
        }

        // copy/interleave/convert samples
        TransferSamples(buffer, type, MACHINE_IS_BIG_ENDIAN, srcchannel, nsrcchannels,
                        samplebuffer, format->GetSampleFormat(), format->GetSamplesBigEndian(), clip.channel + firstchannel, nchannels,
                        ~0,       // number of channels actually transfer will be limited by nsrcchannels and nchannels above
                        nframes);

        if ((res = file->fwrite(samplebuffer, bpf, nframes)) > 0)
        {
          nframes     = (uint_t)res;
          n          += nframes;
          buffer     += nframes * nsrcchannels * GetBytesPerSample(type);
          nsrcframes -= nframes;
          samplepos  += nframes;

          totalsamples  = std::max(totalsamples,  samplepos);
          clip.nsamples = std::max(clip.nsamples, totalsamples - clip.start);

          totalbytes    = totalsamples * format->GetBytesPerFrame();
        }
        else if (res <= 0)
        {
          BBCERROR("Failed to write %u frames (%u bytes) to file, error %s", nframes, nframes * bpf, strerror(file->ferror()));
          break;
        }
        else
        {
          BBCDEBUG3(("No data left!"));
          break;
        }
      }
    }
    else
    {
      // no channels to transfer, just increment position and return number of requested frames
      n          = nsrcframes;
      samplepos += n;
    }

    UpdatePosition();
  }
  else BBCERROR("No file or sample buffer");

  return n;
}

void SoundFileSamples::UpdateData()
{
  if (format)
  {
    totalsamples = totalbytes / format->GetBytesPerFrame();
    if (samplebuffer) delete[] samplebuffer;
    samplebuffer = new uint8_t[samplebufferframes * format->GetChannels() * sizeof(double)];

    Clip_t newclip =
    {
      0, ~(uint64_t)0,
      0, ~(uint_t)0,
    };
    SetClip(newclip);

    timebase = format->GetTimeBase();
  }
}

BBC_AUDIOTOOLBOX_END
