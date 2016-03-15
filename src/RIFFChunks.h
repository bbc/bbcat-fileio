
#ifndef __RIFF_CHUNKS__
#define __RIFF_CHUNKS__

#include <string>

#include "RIFFChunk.h"
#include "SoundFileAttributes.h"

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** Chunk specific handling
 */
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
/** RIFF/RIFF64 chunk - the first chunk of any WAVE file
 *
 * Don't read the data, no specific handling
 */
/*--------------------------------------------------------------------------------*/
class RIFFRIFFChunk : public RIFFChunk
{
public:
  RIFFRIFFChunk(uint32_t chunk_id) : RIFFChunk(chunk_id) {}
  virtual ~RIFFRIFFChunk() {}

  // set chunk to RF64
  virtual void EnableRIFF64();

  // provider function register for this object
  static void Register();

protected:
  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFRIFFChunk(id);
  }

protected:
  // read limited data of chunk
  virtual ChunkHandling_t GetChunkHandling() const {return ChunkHandling_RemainInChunkData;}
  // just return true - no data to write
  virtual bool WriteChunkData(EnhancedFile *file);
  // return that this chunk changes its behaviour for RIFF64 files
  virtual bool RIFF64Capable() {return true;}
  // must write chunk
  virtual bool WriteEmptyChunk() const {return true;}
};

/*--------------------------------------------------------------------------------*/
/** WAVE chunk - specifies that the file contains WAV data
 *
 * This isn't actually a proper chunk - there is no length, just the ID, hence
 * a specific ReadChunk() function
 */
/*--------------------------------------------------------------------------------*/
class RIFFWAVEChunk : public RIFFChunk
{
public:
  RIFFWAVEChunk(uint32_t chunk_id) : RIFFChunk(chunk_id) {}
  virtual ~RIFFWAVEChunk() {}

  // WAVE chunk ONLY takes up 4 bytes
  virtual uint64_t GetLengthOnFile() const {return 4;}

  // special write chunk function (no length or data)
  virtual bool WriteChunk(EnhancedFile *file);

  // provider function register for this object
  static void Register();

protected:
  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFWAVEChunk(id);
  }

protected:
  // dummy read function
  virtual bool ReadChunk(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler);
  // must write chunk
  virtual bool WriteEmptyChunk() const {return true;}
};

/*--------------------------------------------------------------------------------*/
/** ds64 chunk - specifies chunk sizes for RIFF64 files
 *
 * The chunk data is read, byte swapped and then processed
 *
 */
/*--------------------------------------------------------------------------------*/
class RIFFds64Chunk : public RIFFChunk, public RIFFChunkSizeHandler
{
public:
  RIFFds64Chunk(uint32_t chunk_id) : RIFFChunk(chunk_id),
                                     RIFFChunkSizeHandler(),
                                     tablecount(0) {}
  virtual ~RIFFds64Chunk() {}

  // create write data
  virtual bool CreateWriteData();

  uint64_t GetRIFFSize() const;
  uint64_t GetdataSize() const;
  uint64_t GetSampleCount() const;
  uint_t   GetTableCount() const;
  uint64_t GetTableEntrySize(uint_t entry, char *id = NULL) const;
  uint64_t GetTableEntrySize(uint_t entry, uint32_t& id) const;

  void     SetRIFFSize(uint64_t size);
  void     SetdataSize(uint64_t size);
  void     SetSampleCount(uint64_t count);
  void     SetTableCount(uint32_t count);

  virtual bool     SetChunkSize(uint32_t id, uint64_t length);
  virtual uint64_t GetChunkSize(uint32_t id, uint64_t original_length) const;
  
  // provider function register for this object
  static void Register();

protected:
  /*--------------------------------------------------------------------------------*/
  /** Return ID when writing chunk
   *
   * @note can be used to mark chunks as 'JUNK' if they are not required
   */
  /*--------------------------------------------------------------------------------*/
  virtual uint32_t GetWriteID() const;

  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFds64Chunk(id);
  }

  static uint64_t Convert32bitSizes(uint32_t low, uint32_t high) {return ((uint64_t)low + ((uint64_t)high << 32));}

