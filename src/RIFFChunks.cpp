
#include <string.h>
#include <errno.h>

#define BBCDEBUG_LEVEL 1
#include <bbcat-base/ByteSwap.h>

#include "RIFFChunks.h"
#include "RIFFChunk_Definitions.h"
#include "RIFFFile.h"

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** RIFF chunk - the first chunk of any WAVE file
 *
 * Don't read the data, no specific handling
 */
/*--------------------------------------------------------------------------------*/

// set chunk to RF64
void RIFFRIFFChunk::EnableRIFF64()
{
  RIFFChunk::EnableRIFF64();

  id   = RF64_ID;
  name = "RF64";
}

// just return true - no data to write
bool RIFFRIFFChunk::WriteChunkData(EnhancedFile *file)
{
  UNUSED_PARAMETER(file);
  return true;
}

void RIFFRIFFChunk::Register()
{
  RIFFChunk::RegisterProvider("RIFF", &Create);
  RIFFChunk::RegisterProvider("RF64", &Create);
}

/*----------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
/** WAVE chunk - specifies that the file contains WAV data
 *
 * This isn't actually a proper chunk - there is no length, just the ID, hence
 * a specific ReadChunk() function
 */
/*--------------------------------------------------------------------------------*/
bool RIFFWAVEChunk::ReadChunk(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler)
{
  UNUSED_PARAMETER(file);
  UNUSED_PARAMETER(sizehandler);

  // there is no data after WAVE to read
  return true;
}

// special write chunk function (no length or data)
bool RIFFWAVEChunk::WriteChunk(EnhancedFile *file)
{
  uint32_t data[] = {id};
  bool success = false;

  // treat ID  as big-endian
  ByteSwap(data[0], SWAP_FOR_BE);

  if (file->fwrite(data, NUMBEROF(data), sizeof(data[0])) > 0)
  {
    datapos = file->ftell();
    success = true;
  }

  return success;
}

void RIFFWAVEChunk::Register()
{
  RIFFChunk::RegisterProvider("WAVE", &Create);
}

/*----------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
/** ds64 chunk - specifies chunk sizes for RIFF64 files
 *
 * The chunk data is read, byte swapped and then processed
 *
 */
/*--------------------------------------------------------------------------------*/
void RIFFds64Chunk::Register()
{
  RIFFChunk::RegisterProvider("ds64", &Create);
}

void RIFFds64Chunk::ByteSwapData(bool writing)
{
  if (SwapLittleEndian() && data && (length >= sizeof(ds64_CHUNK)))
  {
    ds64_CHUNK& chunk = *(ds64_CHUNK *)data;
    uint32_t i;

    if (writing)
    {
      // when writing, byte-swap these BEFORE chunk.TableEntryCount gets byte-swapped
      for (i = 0; i < chunk.TableEntryCount; i++)
      {
        BYTESWAP_VAR(chunk.Table[i].ChunkSizeLow);
        BYTESWAP_VAR(chunk.Table[i].ChunkSizeHigh);
      }
    }

    BYTESWAP_VAR(chunk.RIFFSizeLow);
    BYTESWAP_VAR(chunk.RIFFSizeHigh);
    BYTESWAP_VAR(chunk.dataSizeLow);
    BYTESWAP_VAR(chunk.dataSizeHigh);
    BYTESWAP_VAR(chunk.SampleCountLow);
    BYTESWAP_VAR(chunk.SampleCountHigh);
    BYTESWAP_VAR(chunk.TableEntryCount);

    if (!writing)
    {
      // when reading, byte-swap these AFTER chunk.TableEntryCount gets byte-swapped
      for (i = 0; i < chunk.TableEntryCount; i++)
      {
        BYTESWAP_VAR(chunk.Table[i].ChunkSizeLow);
        BYTESWAP_VAR(chunk.Table[i].ChunkSizeHigh);
      }
    }
  }
}

/*--------------------------------------------------------------------------------*/
/** Return ID when writing chunk
 *
 * @note can be used to mark chunks as 'JUNK' if they are not required
 */
