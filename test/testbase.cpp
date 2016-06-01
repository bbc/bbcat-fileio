#include <bbcat-base/misc.h>
#include <bbcat-base/SystemParameters.h>

#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>

BBC_AUDIOTOOLBOX_START
extern bool bbcat_register_bbcat_fileio();
BBC_AUDIOTOOLBOX_END

USE_BBC_AUDIOTOOLBOX

TEST_CASE("init")
{
  bbcat_register_bbcat_fileio();

  // modify share directory in SystemParameters to include local share
  std::string sharedir;
  SystemParameters::Get().Get(SystemParameters::sharedirkey, sharedir);
  SystemParameters::Get().Set(SystemParameters::sharedirkey, "../share;" + sharedir);
}
