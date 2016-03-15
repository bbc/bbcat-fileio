
#include <stdio.h>
#include <stdlib.h>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-fileio/XMLADMData.h>

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
    fprintf(stderr, "Usage: play-metadata <xml-file>\n");
    exit(1);
  }
  
  // create basic ADM
  XMLADMData *adm;
  if ((adm = XMLADMData::CreateADM()) != NULL)
  {
    if (adm->ReadXMLFromFile(argv[1]))
    {
      std::vector<ADMTrackCursor *> cursors;
      
      uint_t i, n = (uint_t)adm->GetTrackList().size();
      printf("XML file has %u tracks\n", n);
      
      // create cursors and find maximum length of objects
      uint64_t maxtime = 0;
      for (i = 0; i < n; i++)
      {
        cursors.push_back(new ADMTrackCursor(i));
        cursors[i]->Add(adm->GetAudioObjectList());
        maxtime = std::max(maxtime, cursors[i]->GetEndTime());
      }

      printf("Length is %s\n", GenerateTime(maxtime).c_str());
      
      // play metadata, detecting changes
      std::vector<AudioObjectParameters> channels(n);
      AudioObjectParameters params;
      uint64_t t, step = 5000000;  // 5ms in ns
      for (t = 0; t < maxtime; t += step)
      {
        for (i = 0; i < cursors.size(); i++)
        {
          if (cursors[i]->Seek(t) || !t)
          {
            if (cursors[i]->GetObjectParameters(params))
            {
              if (params != channels[i])
              {
                channels[i] = params;
                printf("Time %s Channel %2u: %s\n", GenerateTime(t).c_str(), i, channels[i].ToString().c_str());
              }
            }
            else printf("No valid parameters for channel %2u at %s\n", i, GenerateTime(t).c_str());
          }
        }
      }
    }
    else fprintf(stderr, "Failed to read XML from '%s'\n", argv[1]);

    delete adm;
  }

  return 0;
}
