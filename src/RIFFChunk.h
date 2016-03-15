
#ifndef __RIFF_CHUNK__
#define __RIFF_CHUNK__

#include <map>
#include <string>

#include <bbcat-base/misc.h>

#include "SoundFileAttributes.h"

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** An interface to allow overriding of chunk sizes in cases such as RF64
 *
 */
/*--------------------------------------------------------------------------------*/
class RIFFChunkSizeHandler
{
public:
  RIFFChunkSizeHandler() {}
  virtual ~RIFFChunkSizeHandler() {}

  virtual bool     SetChunkSize(uint32_t id, uint64_t length) = 0;
  virtual uint64_t GetChunkSize(uint32_t id, uint64_t original_length) const = 0;
};

/*--------------------------------------------------------------------------------*/
/** Class for handling RIFF file chunks (used in WAV/AIFF/AIFC files)
 *
 * This object provides basic chunk handling facilities which can be used for
 * unknown chunks whereas the majority of chunks should use derived versions of this
 *
 * Chunk ID's are stored as 32-bit ints in big endian format, therefore 'RIFF' would
 * be stored as 0x52494646
 *
 * The chunk ID governs what object gets created to handle the data, 'providers' are
 * registered (using RIFFChunk::RegisterProvider()) and must provide a function to
 * create a RIFFChunk derived object to handle the chunk
 *
 * Chunk data is not NECESSARILY read and stored within this object!  It is up to
 * the derived objects to decide whether the chunk data should automatically be
 * read
 *
 * The function ChunkNeedsReading() controls what happens to the chunk:
 *   if it returns true, the chunk data is read, byte swapped and processed
 *   if it returns false, the chunk data is skipped over
 * The chunk data can be read subsequently by calling ReadData() with an open file
 * handle to the file
 * 
 */
/*--------------------------------------------------------------------------------*/
class RIFFFile;
class RIFFChunk
{
public:
  virtual ~RIFFChunk();

  /*--------------------------------------------------------------------------------*/
  /** Return chunk ID as 32-bit, big endian, integer
   */
  /*--------------------------------------------------------------------------------*/
  uint32_t       GetID()     const {return id;}

  /*--------------------------------------------------------------------------------*/
  /** Return chunk name as ASCII string (held internally so there's no need to free the return)
   */
  /*--------------------------------------------------------------------------------*/
  const char    *GetName()   const {return name.c_str();}

  /*--------------------------------------------------------------------------------*/
  /** Return chunk data length
   */
  /*--------------------------------------------------------------------------------*/
  uint64_t       GetLength() const {return length;}

  // maximum chunk size without using ds64
  static const uint64_t RIFF_MaxSize;

  /*--------------------------------------------------------------------------------*/
  /** Tell this chunk that the file is a RIFF64 file
   *
   * @note the riff64 boolean is only set if RIFF64Capable() returns true
   */
  /*--------------------------------------------------------------------------------*/
  virtual void EnableRIFF64() {riff64 = RIFF64Capable();}

  /*--------------------------------------------------------------------------------*/
  /** Return chunk data length as stored on file (header plus length plus padding if necessary)
   */
  /*--------------------------------------------------------------------------------*/
  virtual uint64_t GetLengthOnFile() const {return WriteThisChunk() ? 8 + length + (length & align) : 0;}

  /*--------------------------------------------------------------------------------*/
  /** Return chunk data (or NULL if data has not yet been read)
   */
  /*--------------------------------------------------------------------------------*/
  const uint8_t *GetData() const {return data;}

  /*--------------------------------------------------------------------------------*/
  /** Return chunk data that can be written to
   */
  /*--------------------------------------------------------------------------------*/
  uint8_t *GetDataWritable() {return data;}

  /*--------------------------------------------------------------------------------*/
  /** Delete read data
   */
  /*--------------------------------------------------------------------------------*/
  virtual void DeleteData();