protected:
  // data will need byte swapping on certain machines
  virtual void ByteSwapData(bool writing);
  // always read and process his kind of chunk
  virtual ChunkHandling_t GetChunkHandling() const {return ChunkHandling_ReadChunk;}
  // return that this chunk changes its behaviour for RIFF64 files
  virtual bool RIFF64Capable() {return true;}

protected:
  uint32_t tablecount;
};

/*--------------------------------------------------------------------------------*/
/** fmt chunk - specifies the format of the WAVE data
 *
 * The chunk data is read, byte swapped and then processed
 *
 * Note this object is also derived from the SoundFormat class to allow generic
 * format handling facilities
 */
/*--------------------------------------------------------------------------------*/
class RIFFfmtChunk : public RIFFChunk, public SoundFormat
{
public:
  RIFFfmtChunk(uint32_t chunk_id) : RIFFChunk(chunk_id),
                                    SoundFormat() {}
  virtual ~RIFFfmtChunk() {}

  // create write data
  virtual bool CreateWriteData();

  // provider function register for this object
  static void Register();

protected:
  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFfmtChunk(id);
  }

protected:
  // data will need byte swapping on certain machines
  virtual void ByteSwapData(bool writing);
  // always read and process his kind of chunk
  virtual ChunkHandling_t GetChunkHandling() const {return ChunkHandling_ReadChunk;}
  // chunk processing
  virtual bool ProcessChunkData();
};

/*--------------------------------------------------------------------------------*/
/** bext chunk - specifies the Broadcast Extension data
 *
 * The chunk data is read, byte swapped but not processed
 *
 */
/*--------------------------------------------------------------------------------*/
class RIFFbextChunk : public RIFFChunk
{
public:
  RIFFbextChunk(uint32_t chunk_id) : RIFFChunk(chunk_id) {}
  virtual ~RIFFbextChunk() {}

  std::string   GetDescription() const;
  std::string   GetOriginator() const;
  std::string   GetOriginatorReference() const;
  std::string   GetOriginationDate() const;
  std::string   GetOriginationTime() const;
  uint64_t      GetTimeReference() const;
  uint_t        GetVersion() const;
  const uint8_t *GetUMID() const;
  uint_t        GetLoudnessValue() const;
  uint_t        GetLoudnessRange() const;
  uint_t        GetMaxTruePeakLevel() const;
  uint_t        GetMaxMomentaryLoudness() const;
  uint_t        GetMaxShortTermLoudness() const;
  std::string   GetCodingHistory() const;

  virtual void  SetDescription(const char *str);
  virtual void  SetOriginator(const char *str);
  virtual void  SetOriginatorReference(const char *str);
  virtual void  SetOriginationDate(const char *str);
  virtual void  SetOriginationTime(const char *str);
  virtual void  SetTimeReference(uint64_t value);
  virtual void  SetVersion(uint_t value);
  virtual void  SetUMID(const uint8_t umid[64]);
  virtual void  SetLoudnessValue(uint_t value);
  virtual void  SetLoudnessRange(uint_t value);
  virtual void  SetMaxTruePeakLevel(uint_t value);
  virtual void  SetMaxMomentaryLoudness(uint_t value);
  virtual void  SetMaxShortTermLoudness(uint_t value);
  virtual void  SetCodingHistory(const char *str);

  // create write data
  virtual bool CreateWriteData();

  // provider function register for this object
  static void Register();

protected:
  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFbextChunk(id);
  }

  /*--------------------------------------------------------------------------------*/
  /** Assign a length-limited, possibly unterminated, string to a std::string
   */
  /*--------------------------------------------------------------------------------*/
  static void GetUnterminatedString(std::string& res, const char *str, size_t maxlen);