/*--------------------------------------------------------------------------------*/
uint32_t RIFFds64Chunk::GetWriteID() const
{
  return riff64 ? id : JUNK_ID;
}

// create write data
bool RIFFds64Chunk::CreateWriteData()
{
  bool success = (length && data);

  if (!success)
  {
    length = sizeof(ds64_CHUNK) + tablecount * sizeof(CHUNKSIZE64);
    if ((data = new uint8_t[length]) != NULL)
    {
      memset(data, 0, length);

      // set table count
      ((ds64_CHUNK *)data)->TableEntryCount = tablecount;

      success = true;
    }
  }

  return success;
}

uint64_t RIFFds64Chunk::GetRIFFSize() const
{
  const ds64_CHUNK *ds64;
  uint64_t size = 0;

  if ((ds64 = (const ds64_CHUNK *)GetData()) != NULL) size = Convert32bitSizes(ds64->RIFFSizeLow, ds64->RIFFSizeHigh);

  return size;
}

uint64_t RIFFds64Chunk::GetdataSize() const
{
  const ds64_CHUNK *ds64;
  uint64_t size = 0;

  if ((ds64 = (const ds64_CHUNK *)GetData()) != NULL) size = Convert32bitSizes(ds64->dataSizeLow, ds64->dataSizeHigh);

  return size;
}

uint64_t RIFFds64Chunk::GetSampleCount() const
{
  const ds64_CHUNK *ds64;
  uint64_t count = 0;

  if ((ds64 = (const ds64_CHUNK *)GetData()) != NULL) count = Convert32bitSizes(ds64->SampleCountLow, ds64->SampleCountHigh);

  return count;
}

uint_t RIFFds64Chunk::GetTableCount() const
{
  const ds64_CHUNK *ds64;
  uint_t count = 0;

  if ((ds64 = (const ds64_CHUNK *)GetData()) != NULL) count = ds64->TableEntryCount;

  return count;
}

uint64_t RIFFds64Chunk::GetTableEntrySize(uint_t entry, char *id) const
{
  const ds64_CHUNK *ds64;
  uint64_t size = 0;

  if (((ds64 = (const ds64_CHUNK *)GetData()) != NULL) && (entry < ds64->TableEntryCount))
  {
    size = Convert32bitSizes(ds64->Table[entry].ChunkSizeLow, ds64->Table[entry].ChunkSizeHigh);
    if (id) memcpy(id, ds64->Table[entry].ChunkId, sizeof(ds64->Table[entry].ChunkId));
  }

  return size;
}

uint64_t RIFFds64Chunk::GetTableEntrySize(uint_t entry, uint32_t& id) const
{
  const ds64_CHUNK *ds64;
  uint64_t size = 0;

  if (((ds64 = (const ds64_CHUNK *)GetData()) != NULL) && (entry < ds64->TableEntryCount))
  {
    size = Convert32bitSizes(ds64->Table[entry].ChunkSizeLow, ds64->Table[entry].ChunkSizeHigh);
    id   = IFFID(ds64->Table[entry].ChunkId);
  }

  return size;
}

void RIFFds64Chunk::SetRIFFSize(uint64_t size)
{
  ds64_CHUNK *ds64;

  if ((ds64 = (ds64_CHUNK *)GetData()) != NULL) {
    ds64->RIFFSizeLow  = (uint32_t)size;
    ds64->RIFFSizeHigh = (uint32_t)(size >> 32);
  }
}

void RIFFds64Chunk::SetdataSize(uint64_t size)
{
  ds64_CHUNK *ds64;

  if ((ds64 = (ds64_CHUNK *)GetData()) != NULL) {
    ds64->dataSizeLow  = (uint32_t)size;
    ds64->dataSizeHigh = (uint32_t)(size >> 32);
  }
}

void RIFFds64Chunk::SetSampleCount(uint64_t count)
{
  ds64_CHUNK *ds64;

  if ((ds64 = (ds64_CHUNK *)GetData()) != NULL) {
    ds64->SampleCountLow  = (uint32_t)count;
    ds64->SampleCountHigh = (uint32_t)(count >> 32);
  }
}