  /*--------------------------------------------------------------------------------*/
  /** By default, all chunks are written before samples (data chunk)
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool WriteChunkBeforeSamples() const {return true;}

  /*--------------------------------------------------------------------------------*/
  /** Return whether this chunk should be written (empty chunks need not be written)
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool WriteThisChunk() const {return (length || WriteEmptyChunk());}

  /*--------------------------------------------------------------------------------*/
  /** Write chunk
   *
   * @return true if chunk successfully written
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool WriteChunk(EnhancedFile *file);

  /*--------------------------------------------------------------------------------*/
  /** Supply chunk data for writing
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool CreateChunkData(const void *_data, uint64_t _length);

  /*--------------------------------------------------------------------------------*/
  /** Update chunk data for writing
   *
   * @note returns false is length is different!
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool UpdateChunkData(const void *_data, uint64_t _length);

  /*--------------------------------------------------------------------------------*/
  /** Create/update data to the specified size
   *
   * @note if a data block already exists it will be copied to the new block
   * @note this functions allows expansion of but *not* shrinking of the data block
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool CreateChunkData(uint64_t _length);

  /*--------------------------------------------------------------------------------*/
  /** Create data for writing to chunk
  *
  * @return true if data created successfully
  */
  /*--------------------------------------------------------------------------------*/
  virtual bool CreateWriteData() {return true;}

  /*--------------------------------------------------------------------------------*/
  /** Return whether ANY providers have been registered (to allow automatic registration)
   */
  /*--------------------------------------------------------------------------------*/
  static bool NoProvidersRegistered() {return (providermap.end() == providermap.begin());}

  /*--------------------------------------------------------------------------------*/
  /** Register a chunk handler
   *
   * @param id 32-bit chunk ID (big-endian format)
   * @param fn function to create RIFFChunk derived object
   * @param context optional userdata to be supplied to the above function
   *
   */
  /*--------------------------------------------------------------------------------*/
  static void RegisterProvider(uint32_t id, RIFFChunk *(*fn)(uint32_t id, void *context), void *context = NULL);

  /*--------------------------------------------------------------------------------*/
  /** Register a chunk handler
   *
   * @param name ASCII chunk name (optionally terminated)
   * @param fn function to create RIFFChunk derived object
   * @param context optional userdata to be supplied to the above function
   *
   */
  /*--------------------------------------------------------------------------------*/
  static void RegisterProvider(const char *name, RIFFChunk *(*fn)(uint32_t id, void *context), void *context = NULL);

  /*--------------------------------------------------------------------------------*/
  /** Return ASCII name representation of chunk ID
   */
  /*--------------------------------------------------------------------------------*/
  static std::string GetChunkName(uint32_t id);

  /*--------------------------------------------------------------------------------*/
  /** The primary chunk creation function when reading files
   *
   * @param file open file positioned at chunk ID point
   *
   * @return RIFFChunk object for the chunk
   *
   * @note at return, the new file position will be at the start of the next chunk
   */
  /*--------------------------------------------------------------------------------*/
  static RIFFChunk *Create(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler = NULL);

  /*--------------------------------------------------------------------------------*/
  /** The primary chunk creation function when writing files
   *
   * @param file open file
   * @param id RIFF ID
   * @param name RIFF name
   *
   * @return RIFFChunk object for the chunk
   *
   */
  /*--------------------------------------------------------------------------------*/
  static RIFFChunk *Create(uint32_t id);
  static RIFFChunk *Create(const char *name);

protected:
  /*--------------------------------------------------------------------------------*/
  /** Constructor - can only be called by static member function!
   */
  /*--------------------------------------------------------------------------------*/
  RIFFChunk(uint32_t chunk_id);

private:
  RIFFChunk(const RIFFChunk& obj) {(void)obj;}    // do not allow copies

protected:
  /*--------------------------------------------------------------------------------*/
  /** Initialise chunk for writing - called when empty chunk is first created
  */
  /*--------------------------------------------------------------------------------*/
  virtual bool InitialiseForWriting() {return true;}
  /*--------------------------------------------------------------------------------*/
  /** Read chunk data length and decide what to do
   *
   * @return true if chunk successfully read/handled
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool ReadChunk(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler);
  /*--------------------------------------------------------------------------------*/
  /** Read chunk data and byte swap it (derived object provided)
   *
   * @return true if chunk successfully read
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool ReadData(EnhancedFile *file);

  /*--------------------------------------------------------------------------------*/
  /** placeholder for byte swapping function provided by derived objects
   *
   * @param writing true if data is about to be written (i.e. byte swap is possibly away from native order)
   */
  /*--------------------------------------------------------------------------------*/
  virtual void ByteSwapData(bool writing = false) {UNUSED_PARAMETER(writing);}

