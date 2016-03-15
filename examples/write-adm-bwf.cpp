
#include <stdio.h>
#include <stdlib.h>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-fileio/ADMRIFFFile.h>

using namespace bbcat;

// ensure the version numbers of the linked libraries and registered
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_base_version);
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_dsp_version);
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_adm_version);
BBC_AUDIOTOOLBOX_REQUIRE(bbcat_fileio_version);

// ensure the TinyXMLADMData object file is kept in the application
BBC_AUDIOTOOLBOX_REQUIRE(TinyXMLADMData);

int main(void)
{
  // print library versions (the actual loaded versions, if dynamically linked)
  printf("Versions:\n%s\n", LoadedVersions::Get().GetVersionsList().c_str());

  // ADM aware WAV file
  ADMRIFFFile file;
  const char *filename = "adm-bwf.wav";
  
  // IMPORTANT: create basic ADM here - if this is not done, the file will be a plain WAV file!
  file.CreateADM();

  // create file
  if (file.Create(filename, 48000, 5))
  {
    ADMData *adm = file.GetADM();

    if (adm)
    {
      ADMData::OBJECTNAMES names;

      printf("Created '%s' okay, %u channels at %luHz (%u bytes per sample)\n", filename, file.GetChannels(), (ulong_t)file.GetSampleRate(), (uint_t)file.GetBytesPerSample());

      // set programme name
      // if an audioProgramme object of this name doesn't exist, one will be created
      names.programmeName = "ADM Test Programme";

      // set content name
      // if an audioContent object of this name doesn't exist, one will be created
      names.contentName   = "ADM Test Content";
    
      // create tracks, channels and streams
      uint_t t;
      const ADMData::TRACKLIST& tracklist = adm->GetTrackList();
      for (t = 0; t < tracklist.size(); t++)
      {
        std::string trackname;

        printf("------------- Track %2u -----------------\n", t + 1);

        // create default audioTrackFormat name (used for audioStreamFormat objects as well)
        Printf(trackname, "Track %u", t + 1);

        names.trackNumber = t;

        // derive channel and stream names from track name
        names.channelFormatName = trackname;
        names.streamFormatName  = "PCM_" + trackname;
        names.trackFormatName   = "PCM_" + trackname;

        // set object name
        // create 4 objects, each of 4 tracks
        names.objectName = "";  // need this because Printf() APPENDS!
        Printf(names.objectName, "Object %u", 1 + (t / 4));
        
        // set pack name from object name
        // create 4 packs, each of 4 tracks
        names.packFormatName = "";  // need this because Printf() APPENDS!
        Printf(names.packFormatName, "Pack %u", 1 + (t / 4));

        // set default typeLabel
        names.typeLabel = ADMObject::TypeLabel_Objects;

        adm->CreateObjects(names);

        // note how the programme and content names are left in place in 'names'
        // this is necessary to ensure that things are linked up properly
      }
    }
    else fprintf(stderr, "File does not have an ADM associated with it, did you forget to create one?\n");
    
    // write audio
    std::vector<float> audio;
    audio.resize(file.GetChannels());
    double fs = (double)file.GetSampleRate();
    double level = dBToGain(-40.0);
    uint_t i, j, nsamples = 2 * file.GetSampleRate(), step = file.GetSampleRate() / 10;

    // write 20s worth samples
    // this is somewhat inefficient since it is only writing one frame at a time!
    for (i = 0; i < nsamples; i++)
    {
      // every 1/10s set positions of channels
      if (!(i % step))
      {
        uint_t index = i / step;

        for (j = 0; j < audio.size(); j++)
        {
          AudioObjectParameters params;
          Position pos;

          pos.polar  = true;
          pos.pos.az = (double)(index + j) * 20.0;
          pos.pos.el = (double)j / (double)file.GetChannels() * 60.0;
          pos.pos.d  = 1.0;
          params.SetPosition(pos);

          // set some extra parameters
          params.SetWidth((float)j * .2);
          params.SetDepth((float)j * .4);
          params.SetHeight((float)j * .3);
          
          if (j < 2)
          {
            params.SetScreenEdgeLock("azimuth", j ? "right" : "left");
            params.SetScreenEdgeLock("elevation", j ? "bottom" : "top");
          }
          else if (j == 2)
          {
            params.AddExcludedZone("left",  -1, -1, -1, -.8, 1, 1);
            params.AddExcludedZone("right", .8, -1, -1, 1.0, 1, 1);
          }
          else
          {
            params.SetObjectImportance(5 + j - 3);
            params.SetChannelImportance(2 + j - 3);
            params.SetInteract(j != 3);
            params.SetDisableDucking(j == 3);
          }
          file.SetObjectParameters(j, params);
        }
      }

      // create sine waves on each channel
      for (j = 0; j < audio.size(); j++)
      {
        audio[j] = level * sin(2.0 * M_PI * 100.0 * (double)i / fs * (double)(1 + j));
      }

      // write a frame of audio
      file.WriteSamples(&audio[0], 0, audio.size(), 1);
    }

    file.Close();
  }
  else fprintf(stderr, "Failed to open file '%s' for writing!\n", filename);
  
  return 0;
}