void RIFFds64Chunk::SetTableCount(uint32_t count)
{
  if (data && (count != GetTableCount()))
  {
    // if data has been allocaed and table count changed, need to re-allocate
    uint64_t newlength = sizeof(ds64_CHUNK) + count * sizeof(CHUNKSIZE64);
    uint8_t  *newdata;

    // reallocate, copy and replace
    if ((newdata = new uint8_t[newlength]) != NULL)
    {
      // clear data
      memset(newdata, 0, newlength);

      // copy over old data
      memcpy(newdata, data, std::min(length, newlength));
      delete[] data;

      data   = newdata;
      length = newlength;

      // set the number of entries in the table to allow for post-sample writing updates
      ds64_CHUNK *ds64;
      if ((ds64 = (ds64_CHUNK *)GetData()) != NULL) {
        ds64->TableEntryCount = count;
      }
    }
  }
  
  // update internal variable in case data hasn't been allocated yet
  tablecount = count;
}

bool RIFFds64Chunk::SetChunkSize(uint32_t id, uint64_t length)
{
  ds64_CHUNK *ds64;
  bool success = false;
  
  // WARNING: a switch statement cannot be used here because RIFF_ID, etc, uses an array (the name)!
  if (id == RIFF_ID)
  {
    SetRIFFSize(length);
    success = true;
  }
  else if (id == data_ID)
  {
    SetdataSize(length);
    success = true;
  }
  else if ((ds64 = (ds64_CHUNK *)GetData()) != NULL) {
    uint_t i, n = ds64->TableEntryCount;
    uint_t firstempty = ~0;

    // look for chunk in table
    for (i = 0; i < n; i++)
    {
      uint32_t tid = IFFID(ds64->Table[i].ChunkId);

      if (tid == id)
      {
        // found ID -> update chunk size
        ds64->Table[i].ChunkSizeLow  = (uint32_t)length;
        ds64->Table[i].ChunkSizeHigh = (uint32_t)(length >> 32);
        success = true;
        break;
      }
      else if (!tid && (i < firstempty))
      {
        // save first unused slot
        firstempty = i;
      }
    }

    // existing slot not found -> test for empty slot
    if ((i == n) && ((i = firstempty) < n))
    {
      // use free slot -> set ID and length
      ds64->Table[i].ChunkId[0]    = (char)(id >> 24); 
      ds64->Table[i].ChunkId[1]    = (char)(id >> 16); 
      ds64->Table[i].ChunkId[2]    = (char)(id >> 8); 
      ds64->Table[i].ChunkId[3]    = (char)id;
      ds64->Table[i].ChunkSizeLow  = (uint32_t)length;
      ds64->Table[i].ChunkSizeHigh = (uint32_t)(length >> 32);
      success = true;
    }
  }

  return success;
}

uint64_t RIFFds64Chunk::GetChunkSize(uint32_t id, uint64_t original_length) const
{
  uint64_t length = original_length;

  // only override if the original value is 0xffffffff
  if (length == RIFFChunk::RIFF_MaxSize)
  {
    // WARNING: a switch statement cannot be used here because RIFF_ID, etc, uses an array (the name)!
    if      (id == RIFF_ID) length = GetRIFFSize();
    else if (id == data_ID) length = GetdataSize();
    else if (GetTableCount()) {
      uint32_t testid = 0;
      uint_t   i, n = GetTableCount();

      // look for chunk in table
      for (i = 0; i < n; i++)
      {
        uint64_t newlength = GetTableEntrySize(i, testid);

        if (testid == id)
        {
          length = newlength;
          break;
        }
      }
    }

    if (length != original_length) BBCDEBUG2(("Updated chunk size of 0x%08lx to %s bytes", (ulong_t)id, StringFrom(length).c_str()));
  }

  return length;
}

/*----------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
/** fmt chunk - specifies the format of the WAVE data
 *
 * The chunk data is read, byte swapped and then processed
 *
 * Note this object is also derived from the SoundFormat class to allow generic
 * format handling facilities
 */
/*--------------------------------------------------------------------------------*/
void RIFFfmtChunk::Register()
{
  RIFFChunk::RegisterProvider("fmt ", &Create);
}

