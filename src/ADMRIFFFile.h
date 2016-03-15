#ifndef __ADM_RIFF_FILE__
#define __ADM_RIFF_FILE__

#include <string>
#include <map>

#include <bbcat-base/SelfRegisteringParametricObject.h>
#include <bbcat-adm/AudioObjectCursor.h>

#include "RIFFFile.h"
#include "XMLADMData.h"

BBC_AUDIOTOOLBOX_START

#define TYPE_ADMBWF "admbwf"

/*--------------------------------------------------------------------------------*/
/** ADM BWF file support class
 *
 * This class uses an external interpretation mechanism to interpret the ADM data
 * contained within an BWF file to create an ADMData object which is then accessible
 * from this object
 *
 * The ADMData class does NOT provide any interpretation implementation but does provide
 * ADM data manipulation and chunk generation
 *
 * An example implementation of the interpretation is in TinyXMLADMData.h
 */
/*--------------------------------------------------------------------------------*/
class ADMRIFFFile : public RIFFFile
{
public:
  ADMRIFFFile();
  virtual ~ADMRIFFFile();
  
  /*--------------------------------------------------------------------------------*/
  /** Open a WAVE/RIFF file
   *
   * @param filename filename of file to open
   * @param standarddefinitionsfile filename of standard definitions XML file to use
   *
   * @return true if file opened and interpreted correctly (including any extra chunks if present)
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool Open(const char *filename) {return Open(filename, "");}
  virtual bool Open(const char *filename, const std::string& standarddefinitionsfile);

  /*--------------------------------------------------------------------------------*/
  /** Create empty ADM and populate basic track information
   *
   * @param standarddefinitionsfile filename of standard definitions XML file to use
   *
   * @return true if successful
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool CreateADM(const std::string& standarddefinitionsfile = "");

  /*--------------------------------------------------------------------------------*/
  /** Create ADM from text file
   *
   * @param filename text filename (see below for format)
   * @param standarddefinitionsfile filename of standard definitions XML file to use
   *
   * @return true if successful
   *
   * The file MUST be of the following format with each entry on its own line:
   * <ADM programme name>[:<ADM content name>]
   *
   * then for each track:
   * <track>:<trackname>:<objectname>
   *
   * Where <track> is 1..number of tracks available within ADM
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool CreateADMFromFile(const std::string& filename, const std::string& standarddefinitionsfile = "");

  /*--------------------------------------------------------------------------------*/
  /** Set parameters of channel during writing
   *
   * @param channel channel to change the position of
   * @param objparameters object parameters
   */
  /*--------------------------------------------------------------------------------*/
  virtual void SetObjectParameters(uint_t channel, const AudioObjectParameters& objparameters);

  /*--------------------------------------------------------------------------------*/
  /** Close file
   *
   * @param abortwrite true to abort the writing of file
   *
   * @note this may take some time because it copies sample data from a temporary file
   */
  /*--------------------------------------------------------------------------------*/
  virtual void Close(bool abortwrite = false);

  /*--------------------------------------------------------------------------------*/
  /** Create cursors and add all objects to each cursor
   *
   * @note this can be called prior to writing samples or setting positions but it
   * @note *will* be called by SetPositions() if not done so already
   */
  /*--------------------------------------------------------------------------------*/
  virtual void PrepareCursors();

  ADMData *GetADM() const {return adm;}

protected:
  /*--------------------------------------------------------------------------------*/
  /** Post processing function - actually performs the interpretation of the ADM once
   * it has been read
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool PostReadChunks();

  /*--------------------------------------------------------------------------------*/
  /** Optional stage to create extra chunks when writing WAV files
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool CreateExtraChunks();

  /*--------------------------------------------------------------------------------*/
  /** Overrideable called whenever sample position changes
   */
  /*--------------------------------------------------------------------------------*/
  virtual void UpdateSamplePosition();

protected:
  std::string admfile;
  XMLADMData  *adm;
  std::vector<ADMTrackCursor *> cursors;        // *only* used during writing an ADM file
};

BBC_AUDIOTOOLBOX_END

#endif
