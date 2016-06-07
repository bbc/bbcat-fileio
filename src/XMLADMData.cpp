
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#define BBCDEBUG_LEVEL 1
#include <bbcat-base/EnhancedFile.h>
#include <bbcat-base/SystemParameters.h>
#include <bbcat-adm/ADMXMLGenerator.h>

#include "XMLADMData.h"
#include "RIFFChunk_Definitions.h"

// currently the chna chunk is specified as having either 32 or 2048 entries for tracks
// define CHNA_FIXED_UIDS_LOWER as 0 to disable this behaviour
#define CHNA_FIXED_UIDS_LOWER 0
#define CHNA_FIXED_UIDS_UPPER 2048

BBC_AUDIOTOOLBOX_START

const std::string XMLADMData::DefaultStandardDefinitionsFile = "standarddefinitions.xml";
bool  XMLADMData::defaultebuxmlmode = true;

XMLADMData::XMLADMData() : ADMData(),
                           ebuxmlmode(defaultebuxmlmode)
{
}

XMLADMData::XMLADMData(const XMLADMData& obj) : ADMData(obj),
                                                ebuxmlmode(obj.ebuxmlmode)
{
}

XMLADMData::~XMLADMData()
{
}

/*--------------------------------------------------------------------------------*/
/** Return provider list, creating as necessary
 */
/*--------------------------------------------------------------------------------*/
std::vector<XMLADMData::PROVIDER>& XMLADMData::GetProviderList()
{
  // create here so that this function can be called before this object's static data is constructed
  static std::vector<PROVIDER> _providerlist;
  return _providerlist;
}

/*--------------------------------------------------------------------------------*/
/** Load standard definitions file into ADM
 */
