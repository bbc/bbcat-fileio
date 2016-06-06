
#include <catch/catch.hpp>

#include "ADMRIFFFile.h"

USE_BBC_AUDIOTOOLBOX

void reporterrors(const ADMData *adm)
{
  if (adm->GetErrorCount() > 0)
  {
    const std::vector<std::string>& errors = adm->GetErrors();
    uint_t i;
    
    printf("--------------------------------------------------------------------------------\n");
    printf("Errors found during processing:\n");
    for (i = 0; i < (uint_t)errors.size(); i++)
    {
      printf("  %s\n", errors[i].c_str());
    }
    printf("--------------------------------------------------------------------------------\n");
  }
}

TEST_CASE("comparison")
{
  const std::string testfiles[] =
  {
    "test1.xml",
    "test2.xml",
    "test3.xml",
  };
  uint_t i;

#ifdef CMAKE_BUILD
      std::string build = "cmake";
#else
      std::string build = "autotools";
#endif
      
#ifdef TARGET_OS_WINDOWS
      std::string platform = "win64";
#endif
#ifdef TARGET_OS_UNIXBSD
#ifdef __LINUX__
      std::string platform = "linux";
#else
      std::string platform = "mac";
#endif      
#endif

  for (i = 0; i < NUMBEROF(testfiles); i++)
  {
    XMLADMData *adm1 = XMLADMData::CreateADM();
    XMLADMData *adm2 = XMLADMData::CreateADM();
    const std::string& testfile = testfiles[i];
    
    REQUIRE(adm1 != NULL);
    REQUIRE(adm2 != NULL);

    if (adm1)
    {
      EnhancedFile f;
      
      REQUIRE(adm1->ReadXMLFromFile(testfile) == true);

      CHECK(adm1->GetErrorCount() == 0);
      reporterrors(adm1);

      // create build / target specific output filename
      std::string output = "res-" + build + "-" + platform + "-" + testfiles[i];
      REQUIRE(f.fopen(output.c_str(), "wb") == true);
      if (f.isopen())
      {
        f.fprintf("%s", adm1->GetAxml().c_str());
        f.fclose();

        if (adm2)
        {
          CHECK(adm2->ReadXMLFromFile(output.c_str()) == true);

          CHECK(adm1->GetErrorCount() == 0);
          reporterrors(adm1);

          // check that axml's are identical
          CHECK(adm1->GetAxml() == adm2->GetAxml());
        }
      }
    }

    if (adm2) delete adm2;
    if (adm1) delete adm1;
  }
}