void RIFFfmtChunk::ByteSwapData(bool writing)
{
  UNUSED_PARAMETER(writing);

  if (SwapLittleEndian() && data && (length >= sizeof(WAVEFORMAT_CHUNK)))
  {
    WAVEFORMAT_CHUNK& chunk = *(WAVEFORMAT_CHUNK *)data;
  
    BYTESWAP_VAR(chunk.Format);
    BYTESWAP_VAR(chunk.Channels);
    BYTESWAP_VAR(chunk.SampleRate);
    BYTESWAP_VAR(chunk.BytesPerSecond);
    BYTESWAP_VAR(chunk.BlockAlign);
    BYTESWAP_VAR(chunk.BitsPerSample);

    if ((chunk.Format == BBCAT_WAVE_FORMAT_EXTENSIBLE) && (length >= sizeof(WAVEFORMAT_EXTENSIBLE_CHUNK)))
    {
      WAVEFORMAT_EXTENSIBLE_CHUNK& exchunk = *(WAVEFORMAT_EXTENSIBLE_CHUNK *)data;

      BYTESWAP_VAR(exchunk.ExtensionSize);
      BYTESWAP_VAR(exchunk.Samples.Reserved);
      BYTESWAP_VAR(exchunk.ChannelMask);
    }
  }
}

bool RIFFfmtChunk::ProcessChunkData()
{
  const WAVEFORMAT_CHUNK& chunk = *(const WAVEFORMAT_CHUNK *)data;
  bool success = false;

  if ((chunk.Format == BBCAT_WAVE_FORMAT_PCM)  ||
      (chunk.Format == BBCAT_WAVE_FORMAT_IEEE) ||
      (chunk.Format == BBCAT_WAVE_FORMAT_EXTENSIBLE))
  {
    // cannot handle anything other that PCM samples
    const WAVEFORMAT_EXTENSIBLE_CHUNK& exchunk = *(const WAVEFORMAT_EXTENSIBLE_CHUNK *)data;
    uint_t _bitspersample = chunk.BitsPerSample;

    BBCDEBUG2(("Reading format data"));

    if ((chunk.Format == BBCAT_WAVE_FORMAT_EXTENSIBLE) &&
        (length >= sizeof(exchunk))   &&
        (exchunk.ExtensionSize >= 22) &&
        (exchunk.Samples.ValidBitsPerSample > 0))
    {
      // explicit bits per sample specified by exchunk
      _bitspersample = std::min(_bitspersample, (uint_t)exchunk.Samples.ValidBitsPerSample);
    }

    // set parameters within SoundFormat according to data from this chunk
    SetSampleRate(chunk.SampleRate);
    SetChannels(chunk.Channels);

    // best guess at sample data format
    if (chunk.Format == BBCAT_WAVE_FORMAT_IEEE)
    {
      SetSampleFormat((_bitspersample == 32) ? SampleFormat_Float : SampleFormat_Double);
    }
    else if (_bitspersample <= 16)
    {
      SetSampleFormat(SampleFormat_16bit);
    }
    else if (_bitspersample <= 24)
    {
      SetSampleFormat(SampleFormat_24bit);
    }
    else
    {
      SetSampleFormat(SampleFormat_32bit);
    }

    // WAVE is always little-endian
    SetSamplesBigEndian(false);

    success = true;
  }
  else BBCERROR("Format is 0x%04x, not PCM", chunk.Format);

  return success;
}

// create write data
bool RIFFfmtChunk::CreateWriteData()
{
  bool success = (length && data);

  if (!success)
  {
    WAVEFORMAT_CHUNK chunk;

    memset(&chunk, 0, sizeof(chunk));

    chunk.Format         = limited::inrange(format, _SampleFormat_Float_First, _SampleFormat_Float_Last) ? BBCAT_WAVE_FORMAT_IEEE : BBCAT_WAVE_FORMAT_PCM;
    chunk.SampleRate     = samplerate;
    chunk.Channels       = channels;
    chunk.BytesPerSecond = samplerate * channels * bytespersample;
	chunk.BitsPerSample  = bbcat::GetBitsPerSample(format);
    chunk.BlockAlign     = channels * bytespersample;

    length = sizeof(chunk);
    if ((data = new uint8_t[length]) != NULL)
    {
      memcpy(data, &chunk, length);

      success = true;
    }
  }

  return success;
}

