#include <bbcat-base/misc.h>
#include <bbcat-base/SystemParameters.h>
#include "register.h"

#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>

USE_BBC_AUDIOTOOLBOX

TEST_CASE("init")
{
  bbcat_register_bbcat_fileio();

  // modify share directory in SystemParameters to include local share
  std::string sharedir;
  SystemParameters::Get().Get(SystemParameters::sharedirkey, sharedir);
  SystemParameters::Get().Set(SystemParameters::sharedirkey, "../share;" + sharedir);

  // add local share folder for fileio data location
  SystemParameters::Get().Set("fileiosharedir", "../share");
}
