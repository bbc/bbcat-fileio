
#include <catch/catch.hpp>

#include "ADMRIFFFile.h"

USE_BBC_AUDIOTOOLBOX

TEST_CASE("comparison")
{
  const std::string testfiles[] =
  {
    "test1.xml",
    "test2.xml",
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
    
    CHECK(adm1 != NULL);
    CHECK(adm2 != NULL);

    if (adm1)
    {
      EnhancedFile f;
      
      REQUIRE(adm1->ReadXMLFromFile(testfile) == true);

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

          // check that axml's are identical
          CHECK(adm1->GetAxml() == adm2->GetAxml());

          // also check that files are identical
          std::string cmd;
          Printf(cmd, "diff %s %s", testfile.c_str(), output.c_str());
          CHECK(system(cmd.c_str()) == 0);
        }
      }
    }

    if (adm2) delete adm2;
    if (adm1) delete adm1;
  }
}