  /*--------------------------------------------------------------------------------*/
  /** Return ID when writing chunk
   *
   * @note can be used to mark chunks as 'JUNK' if they are not required
   */
  /*--------------------------------------------------------------------------------*/
  virtual uint32_t GetWriteID() const {return id;}

  /*--------------------------------------------------------------------------------*/
  /** Return whether to write this chunk even if it is empty (usually false)
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool WriteEmptyChunk() const {return false;}

  /*--------------------------------------------------------------------------------*/
  /** Write chunk data
   *
   * @return true if chunk successfully written
   *
   * @note if data exists it will be written otherwise the chunk will be blanked 
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool WriteChunkData(EnhancedFile *file);

  typedef enum
  {
    ChunkHandling_SkipOverChunk,
    ChunkHandling_RemainInChunkData,
    ChunkHandling_ReadChunk,
  } ChunkHandling_t;

  /*--------------------------------------------------------------------------------*/
  /** Returns what to do with the chunk:
   *  SkipOverChunk: Skip over data
   *  RemainInChunkData: Remain at start of data (used for nested chunks)
   *  ReadChunk: Read and process chunk data
   */
  /*--------------------------------------------------------------------------------*/
  virtual ChunkHandling_t GetChunkHandling() const {return ChunkHandling_SkipOverChunk;}

  /*--------------------------------------------------------------------------------*/
  /** placeholder for chunk data processing function provided by derived objects
   *
   * @return false to fail chunk reading
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool ProcessChunkData() {return true;}

  /*--------------------------------------------------------------------------------*/
  /** Return whether to delete data after processing
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool DeleteDataAfterProcessing() const {return false;}

  /*--------------------------------------------------------------------------------*/
  /** Return whether this chunk changes its behaviour for RIFF64 files
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool RIFF64Capable() {return false;}
 
  /*--------------------------------------------------------------------------------*/
  /** Return whether it is necessary to byte swap data
   *
   * SwapBigEndian()    returns true if machine is little endian
   * SwapLittleEndian() returns true if machine is big endian
   *
   * For big-endian data,    use if (SwapBigEndian()) {byte swap data...}
   * For little-endian data, use if (SwapLittleEndian()) {byte swap data...}
   */
  /*--------------------------------------------------------------------------------*/
  static bool SwapBigEndian();
  static bool SwapLittleEndian();

protected:
  /*--------------------------------------------------------------------------------*/
  /** Structure to contain providers to create chunk objects from chunk ID's
   */
  /*--------------------------------------------------------------------------------*/
  typedef struct
  {
    RIFFChunk *(*fn)(uint32_t id, void *context);
    void      *context;
  } PROVIDER;

protected:
  uint32_t    id;             ///< chunk ID
  std::string name;           ///< chunk ID as string
  uint64_t    length;         ///< chunk data length
  uint64_t    extrabytes;     ///< additional bytes to be allocted (and cleared) for chunk data (used for terminators, etc)
  uint64_t    datapos;        ///< chunk data file position
  uint8_t     *data;          ///< chunk data (if read)
  uint8_t     align;          ///< file alignment: 0 for no alignment, 1 for even byte alignment
  bool        riff64;         ///< true if file is RIFF64

  static std::map<uint32_t,PROVIDER> providermap;
};

BBC_AUDIOTOOLBOX_END

#endif