/*----------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
/** bext chunk - specifies the Broadcast Extension data
 *
 * The chunk data is read, byte swapped but not processed
 *
 */
/*--------------------------------------------------------------------------------*/
void RIFFbextChunk::Register()
{
  RIFFChunk::RegisterProvider("bext", &Create);
}

void RIFFbextChunk::ByteSwapData(bool writing)
{
  UNUSED_PARAMETER(writing);

  if (SwapLittleEndian() && data && (length >= sizeof(BROADCAST_CHUNK)))
  {
    BROADCAST_CHUNK& chunk = *(BROADCAST_CHUNK *)data;
  
    BYTESWAP_VAR(chunk.TimeReferenceLow);
    BYTESWAP_VAR(chunk.TimeReferenceHigh);
    BYTESWAP_VAR(chunk.Version);
    BYTESWAP_VAR(chunk.LoudnessValue);
    BYTESWAP_VAR(chunk.LoudnessRange);
    BYTESWAP_VAR(chunk.MaxTruePeakLevel);
    BYTESWAP_VAR(chunk.MaxMomentaryLoudness);
    BYTESWAP_VAR(chunk.MaxShortTermLoudness);
  }
}


/*--------------------------------------------------------------------------------*/
/** Assign a length-limited, possibly unterminated, string to a std::string
 */
/*--------------------------------------------------------------------------------*/
void RIFFbextChunk::GetUnterminatedString(std::string& res, const char *str, size_t maxlen)
{
  size_t i;

  // find length of string whilst being limited to a specified length
  // (this may seem contrived but strlen() will keep searching for a nul, way off the end of the array)
  for (i = 0; (i < maxlen) && str[i]; i++) ;

  res.assign(str, i);
}

#define GET_BEXT_STRING(name)                                           \
std::string RIFFbextChunk::Get##name() const                            \
{                                                                       \
  const BROADCAST_CHUNK *chunk = (const BROADCAST_CHUNK *)data;         \
  std::string res;                                                      \
                                                                        \
  if (chunk && (length >= sizeof(*chunk))) GetUnterminatedString(res, chunk->name, sizeof(chunk->name)); \
                                                                        \
  return res;                                                           \
}

#define GET_BEXT_VALUE(type, name)                              \
type RIFFbextChunk::Get##name() const                           \
{                                                               \
  const BROADCAST_CHUNK *chunk = (const BROADCAST_CHUNK *)data; \
  type value = 0;                                               \
                                                                \
  if (chunk && (length >= sizeof(*chunk))) value = chunk->name; \
                                                                \
  return value;                                                 \
}

GET_BEXT_STRING(Description);
GET_BEXT_STRING(Originator);
GET_BEXT_STRING(OriginatorReference);
GET_BEXT_STRING(OriginationDate);
GET_BEXT_STRING(OriginationTime);

uint64_t RIFFbextChunk::GetTimeReference() const
{
  const BROADCAST_CHUNK *chunk = (const BROADCAST_CHUNK *)data;
  uint64_t value = 0;

  // coding history is an arbitary length string on the end of the structure
  if (chunk && (length >= sizeof(*chunk))) value = ((uint64_t)chunk->TimeReferenceHigh << 32) | (uint64_t)chunk->TimeReferenceLow;

  return value;
}

GET_BEXT_VALUE(uint_t, Version);

const uint8_t *RIFFbextChunk::GetUMID() const
{
  const BROADCAST_CHUNK *chunk = (const BROADCAST_CHUNK *)data;
  const uint8_t *umid = NULL;

  if (chunk && (length >= sizeof(*chunk))) umid = chunk->UMID;
  
  return umid;
}

