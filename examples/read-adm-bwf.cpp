
#include <stdio.h>
#include <stdlib.h>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-fileio/ADMRIFFFile.h>
#include <bbcat-fileio/ADMAudioFileSamples.h>

using namespace bbcat;

// ensure the version numbers of the linked libraries and registered
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_base_version);
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_dsp_version);
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_adm_version);
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_fileio_version);

// ensure the TinyXMLADMData object file is kept in the application
BBC_AUDIOTOOLBOX_REQUIRE(TinyXMLADMData);

int main(int argc, char *argv[])
{
  // print library versions (the actual loaded versions, if dynamically linked)
  printf("Versions:\n%s\n", LoadedVersions::Get().GetVersionsList().c_str());

  if (argc < 2)
  {
    fprintf(stderr, "Usage: read-adm-bwf <bwf-file>\n");
    exit(1);
  }

  // ADM aware WAV file
  ADMRIFFFile file;

  if (file.Open(argv[1]))
  {
    // a list of ADM object types to list
    static const char *types[] = {
      "audioProgramme",
      "audioContent",
      "audioObject",
      "audioChannelFormat",
      "audioTrackUID",
    };
    // get access to ADM data object
    const ADMData *adm = file.GetADM();
    uint_t j;

    printf("Opened '%s' okay, %u channels at %luHz (%u bytes per sample)\n", argv[1], file.GetChannels(), (ulong_t)file.GetSampleRate(), (uint_t)file.GetBytesPerSample());

    // list ADM objects of different types
    for (j = 0; j < NUMBEROF(types); j++)
    {
      // list objects of a certain type
      std::vector<const ADMObject *>list;
      const char *type = types[j];
      uint_t k;

      // get list of objects of specified type
      adm->GetObjects(type, list);

      if (list.size() > 0)
      {
        printf("Objects of type '%s':\n", type);
          
        for (k = 0; k < list.size(); k++)
        {
          const ADMObject *obj = list[k];

          // print out information about the object
          printf("  %s\n", obj->ToString().c_str());
        }

        printf("\n");
      }
      else printf("No objects of type '%s'!\n", type);
    }

    // access the audio of an audio object
    {
      std::vector<const ADMObject *>list;
        
      // get a list of audioObjects
      adm->GetObjects("audioObject", list);

      if (list.size() > 0)
      {
        const ADMAudioObject *obj;

        // pick last object
        if ((obj = dynamic_cast<const ADMAudioObject *>(list.back())) != NULL)
        {
          // create audio samples handler for the audio object
          ADMAudioFileSamples   handler(file.GetSamples(), obj);
          std::vector<Sample_t> samples;
          uint_t n, nsamples = 1024;

          printf("Reading audio from %s (%u channels)\n", obj->ToString().c_str(), handler.GetChannels());

          // make buffer for 1024 frames worth of samples
          samples.resize(handler.GetChannels() * nsamples);

          // read samples (start channel and interleaving are handled by handler)
          while ((handler.GetSamplePosition() < 16384) && ((n = handler.ReadSamples(&samples[0], 0, handler.GetChannels(), nsamples)) > 0))     // arbitrary stop point of 16384!
          {
            printf("%u/%u sample frames read, sample position %s/%s\n", n, nsamples, StringFrom(handler.GetSamplePosition()).c_str(), StringFrom(handler.GetSampleLength()).c_str());
          }
        }
      }
      else fprintf(stderr, "No audio objects in '%s'!\n", argv[1]);
    }

    // access raw audio data of entire file
    {
      // get audio samples handler for entire file
      SoundFileSamples *handler = file.GetSamples();

      UNUSED_PARAMETER(handler);

      /*
       * handler->ReadSamples(...) works just like handler.ReadSamples(...) above but on the entire file instead of just a single audio object
       *
       */
    }
  }
  else fprintf(stderr, "Failed to open file '%s' for reading!\n", argv[1]);

  return 0;
}