/*--------------------------------------------------------------------------------*/
bool XMLADMData::LoadStandardDefinitions(const std::string& filename)
{
  static const char *paths[] =
  {
    "{env:BBCATFILEIOSHAREDIR}",    // BBCATFILEIOSHAREDIR environment variable
    "{fileiosharedir}",             // system parameter 'fileiosharedir'
    "share",                        // local location
    "{sharedir}/bbcat-fileio",      // global location
  };
  std::string filename2 = filename;
  bool success = false;

  // try various sources for the filename if it is empty
  if (filename2.empty()) filename2 = SystemParameters::Get().Substitute("{env:BBCATSTANDARDDEFINITIONSFILE}");
  if (filename2.empty()) SystemParameters::Get().GetSubstituted("standarddefinitionsfile", filename2);
  // use default name if none above work
  if (filename2.empty()) filename2 = "standarddefinitions.xml";

  BBCDEBUG3(("Using '%s' as filename for standard definitions file", filename2.c_str()));
  
  // delete any existing objects
  Delete();

  if (!(filename2 = FindFile(filename2, paths, NUMBEROF(paths))).empty())
  {
    BBCDEBUG("Found standard definitions file '%s' as '%s'", filename.c_str(), filename2.c_str());

    if (ReadXMLFromFile(filename2.c_str()))
    {
      ADMOBJECTS_IT it;

      // set standard def flag on all loaded objects
      for (it = admobjects.begin(); it != admobjects.end(); ++it)
      {
        it->second->SetStandardDefinition();
      }

      success = true;
    }
    else LogError("Failed to read standard definitions file '%s'", filename2.c_str());
  }
  else BBCDEBUG1(("Standard definitions file '%s' doesn't exist", filename2.c_str()));

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Read ADM data from the chna RIFF chunk
 *
 * @param data ptr to chna chunk data
 * @param len length of chna chunk
 *
 * @return true if data read successfully
 */
/*--------------------------------------------------------------------------------*/
bool XMLADMData::SetChna(const uint8_t *data, uint64_t len)
{
  const CHNA_CHUNK& chna = *(const CHNA_CHUNK *)data;
  uint_t maxuids = (uint_t)((len - sizeof(CHNA_CHUNK)) / sizeof(chna.UIDs[0]));   // calculate maximum number of UIDs given chunk length
  std::string terminator;
  bool success = true;

  // create string with a single 0 byte in it to detect terminators
  terminator.push_back(0);

  if (maxuids < chna.UIDCount) LogError("Warning: chna specifies %u UIDs but chunk has only length for %u", (uint_t)chna.UIDCount, maxuids);

  uint16_t i;
  for (i = 0; (i < chna.UIDCount) && (i < maxuids); i++)
  {
    // only handle non-zero track numbers
    if (chna.UIDs[i].TrackNum)
    {
      ADMAudioTrack *track;
      std::string id;

      id.assign(chna.UIDs[i].UID, sizeof(chna.UIDs[i].UID));
      id = id.substr(0, id.find(terminator));

      // delete any existing ADMAudioTracks of the same ID
      if ((track = dynamic_cast<ADMAudioTrack *>(Create(ADMAudioTrack::Type, id, "", true))) != NULL)
      {
        XMLValue tvalue, pvalue;

        track->SetTrackNum(chna.UIDs[i].TrackNum - 1);

        tvalue.name = ADMAudioTrackFormat::Reference;
        tvalue.value.assign(chna.UIDs[i].TrackRef, sizeof(chna.UIDs[i].TrackRef));
        // trim any zero bytes off the end of the string
        tvalue.value = tvalue.value.substr(0, tvalue.value.find(terminator));
        track->AddValue(tvalue);

        pvalue.name = ADMAudioPackFormat::Reference;
        pvalue.value.assign(chna.UIDs[i].PackRef, sizeof(chna.UIDs[i].PackRef));
        // trim any zero bytes off the end of the string
        pvalue.value = pvalue.value.substr(0, pvalue.value.find(terminator));
        track->AddValue(pvalue);

        track->SetValues();

        BBCDEBUG2(("Track %u/%u: Index %u UID '%s' TrackFormatRef '%s' PackFormatRef '%s'",
                i, (uint_t)tracklist.size(),
                track->GetTrackNum() + 1,
                track->GetID().c_str(),
                tvalue.value.c_str(),
                pvalue.value.c_str()));
      }
      else LogError("Failed to create AudioTrack for UID %u", i);
    }
  }

  SortTracks();

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Read ADM data from the axml RIFF chunk
 *
 * @param data ptr to axml chunk data (MUST be terminated!)
 * @param finalise true to attempt to finalise ADM after loading
 *
 * @return true if data read successfully
 */
/*--------------------------------------------------------------------------------*/
bool XMLADMData::SetAxml(const char *data, bool finalise)
{
  bool success = false;

  BBCDEBUG3(("Read XML:\n%s", data));

  if (TranslateXML(data))
  {
    if (finalise) Finalise();

    success = true;
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Read ADM data from explicit XML
 *
 * @param data XML data stored as a string
 * @param finalise true to attempt to finalise ADM after loading
 *
 * @return true if data read successfully
 */
/*--------------------------------------------------------------------------------*/
bool XMLADMData::SetAxml(const std::string& data, bool finalise)
{
  bool success = false;

  BBCDEBUG3(("Read XML:\n%s", data.c_str()));

  if (TranslateXML(data.c_str()))
  {
    if (finalise) Finalise();

    success = true;
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Load CHNA data from file
 *
 * @param filename file containing chna in text form
 * @param finalise true to attempt to finalise ADM after loading
 *
 * @return true if data read successfully
 */
/*--------------------------------------------------------------------------------*/
bool XMLADMData::ReadChnaFromFile(const std::string& filename, bool finalise)
{
  EnhancedFile fp;
  bool success = false;

  if (fp.fopen(filename.c_str()))
  {
    char buffer[1024];
    int l;

    success = true;

    while (success && ((l = fp.readline(buffer, sizeof(buffer) - 1)) != EOF))
    {
      if (l > 0)
      {
        std::vector<std::string> words;

        SplitString(std::string(buffer), words);

        if (words.size() == 4)
        {
          uint_t tracknum;

          if (Evaluate(words[0], tracknum))
          {
            if (tracknum)
            {
              const std::string& trackuid    = words[1];
              const std::string& trackformat = words[2];
              const std::string& packformat  = words[3];
              ADMAudioTrack *track;
              std::string id;

              if ((track = dynamic_cast<ADMAudioTrack *>(Create(ADMAudioTrack::Type, trackuid, ""))) != NULL)
              {
                XMLValue tvalue, pvalue;

                track->SetTrackNum(tracknum - 1);

                tvalue.name = ADMAudioTrackFormat::Reference;
                tvalue.value = trackformat;
                track->AddValue(tvalue);

                pvalue.name = ADMAudioPackFormat::Reference;
                pvalue.value = packformat;
                track->AddValue(pvalue);

                track->SetValues();

                BBCDEBUG2(("Track %u: Index %u UID '%s' TrackFormatRef '%s' PackFormatRef '%s'",
                        (uint_t)tracklist.size(),
                        track->GetTrackNum() + 1,
                        track->GetID().c_str(),
                        tvalue.value.c_str(),
                        pvalue.value.c_str()));
              }
              else
              {
                LogError("Failed to create AudioTrack for UID %u", tracknum);
                success = false;
              }
            }
          }
          else
          {
            LogError("CHNA line '%s' word 1 ('%s') should be a track number", buffer, words[0].c_str());
            success = false;
          }
        }
        else
        {
          LogError("CHNA line '%s' requires 4 words", buffer);
          success = false;
        }
      }
    }

    fp.fclose();

    if (success && finalise) Finalise();
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Load ADM data from file
 *
 * @param filename file containing XML
 * @param finalise true to attempt to finalise ADM after loading
 *
 * @return true if data read successfully
 */
/*--------------------------------------------------------------------------------*/
bool XMLADMData::ReadXMLFromFile(const std::string& filename, bool finalise)
{
  EnhancedFile fp;
  bool success = false;

  if (fp.fopen(filename.c_str()))
  {
    fp.fseek(0, SEEK_END);
    off_t len = fp.ftell();
    fp.rewind();

    char *buffer;
    if ((buffer = new char[len + 1]) != NULL)
    {
      len = fp.fread(buffer, sizeof(char), len);
      buffer[len] = 0;

      success = SetAxml(buffer, finalise);

      delete[] buffer;
    }
    else LogError("Failed to allocate %s chars for file '%s'", StringFrom(len + 1).c_str(), filename.c_str());

    fp.fclose();
  }
  else LogError("Failed to open file '%s' for reading", filename.c_str());

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Read ADM data from the chna and axml RIFF chunks
 *
 * @param chna ptr to chna chunk data
 * @param chnalength length of chna data
 * @param axml ptr to axml chunk data (MUST be terminated like a string)
 *
 * @return true if data read successfully
 */
/*--------------------------------------------------------------------------------*/
bool XMLADMData::Set(const uint8_t *chna, uint64_t chnalength, const char *axml)
{
  return (SetChna(chna, chnalength) && SetAxml(axml));
}

/*--------------------------------------------------------------------------------*/
/** Create chna chunk data
 *
 * @param len reference to length variable to be updated with the size of the chunk
 *
 * @return ptr to chunk data
 */
/*--------------------------------------------------------------------------------*/
uint8_t *XMLADMData::GetChna(uint64_t& len) const
{
  CHNA_CHUNK *p = NULL;
  uint_t nuids = (uint_t)tracklist.size();

#if CHNA_FIXED_UIDS_LOWER > 0
  nuids = (nuids <= CHNA_FIXED_UIDS_LOWER) ? CHNA_FIXED_UIDS_LOWER : CHNA_FIXED_UIDS_UPPER;
#endif

  // calculate size of chna chunk
  len = sizeof(*p) + (uint64_t)nuids * (uint64_t)sizeof(p->UIDs[0]);
  if ((p = (CHNA_CHUNK *)calloc(1, len)) != NULL)
  {
    std::vector<bool> uniquetracks; // list of unique tracks
    uint_t i;

    // allocate maximum number of tracks
    uniquetracks.resize(tracklist.size());

    // populate structure
    p->TrackCount = 0;
    p->UIDCount   = 0;

    for (i = 0; (i < tracklist.size()) && (p->UIDCount < nuids); i++)
    {
      const ADMAudioTrack *track = tracklist[i];
      uint_t tr = track->GetTrackNum();

      // test to see if uniquetracks array needs expanding
      if (tr >= uniquetracks.size()) uniquetracks.resize(tr + 1);

      // if this is a new track (i.e. one not previously used),increment TrackCount
      if (!uniquetracks[tr])
      {
        // set flag
        uniquetracks[tr] = true;
        // increment TrackCount
        p->TrackCount++;
      }

      // set track number (1- based) and UID
      p->UIDs[i].TrackNum = tr + 1;
      strncpy(p->UIDs[i].UID, track->GetID().c_str(), sizeof(p->UIDs[i].UID));

      // set trackformat references
      const ADMAudioTrackFormat *trackref = NULL;
      if (track->GetTrackFormatRefs().size() && ((trackref = track->GetTrackFormatRefs()[0]) != NULL))
      {
        strncpy(p->UIDs[i].TrackRef, trackref->GetID().c_str(), sizeof(p->UIDs[i].TrackRef));
      }

      // set packformat references
      const ADMAudioPackFormat *packref = NULL;
      if (track->GetPackFormatRefs().size() && ((packref = track->GetPackFormatRefs()[0]) != NULL))
      {
        strncpy(p->UIDs[i].PackRef, packref->GetID().c_str(), sizeof(p->UIDs[i].PackRef));
      }

      p->UIDCount++;

      BBCDEBUG2(("Track %u/%u: Index %u UID '%s' TrackFormatRef '%s' PackFormatRef '%s'",
              i, (uint_t)tracklist.size(),
              track->GetTrackNum() + 1,
              track->GetID().c_str(),
              trackref ? trackref->GetID().c_str() : "<none>",
              packref  ? packref->GetID().c_str()  : "<none>"));
    }
  }

  return (uint8_t *)p;
}

/*--------------------------------------------------------------------------------*/
/** Create axml chunk data
 *
 * @param indent indent string to use within XML
 * @param eol end of line string to use within XML
 * @param ind_level initial indentation level
 *
 * @return string containing XML data for axml chunk
 */
/*--------------------------------------------------------------------------------*/
std::string XMLADMData::GetAxml(const std::string& indent, const std::string& eol, uint_t ind_level) const
{
  return ADMXMLGenerator::GetAxml(this, ebuxmlmode, indent, eol, ind_level);
}

/*--------------------------------------------------------------------------------*/
/** Create XML representation of ADM in a data buffer
 *
 * @param buf buffer to store XML in (or NULL to just get the length)
 * @param buflen maximum length of buf (excluding terminator)
 * @param indent indentation for each level of objects
 * @param eol end-of-line string
 * @param level initial indentation level
 *
 * @return total length of XML
 */
/*--------------------------------------------------------------------------------*/
uint64_t XMLADMData::GetAxmlBuffer(uint8_t *buf, uint64_t buflen, const std::string& indent, const std::string& eol, uint_t ind_level) const
{
  return ADMXMLGenerator::GetAxmlBuffer(this, buf, buflen, ebuxmlmode, indent, eol, ind_level);
}

/*--------------------------------------------------------------------------------*/
/** Create an ADM capable of decoding supplied XML as axml chunk
 */
/*--------------------------------------------------------------------------------*/
XMLADMData *XMLADMData::CreateADM(const std::string& standarddefinitionsfile)
{
  const std::vector<PROVIDER>& providerlist = GetProviderList();
  XMLADMData *data = NULL;
  uint_t i;

  for (i = 0; i < providerlist.size(); i++)
  {
    const PROVIDER& provider = providerlist[i];

    if ((data = (*provider.fn)(standarddefinitionsfile, provider.context)) != NULL) break;
  }

  return data;
}

/*--------------------------------------------------------------------------------*/
/** Register a provider for the above
 */
/*--------------------------------------------------------------------------------*/
void XMLADMData::RegisterProvider(CREATOR fn, void *context)
{
  std::vector<PROVIDER>& providerlist = GetProviderList();
  PROVIDER provider = {fn, context};

  providerlist.push_back(provider);
}

/*--------------------------------------------------------------------------------*/
/** Parse XML section
 *
 * @param type object type
 * @param userdata context data
 *
 * @return newly created object
 */
/*--------------------------------------------------------------------------------*/
ADMObject *XMLADMData::Parse(const std::string& type, void *userdata)
{
  ADMHEADER header;
  ADMObject *obj;

  ParseHeader(header, type, userdata);

  // delete any existing object of the same ID UNLESS it's an ADMAudioTrack
  if ((obj = Create(type, header.id, header.name, (type != ADMAudioTrack::Type))) != NULL)
  {
    ParseValues(obj, userdata);
    PostParse(obj, userdata);

    obj->SetValues();
  }

  return obj;
}

BBC_AUDIOTOOLBOX_END