GET_BEXT_VALUE(uint_t, LoudnessValue);
GET_BEXT_VALUE(uint_t, LoudnessRange);
GET_BEXT_VALUE(uint_t, MaxTruePeakLevel);
GET_BEXT_VALUE(uint_t, MaxMomentaryLoudness);
GET_BEXT_VALUE(uint_t, MaxShortTermLoudness);

std::string RIFFbextChunk::GetCodingHistory() const
{
  const BROADCAST_CHUNK *chunk = (const BROADCAST_CHUNK *)data;
  std::string res;
  
  // coding history is an arbitary length string on the end of the structure
  if (chunk && (length >= sizeof(*chunk))) GetUnterminatedString(res, chunk->CodingHistory, length - sizeof(*chunk));
  
  return res;
}

#define SET_BEXT_STRING(name)                                           \
void RIFFbextChunk::Set##name(const char *str)                          \
{                                                                       \
  /* ensure data block is big enough */                                 \
  CreateChunkData(sizeof(BROADCAST_CHUNK));                             \
                                                                        \
  BROADCAST_CHUNK *chunk = (BROADCAST_CHUNK *)data;                     \
  if (chunk && (length >= sizeof(*chunk))) strncpy(chunk->name, str, sizeof(chunk->name)); \
}

#define SET_BEXT_VALUE(type, name)                              \
void RIFFbextChunk::Set##name(type value)                       \
{                                                               \
  /* ensure data block is big enough */                         \
  CreateChunkData(sizeof(BROADCAST_CHUNK));                     \
                                                                \
  BROADCAST_CHUNK *chunk = (BROADCAST_CHUNK *)data;             \
  if (chunk && (length >= sizeof(*chunk))) chunk->name = value; \
}

SET_BEXT_STRING(Description);
SET_BEXT_STRING(Originator);
SET_BEXT_STRING(OriginatorReference);
SET_BEXT_STRING(OriginationDate);
SET_BEXT_STRING(OriginationTime);

void RIFFbextChunk::SetTimeReference(uint64_t value)
{
  /* ensure data block is big enough */
  CreateChunkData(sizeof(BROADCAST_CHUNK));

  BROADCAST_CHUNK *chunk = (BROADCAST_CHUNK *)data;
  if (chunk && (length >= sizeof(*chunk)))
  {
    chunk->TimeReferenceHigh = (uint32_t)(value >> 32);
    chunk->TimeReferenceLow  = (uint32_t)value;
  }
}

SET_BEXT_VALUE(uint_t, Version);
void RIFFbextChunk::SetUMID(const uint8_t umid[64])
{
  /* ensure data block is big enough */
  CreateChunkData(sizeof(BROADCAST_CHUNK));

  BROADCAST_CHUNK *chunk = (BROADCAST_CHUNK *)data;
  if (chunk && (length >= sizeof(*chunk))) memcpy(chunk->UMID, umid, sizeof(chunk->UMID));
}

SET_BEXT_VALUE(uint_t, LoudnessValue);
SET_BEXT_VALUE(uint_t, LoudnessRange);
SET_BEXT_VALUE(uint_t, MaxTruePeakLevel);
SET_BEXT_VALUE(uint_t, MaxMomentaryLoudness);
SET_BEXT_VALUE(uint_t, MaxShortTermLoudness);

void RIFFbextChunk::SetCodingHistory(const char *str)
{
  size_t len = strlen(str);

  /* ensure data block is big enough - it is likely to be expanded by this function */
  CreateChunkData(sizeof(BROADCAST_CHUNK) + len);

  BROADCAST_CHUNK *chunk = (BROADCAST_CHUNK *)data;
  if (chunk && (length >= (sizeof(*chunk) + len))) memcpy(chunk->CodingHistory, str, len);       // note terminator is NOT copied
}

// create write data
bool RIFFbextChunk::CreateWriteData()
{
  bool success = (length && data);

  if (!success)
  {
    // this should never be called unless nothing has already created the chunk and populated it!
    success = CreateChunkData(sizeof(BROADCAST_CHUNK));
  }
  
  return success;
}

/*----------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
/** chna chunk - part of the ADM (EBU Tech 3364)
 *
 * The chunk data is read, byte swapped but not processed (it is handled by the parent)
 *
 */
