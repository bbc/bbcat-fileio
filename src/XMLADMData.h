#ifndef __XML_ADM_DATA__
#define __XML_ADM_DATA__

#include <bbcat-adm/ADMData.h>

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** ADM data class
 *
 * This class forms the base class of ADM reading and writing
 *
 * It CANNOT, by itself decode the XML from axml chunks, that must be done by a derived class.
 *
 * It CAN, however, generate XML (using a very simple method) from a manually created ADM
 *
 */
/*--------------------------------------------------------------------------------*/
class XMLADMData : public ADMData
{
public:
  XMLADMData();
  XMLADMData(const XMLADMData& obj);
  virtual ~XMLADMData();

  /*--------------------------------------------------------------------------------*/
  /** Read ADM data from the chna and axml RIFF chunks
   *
   * @param chna ptr to chna chunk data 
   * @param chnalength length of chna data
   * @param axml ptr to axml chunk data (MUST be terminated like a string) 
   *
   * @return true if data read successfully
   *
   * @note this requires facilities from a derived class
   */
  /*--------------------------------------------------------------------------------*/
  bool Set(const uint8_t *chna, uint64_t chnalength, const char *axml);

  /*--------------------------------------------------------------------------------*/
  /** Read ADM data from the chna RIFF chunk
   *
   * @param data ptr to chna chunk data 
   * @param len length of chna chunk
   *
   * @return true if data read successfully
   */
  /*--------------------------------------------------------------------------------*/
  bool SetChna(const uint8_t *data, uint64_t len);

  /*--------------------------------------------------------------------------------*/
  /** Read ADM data from the axml RIFF chunk
   *
   * @param data ptr to axml chunk data (MUST be terminated!)
   *
   * @return true if data read successfully
   */
  /*--------------------------------------------------------------------------------*/
  bool SetAxml(const char *data);

  /*--------------------------------------------------------------------------------*/
  /** Read ADM data from explicit XML
   *
   * @param data XML data stored as a string
   *
   * @return true if data read successfully
   */
  /*--------------------------------------------------------------------------------*/
  bool SetAxml(const std::string& data);

  /*--------------------------------------------------------------------------------*/
  /** Load CHNA data from file
   *
   * @param filename file containing chna in text form
   *
   * @return true if data read successfully
   */
  /*--------------------------------------------------------------------------------*/
  bool ReadChnaFromFile(const std::string& filename);

  /*--------------------------------------------------------------------------------*/
  /** Load ADM data from file
   *
   * @param filename file containing XML
   *
   * @return true if data read successfully
   */
  /*--------------------------------------------------------------------------------*/
  bool ReadXMLFromFile(const std::string& filename);

  /*--------------------------------------------------------------------------------*/
  /** Create chna chunk data
   *
   * @param len reference to length variable to be updated with the size of the chunk
   *
   * @return ptr to chunk data
   */
  /*--------------------------------------------------------------------------------*/
  uint8_t *GetChna(uint64_t& len) const;

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
  std::string GetAxml(const std::string& indent = "\t", const std::string& eol = "\n", uint_t ind_level = 0) const;

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
  uint64_t GetAxmlBuffer(uint8_t *buf, uint64_t buflen, const std::string& indent = "\t", const std::string& eol = "\n", uint_t ind_level = 0) const;

  /*--------------------------------------------------------------------------------*/
  /** Set default EBU XML output mode
   */
  /*--------------------------------------------------------------------------------*/
  static void SetDefaultEBUXMLMode(bool enable = true) {defaultebuxmlmode = enable;}

  /*--------------------------------------------------------------------------------*/
  /** Return whether output mode is EBU or ITU XML
   */
  /*--------------------------------------------------------------------------------*/
  bool EBUXMLMode() const {return ebuxmlmode;}

  /*--------------------------------------------------------------------------------*/
  /** Set whether output mode is EBU or ITU XML
   */
  /*--------------------------------------------------------------------------------*/
  void SetEBUXMLMode(bool enable = true) {ebuxmlmode = enable;}

  static const std::string DefaultStandardDefinitionsFile;
  static XMLADMData *CreateADM(const std::string& standarddefinitionsfile = "");
  
protected:
  /*--------------------------------------------------------------------------------*/
  /** Load standard definitions file into ADM
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool LoadStandardDefinitions(const std::string& filename);

  typedef struct
  {
    std::string type;
    std::string id;
    std::string name;
  } ADMHEADER;

  /*--------------------------------------------------------------------------------*/
  /** Decode XML string as ADM
   *
   * @param data ptr to string containing ADM XML (MUST be terminated)
   *
   * @return true if XML decoded correctly
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool TranslateXML(const char *data) = 0;

  virtual ADMObject *Parse(const std::string& type, void *userdata);

  /*--------------------------------------------------------------------------------*/
  /** Parse XML header
   */
  /*--------------------------------------------------------------------------------*/
  virtual void ParseHeader(ADMHEADER& header, const std::string& type, void *userdata) = 0;

  /*--------------------------------------------------------------------------------*/
  /** Parse attributes and subnodes as values
   *
   * @param obj object to read values from
   * @param userdata user suppled data
   */
  /*--------------------------------------------------------------------------------*/
  virtual void ParseValues(ADMObject *obj, void *userdata) = 0;

  /*--------------------------------------------------------------------------------*/
  /** Optional post parse handler
   */
  /*--------------------------------------------------------------------------------*/
  virtual void PostParse(ADMObject *obj, void *userdata)
  {
    UNUSED_PARAMETER(obj);
    UNUSED_PARAMETER(userdata);
  }

  typedef XMLADMData *(*CREATOR)(const std::string& standarddefinitionsfile, void *context);
  typedef struct
  {
    CREATOR fn;
    void    *context;
  } PROVIDER;

  /*--------------------------------------------------------------------------------*/
  /** Return provider list, creating as necessary
   */
  /*--------------------------------------------------------------------------------*/
  static std::vector<PROVIDER>& GetProviderList();

  static void RegisterProvider(CREATOR fn, void *context = NULL);

protected:
  bool        ebuxmlmode;
  static bool defaultebuxmlmode;
};

BBC_AUDIOTOOLBOX_END

#endif
