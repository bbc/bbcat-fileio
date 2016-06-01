
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-fileio/ADMRIFFFile.h>

using namespace bbcat;

BBC_AUDIOTOOLBOX_START
extern bool bbcat_register_bbcat_fileio();
BBC_AUDIOTOOLBOX_END

int main(int argc, const char *argv[])
{
  // ensure libraries are set up
  bbcat_register_bbcat_fileio();

  XMLADMData *adm;
  if ((adm = XMLADMData::CreateADM()) != NULL)
  {
    int i;
    
    for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-chna") == 0)
      {
        if (!adm->ReadChnaFromFile(argv[++i], false)) fprintf(stderr, "Failed to read Chna from '%s''\n", argv[i]);
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
      else if (!adm->ReadXMLFromFile(argv[i], false)) fprintf(stderr, "Failed to read XML from '%s'\n", argv[i]);
    }

    adm->Finalise();
    
    printf("XML: %s", adm->GetAxml().c_str());

    delete adm;
  }
  else fprintf(stderr, "Failed to create blank ADM!\n");

  return 0;
}