/*--------------------------------------------------------------------------------*/
void RIFFchnaChunk::Register()
{
  RIFFChunk::RegisterProvider("chna", &Create);
}

void RIFFchnaChunk::ByteSwapData(bool writing)
{
  if (SwapLittleEndian() && data && (length >= sizeof(CHNA_CHUNK)))
  {
    CHNA_CHUNK& chunk = *(CHNA_CHUNK *)data;  
    uint16_t i;

    if (writing)
    {
      // when writing, swap these BEFORE chunk.UIDCount is byte-swapped
      for (i = 0; i < chunk.UIDCount; i++)
      {
        BYTESWAP_VAR(chunk.UIDs[i].TrackNum);
      }
    }

    BYTESWAP_VAR(chunk.TrackCount);
    BYTESWAP_VAR(chunk.UIDCount);

    if (!writing)
    {
      // when reading, swap these AFTER chunk.UIDCount is byte-swapped
      for (i = 0; i < chunk.UIDCount; i++)
      {
        BYTESWAP_VAR(chunk.UIDs[i].TrackNum);
      }
    }
  }
}

/*----------------------------------------------------------------------------------------------------*/

RIFFaxmlChunk::RIFFaxmlChunk(uint32_t chunk_id) : RIFFChunk(chunk_id)
{
  // include an extra byte when allocating/reading data for string terminator
  extrabytes = 1;
}

/*--------------------------------------------------------------------------------*/
/** axml chunk - part of the ADM (EBU Tech 3364)
 *
 * The chunk data is read, not byte swapped (it is pure XML) and not processed (it is handled by the parent)
 *
 */
/*--------------------------------------------------------------------------------*/
void RIFFaxmlChunk::Register()
{
  RIFFChunk::RegisterProvider("axml", &Create);
}

/*----------------------------------------------------------------------------------------------------*/

RIFFdataChunk::~RIFFdataChunk()
{
}

/*--------------------------------------------------------------------------------*/
/** data chunk - WAVE data
 *
 * The data is not read (it could be too big to fit in memory)
 *
 * Note that the object is also derived from SoundfileSamples which provides sample
 * reading and conversion facilities
 *
 */
/*--------------------------------------------------------------------------------*/
bool RIFFdataChunk::ReadChunk(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler)
{
  bool success = false;

  if (RIFFChunk::ReadChunk(file, sizehandler))
  {
    // link file to SoundFileSamples object
    SetFile(file, datapos, length);

    success = true;
  }

  return success;
}

// set up data length before data is written
bool RIFFdataChunk::CreateWriteData()
{
  // set length
  length = totalbytes;
  return true;
}

// move over data that already been written
bool RIFFdataChunk::WriteChunkData(EnhancedFile *file)
{
  bool success = false;
  
  // tell SoundFileSamples about file it needs to write to
  SetFile(file, file->ftell(), length, false);

  if ((success = file->fseek(length, SEEK_CUR) == 0) == false) BBCERROR("Failed to seek over sample data");

  return success;
}

void RIFFdataChunk::Register()
{
  RIFFChunk::RegisterProvider("data", &Create);
}

/*----------------------------------------------------------------------------------------------------*/

UserRIFFChunk::UserRIFFChunk(uint32_t chunk_id, const void *_data, uint64_t _length, bool _beforesamples) : RIFFChunk(chunk_id),
                                                                                                            beforesamples(_beforesamples)
{
  CreateChunkData(_data, _length);
}

/*--------------------------------------------------------------------------------*/
/** Register all providers from this file
 */
/*--------------------------------------------------------------------------------*/
void RegisterRIFFChunkProviders()
{
  RIFFRIFFChunk::Register();
  RIFFWAVEChunk::Register();
  RIFFds64Chunk::Register();
  RIFFfmtChunk::Register();
  RIFFbextChunk::Register();
  RIFFchnaChunk::Register();
  RIFFaxmlChunk::Register();
  RIFFdataChunk::Register();
}

BBC_AUDIOTOOLBOX_END
