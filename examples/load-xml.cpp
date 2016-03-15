
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, const char *argv[])
{
  // print library versions (the actual loaded versions, if dynamically linked)
  printf("Versions:\n%s\n", LoadedVersions::Get().GetVersionsList().c_str());

  XMLADMData *adm;
  if ((adm = XMLADMData::CreateADM()) != NULL)
  {
    int i;
    
    for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-chna") == 0)
      {
        if (!adm->ReadChnaFromFile(argv[++i])) fprintf(stderr, "Failed to read Chna from '%s''\n", argv[i]);
      }
      else if (strcmp(argv[i], "-write") == 0)
      {
        EnhancedFile f;
        if (f.fopen(argv[++i], "w"))
        {
          f.fprintf("%s", adm->GetAxml().c_str());
          f.fclose();

          printf("Wrote XML to '%s'\n", argv[i]);
        }
        else fprintf(stderr, "Failed to open file '%s' for writing\n", argv[i]);
      }
      else if (!adm->ReadXMLFromFile(argv[i])) fprintf(stderr, "Failed to read XML from '%s'\n", argv[i]);
    }

    printf("XML: %s", adm->GetAxml().c_str());

    delete adm;
  }
  else fprintf(stderr, "Failed to create blank ADM!\n");

  return 0;
}