protected:
  // data will need byte swapping on certain machines
  virtual void ByteSwapData(bool writing);
  // data should be read
  virtual ChunkHandling_t GetChunkHandling() const {return ChunkHandling_ReadChunk;}
};

/*--------------------------------------------------------------------------------*/
/** chna chunk - part of the ADM (EBU Tech 3364)
 *
 * The chunk data is read, byte swapped but not processed (it is handled by the parent)
 *
 */
/*--------------------------------------------------------------------------------*/
class RIFFchnaChunk : public RIFFChunk
{
public:
  RIFFchnaChunk(uint32_t chunk_id) : RIFFChunk(chunk_id) {}
  virtual ~RIFFchnaChunk() {}

  // this chunk is written *after* data chunk
  virtual bool WriteChunkBeforeSamples() const {return false;}

  // provider function register for this object
  static void Register();

protected:
  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFchnaChunk(id);
  }

protected:
  // byte swapping required
  virtual void ByteSwapData(bool writing);
  // data should be read
  virtual ChunkHandling_t GetChunkHandling() const {return ChunkHandling_ReadChunk;}
};

/*--------------------------------------------------------------------------------*/
/** axml chunk - part of the ADM (EBU Tech 3364)
 *
 * The chunk data is read, not byte swapped (it is pure XML) and not processed (it is handled by the parent)
 *
 */
/*--------------------------------------------------------------------------------*/
class RIFFaxmlChunk : public RIFFChunk
{
public:
  RIFFaxmlChunk(uint32_t chunk_id);
  virtual ~RIFFaxmlChunk() {}

  // this chunk is written *after* data chunk
  virtual bool WriteChunkBeforeSamples() const {return false;}

  // provider function register for this object
  static void Register();

protected:
  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFaxmlChunk(id);
  }

protected:
  // data should be read
  virtual ChunkHandling_t GetChunkHandling() const {return ChunkHandling_ReadChunk;}
};

/*--------------------------------------------------------------------------------*/
/** data chunk - WAVE data
 *
 * The data is not read (it could be too big to fit in memory)
 *
 * Note that the object is also derived from SoundFileSamples which provides sample
 * reading and conversion facilities
 *
 */
/*--------------------------------------------------------------------------------*/
class RIFFdataChunk : public RIFFChunk, public SoundFileSamples
{
public:
  RIFFdataChunk(uint32_t chunk_id) : RIFFChunk(chunk_id),
                                     SoundFileSamples() {}
  virtual ~RIFFdataChunk();

  // set up data length before data is written
  virtual bool CreateWriteData();

  // provider function register for this object
  static void Register();

protected:
  // provider function for this object
  static RIFFChunk *Create(uint32_t id, void *context)
  {
    (void)context;
    return new RIFFdataChunk(id);
  }

protected:
  // perform additional initialisation after chunk read
  virtual bool ReadChunk(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler);
  // copy sample data from temporary file
  virtual bool WriteChunkData(EnhancedFile *file);
  // return that this chunk changes its behaviour for RIFF64 files
  virtual bool RIFF64Capable() {return true;}
  // must write chunk
  virtual bool WriteEmptyChunk() const {return true;}
};

/*--------------------------------------------------------------------------------*/
/** User RIFF chunk, can be any id and data
 */
/*--------------------------------------------------------------------------------*/
class UserRIFFChunk: public RIFFChunk
{
public:
  UserRIFFChunk(uint32_t chunk_id, const void *_data, uint64_t _length, bool _beforesamples = false);
  virtual ~UserRIFFChunk() {}

  // return where this chunk should be written *before* the data chunk
  virtual bool WriteChunkBeforeSamples() const {return beforesamples;}

protected:
  bool beforesamples;
};
  
/*--------------------------------------------------------------------------------*/
/** Register all providers from this file
 */
/*--------------------------------------------------------------------------------*/
extern void RegisterRIFFChunkProviders();

BBC_AUDIOTOOLBOX_END

#endif
