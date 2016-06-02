
#include <stdio.h>
#include <stdlib.h>

#include <bbcat-fileio/XMLADMData.h>
#include <bbcat-fileio/register.h>

using namespace bbcat;

int main(int argc, char *argv[])
{
  // ensure libraries are set up
  bbcat_register_bbcat_fileio();

 if (argc < 2)
  {
    fprintf(stderr, "Usage: adm-to-json <xml-file>\n");
    exit(1);
  }

  // create basic ADM
  XMLADMData *adm;
  if ((adm = XMLADMData::CreateADM()) != NULL)
  {
    if (adm->ReadXMLFromFile(argv[1])) {
      std::vector<const ADMObject *> channelformats;
      uint_t i;

      printf("XML: %s", adm->GetAxml().c_str());
      
      adm->GetObjects(ADMAudioChannelFormat::Type, channelformats);

      printf("%u channel formats found\n", (uint_t)channelformats.size());
      
      for (i = 0; i < channelformats.size(); i++)
      {
        const ADMAudioChannelFormat *cf = dynamic_cast<const ADMAudioChannelFormat *>(channelformats[i]);

        if (cf)
        {
          const std::vector<ADMAudioBlockFormat *>& blockformats = cf->GetBlockFormatRefs();
          uint_t j;
          
          printf("ChannelFormat %u/%u: %u block formats\n", i + 1, (uint_t)channelformats.size(), (uint_t)blockformats.size());

          for (j = 0; j < blockformats.size(); j++)
          {
            printf("Object %u/%u: %s\n", j + 1, (uint_t)blockformats.size(), blockformats[j]->GetObjectParameters().ToJSONString().c_str());
          }

          printf("\n");
        }
      }
    }
    else BBCERROR("Failed to read XML from '%s'\n", argv[1]);
    
    delete adm;
  }
  
  return 0;
}
