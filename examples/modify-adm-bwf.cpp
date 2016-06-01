
#include <stdio.h>
#include <stdlib.h>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-adm/ADMXMLGenerator.h>
#include <bbcat-fileio/ADMRIFFFile.h>
#include <bbcat-fileio/ADMAudioFileSamples.h>

using namespace bbcat;

BBC_AUDIOTOOLBOX_START
extern bool bbcat_register_bbcat_fileio();
BBC_AUDIOTOOLBOX_END

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
      // copy ADM to destination
      dstfile.GetADM()->Copy(*srcfile.GetADM());

      {
        // adds extra data to ADM (at root node):
        //
        // <additionaldata>
        //  <data index="0" subtype="integer" type="numerical">7</data>
        // 	<data index="1" subtype="decimal" type="numerical">3.4</data>
        // 	<data index="2" type="text">text</data>
        // </additionaldata>
        //
        // there is no limit to the size of the tree that can be added
        //
        XMLValues *values = dstfile.GetADM()->CreateNonADMXML("");

        if (values) {
          XMLValues subvalues;
          XMLValue  object;
          
          // add extra layer (just set name because sub values will be added)
          object.SetValue("additionaldata", "");
          
          // add values to subvalues
          {
            XMLValue value;

            value.SetValue("data", "7");
            value.SetAttribute("index", "0");
            value.SetAttribute("type", "numerical");
            value.SetAttribute("subtype", "integer");

            subvalues.AddValue(value);
          }
          {
            XMLValue value;

            value.SetValue("data", "3.4");
            value.SetAttribute("index", "1");
            value.SetAttribute("type", "numerical");
            value.SetAttribute("subtype", "decimal");

            subvalues.AddValue(value);
          }
          {
            XMLValue value;

            value.SetValue("data", "text");
            value.SetAttribute("index", "2");
            value.SetAttribute("type", "text");

            subvalues.AddValue(value);
          }

          // add subvalues to object
          object.AddSubValues(subvalues);

          // add object into main XML list
          values->AddValue(object);
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
      
      // copy ALL samples from src to dst
      while ((nframes = src->ReadSamples(&buffer[0], format, 0, nchannels, maxframes)) > 0)
      {
        uint_t nframes1;

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