TEST_CASE("standarddefs")
{
  XMLADMData *adm1 = XMLADMData::CreateADM();

  REQUIRE(adm1 != NULL);

  ADMData::OBJECTNAMES names;

  // set programme name
  // if an audioProgramme object of this name doesn't exist, one will be created
  names.programmeName = "ADM 5.1 test signal";

  // set content name
  // if an audioContent object of this name doesn't exist, one will be created
  names.contentName   = "ADM 5.1 content";

  // create tracks, channels and streams

  names.trackNumber       = 0;
  names.channelFormatName = "FrontLeft";
  names.streamFormatName  = "PCM_FrontLeft";
  names.trackFormatName   = "PCM_FrontLeft";

  names.objectName = "5.1 test audio object";
  names.packFormatName = "urn:itu:bs:2051:0:pack:5.1_(0+5+0)";
  names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;

  REQUIRE(adm1->CreateObjects(names) == true);

  REQUIRE(names.objects.programme != NULL);
  REQUIRE(names.objects.content != NULL);
  REQUIRE(names.objects.object != NULL);

  REQUIRE(names.objects.channelFormat != NULL);
  REQUIRE(names.objects.channelFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.streamFormat != NULL);
  REQUIRE(names.objects.streamFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.trackFormat != NULL);
  REQUIRE(names.objects.trackFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.packFormat != NULL);
  REQUIRE(names.objects.packFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions

  names.trackNumber       = 1;
  names.channelFormatName = "FrontRight";
  names.streamFormatName  = "PCM_FrontRight";
  names.trackFormatName   = "PCM_FrontRight";

  names.objectName = "5.1 test audio object";
  names.packFormatName = "urn:itu:bs:2051:0:pack:5.1_(0+5+0)";
  names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;

  REQUIRE(adm1->CreateObjects(names) == true);

  REQUIRE(names.objects.channelFormat != NULL);
  REQUIRE(names.objects.channelFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.streamFormat != NULL);
  REQUIRE(names.objects.streamFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.trackFormat != NULL);
  REQUIRE(names.objects.trackFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.packFormat != NULL);
  REQUIRE(names.objects.packFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions

  names.trackNumber       = 2;
  names.channelFormatName = "FrontCentre";
  names.streamFormatName  = "PCM_FrontCentre";
  names.trackFormatName   = "PCM_FrontCentre";

  names.objectName = "5.1 test audio object";
  names.packFormatName = "urn:itu:bs:2051:0:pack:5.1_(0+5+0)";
  names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;

  REQUIRE(adm1->CreateObjects(names) == true);

  REQUIRE(names.objects.channelFormat != NULL);
  REQUIRE(names.objects.channelFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.streamFormat != NULL);
  REQUIRE(names.objects.streamFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.trackFormat != NULL);
  REQUIRE(names.objects.trackFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.packFormat != NULL);
  REQUIRE(names.objects.packFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions

  names.trackNumber       = 3;
  names.channelFormatName = "LowFrequencyEffects";
  names.streamFormatName  = "PCM_LowFrequencyEffects";
  names.trackFormatName   = "PCM_LowFrequencyEffects";

  names.objectName = "5.1 test audio object";
  names.packFormatName = "urn:itu:bs:2051:0:pack:5.1_(0+5+0)";
  names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;

  REQUIRE(adm1->CreateObjects(names) == true);

  REQUIRE(names.objects.channelFormat != NULL);
  REQUIRE(names.objects.channelFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.streamFormat != NULL);
  REQUIRE(names.objects.streamFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.trackFormat != NULL);
  REQUIRE(names.objects.trackFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.packFormat != NULL);
  REQUIRE(names.objects.packFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions

  names.trackNumber       = 4;
  names.channelFormatName = "SurroundLeft";
  names.streamFormatName  = "PCM_SurroundLeft";
  names.trackFormatName   = "PCM_SurroundLeft";

  names.objectName = "5.1 test audio object";
  names.packFormatName = "urn:itu:bs:2051:0:pack:5.1_(0+5+0)";
  names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;

  REQUIRE(adm1->CreateObjects(names) == true);

  REQUIRE(names.objects.channelFormat != NULL);
  REQUIRE(names.objects.channelFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.streamFormat != NULL);
  REQUIRE(names.objects.streamFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.trackFormat != NULL);
  REQUIRE(names.objects.trackFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.packFormat != NULL);
  REQUIRE(names.objects.packFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions

  names.trackNumber       = 5;
  names.channelFormatName = "SurroundRight";
  names.streamFormatName  = "PCM_SurroundRight";
  names.trackFormatName   = "PCM_SurroundRight";

  names.objectName = "5.1 test audio object";
  names.packFormatName = "urn:itu:bs:2051:0:pack:5.1_(0+5+0)";
  names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;

  REQUIRE(adm1->CreateObjects(names) == true);

  REQUIRE(names.objects.channelFormat != NULL);
  REQUIRE(names.objects.channelFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.streamFormat != NULL);
  REQUIRE(names.objects.streamFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.trackFormat != NULL);
  REQUIRE(names.objects.trackFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions
  REQUIRE(names.objects.packFormat != NULL);
  REQUIRE(names.objects.packFormat->IsStandardDefinition() == true); // make sure referenced object is part of the standard definitions

  CHECK(adm1->GetErrorCount() == 0);
  reporterrors(adm1);

  std::string axml = adm1->GetAxml();

  XMLADMData *adm2 = XMLADMData::CreateADM();

  REQUIRE(adm2 != NULL);

  // make sure AXML generated correctly
  
  adm2->SetAxml(axml);
  
  CHECK(adm2->GetErrorCount() == 0);
  reporterrors(adm2);

  CHECK(adm1->GetAxml() == adm2->GetAxml());
  
  delete adm2;
  delete adm1;
}
