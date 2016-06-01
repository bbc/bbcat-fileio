/* Auto-generated: DO NOT EDIT! */
#include <bbcat-base/LoadedVersions.h>
BBC_AUDIOTOOLBOX_START
// list of libraries this library is dependant on
extern bool bbcat_register_bbcat_adm();

// list of this library's component registration functions
extern void bbcat_register_TinyXMLADMData();

// registration function
bool bbcat_register_bbcat_fileio()
{
  static bool registered = false;
  // prevent registration functions being called more than once
  if (!registered)
  {
    registered = true;
    // register other libraries
	bbcat_register_bbcat_adm();

    // register this library's version number
    LoadedVersions::Get().Register("bbcat-fileio", "0.1.2.2-master");
    // register this library's components
	bbcat_register_TinyXMLADMData();

  }
  return registered;
}
// automatically call registration functions
volatile const bool bbcat_fileio_registered = bbcat_register_bbcat_fileio();
BBC_AUDIOTOOLBOX_END
