
#define BOOST_TEST_MODULE admxmltest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "../src/ADMRIFFFile.h"

USE_BBC_AUDIOTOOLBOX

BOOST_AUTO_TEST_CASE( comparison )
{
  const std::string testfiles[] =
  {
    "test1.xml",
    "test2.xml",
  };
  uint_t i;

  for (i = 0; i < NUMBEROF(testfiles); i++)
  {
    XMLADMData *adm1 = XMLADMData::CreateADM();
    XMLADMData *adm2 = XMLADMData::CreateADM();
    const std::string& testfile = testfiles[i];
    
    BOOST_CHECK(adm1);
    BOOST_CHECK(adm2);

    if (adm1)
    {
      EnhancedFile f;
    
      BOOST_CHECK(adm1->ReadXMLFromFile(testfile));

      BOOST_CHECK(f.fopen("res.xml", "w"));
      if (f.isopen())
      {
        f.fprintf("%s", adm1->GetAxml().c_str());
        f.fclose();

        if (adm2)
        {
          BOOST_CHECK(adm2->ReadXMLFromFile("res.xml"));

          // check that axml's are identical
          BOOST_CHECK(adm1->GetAxml() == adm2->GetAxml());

          // also check that files are identical
          std::string cmd;
          Printf(cmd, "diff %s res.xml", testfile.c_str());
          BOOST_CHECK(system(cmd.c_str()) == 0);
        }
      }
    }

    if (adm2) delete adm2;
    if (adm1) delete adm1;
  }
}
