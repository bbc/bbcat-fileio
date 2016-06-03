
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-adm/ADMXMLGenerator.h>
#include <bbcat-fileio/ADMRIFFFile.h>
#include <bbcat-fileio/ADMAudioFileSamples.h>
#include <bbcat-fileio/register.h>

using namespace bbcat;

int main(int argc, char *argv[])
{
  // ensure libraries are set up
  bbcat_register_bbcat_fileio();

  if (argc < 3)
  {
    fprintf(stderr, "Usage: modify-adm-bwf <input-bwf-file> <output-bwf-file>\n");
    exit(1);
  }

  // ADM aware WAV files
  ADMRIFFFile srcfile, dstfile;

  if (srcfile.Open(argv[1]))
  {
    SampleFormat_t format = srcfile.GetSampleFormat();    // this will be used later to avoid sample format conversion
    uint_t nchannels = srcfile.GetChannels();
    
    printf("Opened '%s' okay, %u channels at %luHz (%u bytes per sample)\n", argv[1], nchannels, (ulong_t)srcfile.GetSampleRate(), (uint_t)srcfile.GetBytesPerSample());

    // create ADM structure in destination BEFORE creating file
    dstfile.CreateADM();
    
    if (dstfile.Create(argv[2], srcfile.GetSampleRate(), nchannels, format))
    {
      ADMData *adm = dstfile.GetADM();
      
      // copy ADM to destination
      adm->Copy(*srcfile.GetADM());

      /*--------------------------------------------------------------------------------*/
      /** Track specific processing
       *
       * This is used to process *all* blockformats on a *specific* track
       *
       * In this case:
       * 1. Set position of track 0
       */
      /*--------------------------------------------------------------------------------*/
      // adjust position of track 0
      {
        // get list of audio objects
        std::vector<const ADMAudioObject *> objectlist;
        uint_t i, j;
      
        adm->GetAudioObjectList(objectlist);

        // find audio object on track 0 (of the WAV file)
        for (i = 0; i < (uint_t)objectlist.size(); i++)
        {
          ADMAudioChannelFormat *channelformat;

          // find channelformat of track 0 within this object (if it exists)
          if ((channelformat = objectlist[i]->GetChannelFormat(0)) != NULL)
          {
            std::vector<ADMAudioBlockFormat *>& blockformats = channelformat->GetBlockFormatRefs();

            printf("Found list of block formats for track 0\n");

            // process all block formats
            for (j = 0; j < (uint_t)blockformats.size(); j++)
            {
              AudioObjectParameters& parameters  = blockformats[j]->GetObjectParameters();

              // adjust position in block
              Position p;
              p.pos.az = 0.0;
              p.pos.el = 40.0;
              p.pos.d  = 1.0;
              p.polar  = true;
              parameters.SetPosition(p);
            }
          }
        }
      }

      /*--------------------------------------------------------------------------------*/
      /** Generic processing
       *
       * This is used to process *all* blockformats on *all* tracks
       *
       * In this case:
       * 1. Correctly set 'cartesian' ADM parameter
       * 2. Strip 'sourcetype' parameter
       */
      /*--------------------------------------------------------------------------------*/
      // process ALL blockformats
      {
        // get access to blockformats through channelsformats
        std::vector<ADMObject *> channelformats;
        uint_t i, j;
        
        adm->GetWritableObjects(ADMAudioChannelFormat::Type, channelformats);
        for (i = 0; i < (uint_t)channelformats.size(); i++)
        {
          ADMAudioChannelFormat *channelformat;

          // cast up to correct type
          if ((channelformat = dynamic_cast<ADMAudioChannelFormat *>(channelformats[i])) != NULL)
          {
            std::vector<ADMAudioBlockFormat *>& blockformats = channelformat->GetBlockFormatRefs();

            for (j = 0; j < (uint_t)blockformats.size(); j++)
            {
              // get access to modifable AudioObjectParameters for blockformat
              AudioObjectParameters& parameters  = blockformats[j]->GetObjectParameters();

              // force Cartesian parameter
              parameters.SetCartesian(!parameters.GetPosition().polar);

              // delete 'sourcetype' parameter from object parameters
              parameters.ResetOtherValue("sourcetype");
            }
          }
        }
      }
      
      //printf("XML:\n%s", ADMXMLGenerator::GetAxml(dstfile.GetADM()).c_str());
             
      // get audio samples handler for entire file
      SoundFileSamples *src = srcfile.GetSamples();
      SoundFileSamples *dst = dstfile.GetSamples();

      // create vector for big block of samples (as raw bytes)
      uint_t nframes, maxframes = 1024, pc = ~0;
      uint64_t pos = 0, length = srcfile.GetSampleLength();
      std::vector<uint8_t> buffer(maxframes * srcfile.GetChannels() * srcfile.GetBytesPerSample());

      // set number of frames to silence at beginning of file
      uint_t nsilenceframes = 0;
      
      // copy ALL samples from src to dst
      while ((nframes = src->ReadSamples(&buffer[0], format, 0, nchannels, maxframes)) > 0)
      {
        uint_t nsilenceframes1 = std::min(nsilenceframes, nframes);
        uint_t nframes1;

        if (nsilenceframes1)
        {
          memset(&buffer[0], 0, nsilenceframes1 * nchannels * srcfile.GetBytesPerSample());
          nsilenceframes -= nsilenceframes1;
        }
        
        if ((nframes1 = dst->WriteSamples(&buffer[0], format, 0, nchannels, nframes)) < nframes)
        {
          fprintf(stderr, "Unable to write all frames from source to destination (%u < %u)\n", nframes1, nframes);
          break;
        }

        // update percentage progress
        pos += nframes;
        uint_t _pc = (uint_t)((100 * pos) / length);
        if (_pc != pc)
        {
          pc = _pc;
          printf("\rCopying samples.... %u%% done", pc);
          fflush(stdout);
        }
      }

      printf("\n");

      dstfile.Close();
    }

    srcfile.Close();
  }
  else fprintf(stderr, "Failed to open file '%s' for reading!\n", argv[1]);

  return 0;
}
