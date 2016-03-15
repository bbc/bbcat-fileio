
#include <string.h>
#include <errno.h>

#define BBCDEBUG_LEVEL 1
#include <bbcat-base/ByteSwap.h>

#include "RIFFChunk.h"
#include "RIFFFile.h"

BBC_AUDIOTOOLBOX_START

std::map<uint32_t, RIFFChunk::PROVIDER> RIFFChunk::providermap;

const uint64_t RIFFChunk::RIFF_MaxSize = 0xffffffff;

/*--------------------------------------------------------------------------------*/
/** Constructor - can only be called by static member function!
 */
/*--------------------------------------------------------------------------------*/
RIFFChunk::RIFFChunk(uint32_t chunk_id) : id(chunk_id),
                                          length(0),
                                          extrabytes(0),
                                          datapos(0),
                                          data(NULL),
                                          align(1),
                                          riff64(false)
{
  // create ASCII name from ID
  char _name[] = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id};

  name.assign(_name, sizeof(_name));
}

RIFFChunk::~RIFFChunk()
{
  DeleteData();
}

/*--------------------------------------------------------------------------------*/
/** Read chunk data length and decide what to do
 *
 * @return true if chunk successfully read/handled
 */
/*--------------------------------------------------------------------------------*/
bool RIFFChunk::ReadChunk(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler)
{
  uint32_t length32;    // internal length is 64-bit so need 32-bit variable temporarily
  bool success = false;

  // chunk ID has already been read, next is chunk length
  if (file && (file->fread(&length32, sizeof(length32), 1) == 1))
  {
    // length is stored little-endian
    ByteSwap(length32, SWAP_FOR_LE);

    // allow ds64 chunk to override length
    if (sizehandler) length = sizehandler->GetChunkSize(GetID(), length32);
    else             length = length32;

    // save file position
    datapos = file->ftell();

    BBCDEBUG2(("Chunk '%s' is %s bytes long", GetName(), StringFrom(length).c_str()));

    // Process chunk
    switch (GetChunkHandling())
    {
      default:
      case ChunkHandling_SkipOverChunk:
        BBCDEBUG2(("Skipping chunk '%s'", GetName()));

        // skip to end of chunk
        if (file->fseek(datapos + length + (length & align), SEEK_SET) == 0) success = true;
        else
        {
          BBCERROR("Failed to seek to end of chunk '%s' (position %s), error %s", GetName(), StringFrom(datapos + length).c_str(), strerror(file->ferror()));
        }
                
        break;

      case ChunkHandling_RemainInChunkData:
        // remain at current position
        success = true;
        break;

      case ChunkHandling_ReadChunk:
        BBCDEBUG2(("Reading and processing chunk '%s'", GetName()));

        // read and process chunk
        if (ReadData(file))
        {
          // process data
          success = ProcessChunkData();

          // if data is not needed after processing, delete it
          if (DeleteDataAfterProcessing())
          {
            DeleteData();
          }
        }
        else
        {
          // failed to read data, skip over it for next chunk
          BBCERROR("Failed to read %s bytes of chunk '%s', error %s", StringFrom(length).c_str(), GetName(), strerror(file->ferror()));

          if (file->fseek(datapos + length + (length & align), SEEK_SET) != 0)
          {
            BBCERROR("Failed to seek to end of chunk '%s' (position %s) (after chunk read failure), error %s", GetName(), StringFrom(datapos + length).c_str(), strerror(file->ferror()));
          }
        }
        break;
    }
  }
  else BBCERROR("Failed to read chunk '%s' length, error %s", GetName(), strerror(file->ferror()));

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Read chunk data and byte swap it (derived object provided)
 *
 * @return true if chunk successfully read
 */
/*--------------------------------------------------------------------------------*/
bool RIFFChunk::ReadData(EnhancedFile *file)
{
  bool success = false;

  if (!data && length)
  {
    // allocate data for chunk data (allow extra space at end of chunk for terminators, etc)
    if ((data = new uint8_t[length + extrabytes]) != NULL)
    {
      // clear extra data
      if (extrabytes) memset(data + length, 0, extrabytes);
      
      // seek to correct position in file (will probably already be there)
      if (file && (file->fseek(datapos, SEEK_SET) == 0))
      {
        // read chunk data
        if (file->fread(data, length, 1) == 1)
        {
          // swap byte ordering for data
          ByteSwapData();

          if (length & align)
          {
            success = (file->fseek(length & align, SEEK_CUR) == 0);
          }
          else success = true;
        }
        else BBCERROR("Failed to read %s bytes for chunk '%s' data, error %s", StringFrom(length).c_str(), GetName(), strerror(file->ferror()));
      }
      else BBCERROR("Failed to seek to position %s to read %s bytes for chunk '%s' data, error %s", StringFrom(datapos).c_str(), StringFrom(length).c_str(), GetName(), strerror(file->ferror()));
    }
    else BBCERROR("Failed to allocate %s bytes for chunk '%s' data", StringFrom(length).c_str(), GetName());
  }
  else success = true;

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Write chunk data
 *
 * @return true if chunk successfully written
 *
 * @note MUST be done when the file is closed!
 */
/*--------------------------------------------------------------------------------*/
bool RIFFChunk::WriteChunk(EnhancedFile *file)
{
  bool success = CreateWriteData();

  if (success && WriteThisChunk())
  {
    // if chunk is marked as RIFF64, explicit store 0xffffffff as the length
    const uint32_t maxsize = RIFF_MaxSize;
    uint32_t data[] = {GetWriteID(), (uint32_t)(riff64 ? maxsize : std::min(length, (uint64_t)maxsize))};

    // treat ID as big-endian, length is little-endian
    ByteSwap(data[0], SWAP_FOR_BE);
    ByteSwap(data[1], SWAP_FOR_LE);

    if (file->fwrite(data, sizeof(data[0]), NUMBEROF(data)) > 0)
    {
      datapos = file->ftell();
      if (WriteChunkData(file))
      {
        // if chunk length is odd, write an extra byte to pad the file
        if (length & align)
        {
          uint8_t _pad = 0;

          success = (file->fwrite(&_pad, sizeof(_pad), 1) > 0);
        }
        else success = true;
      }
    }
    else
    {
      BBCERROR("Failed to write chunk header, error %s", strerror(errno));
    }
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Supply chunk data for writing
 */
/*--------------------------------------------------------------------------------*/
bool RIFFChunk::CreateChunkData(const void *_data, uint64_t _length)
{
  bool success = false;

  if (_data && data)
  {
    delete[] data;
    data = NULL;
  }

  length = _length;
  // include additional storage specified by extrabytes
  if (_data && ((data = new uint8_t[length + extrabytes]) != NULL))
  {
    memcpy(data, _data, length + extrabytes);
    success = true;
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Update chunk data for writing
 *
 * @note returns false is length is different!
 */
/*--------------------------------------------------------------------------------*/
bool RIFFChunk::UpdateChunkData(const void *_data, uint64_t _length)
{
  bool success = false;

  if (data && (_length == length))
  {
    memcpy(data, _data, length);
    success = true;
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Create/update data to the specified size
 *
 * @note if a data block already exists it will be copied to the new block
 * @note this functions allows expansion of but *not* shrinking of the data block
 */
/*--------------------------------------------------------------------------------*/
bool RIFFChunk::CreateChunkData(uint64_t _length)
{
  bool success = false;

  if (!data || (_length > length))
  {
    uint8_t  *olddata  = data;
    uint64_t oldlength = length;

    length = _length;

    if ((data = new uint8_t[length + extrabytes]) != NULL)
    {
      // clear extra bytes
      if (extrabytes) memset(data + length, 0, extrabytes);
      
      // if old data exists, copy it and then clear the remainder
      if (olddata && oldlength)
      {
        memcpy(data, olddata, oldlength);

        // clear any remainder
        if (length > oldlength) memset(data + oldlength, 0, length - oldlength);
      }
      // else just clear entire data
      else memset(data, 0, length);
      success = true;
    }

    if (olddata) delete[] olddata;
  }
  // no need to do anything, block isn't changing
  else success = true;

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Write chunk data
 *
 * @return true if chunk successfully written
 *
 * @note if data exists it will be written otherwise this function will fail!
 */
/*--------------------------------------------------------------------------------*/
bool RIFFChunk::WriteChunkData(EnhancedFile *file)
{
  bool success = false;

  if (data)
  {
    // byte swap JUST before writing!
    ByteSwapData(true);
    success = (file->fwrite(data, 1, length) == length);
    // now byte swap back
    ByteSwapData(false);
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Delete read data
 */
/*--------------------------------------------------------------------------------*/
void RIFFChunk::DeleteData()
{
  if (data)
  {
    delete[] data;
    data = NULL;
  }
}

/*--------------------------------------------------------------------------------*/
/** Register a chunk handler
 *
 * @param id 32-bit chunk ID (big-endian format)
 * @param fn function to create RIFFChunk derived object
 * @param context optional userdata to be supplied to the above function
 *
 */
/*--------------------------------------------------------------------------------*/
void RIFFChunk::RegisterProvider(uint32_t id, RIFFChunk *(*fn)(uint32_t id, void *context), void *context)
{
  PROVIDER provider =
  {
    fn,          // creator function
    context,     // user supplied data for creator
  };

  // save creator against chunk ID
  providermap[id] = provider;
}

/*--------------------------------------------------------------------------------*/
/** Register a chunk handler
 *
 * @param name ASCII chunk name (optionally terminated)
 * @param fn function to create RIFFChunk derived object
 * @param context optional userdata to be supplied to the above function
 *
 */
/*--------------------------------------------------------------------------------*/
void RIFFChunk::RegisterProvider(const char *name, RIFFChunk *(*fn)(uint32_t id, void *context), void *context)
{
  RegisterProvider(IFFID(name), fn, context);
}

/*--------------------------------------------------------------------------------*/
/** Return ASCII name representation of chunk ID
 */
/*--------------------------------------------------------------------------------*/
std::string RIFFChunk::GetChunkName(uint32_t id)
{
  char array[] = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id};
  return std::string(array, sizeof(array));
}

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
RIFFChunk *RIFFChunk::Create(EnhancedFile *file, const RIFFChunkSizeHandler *sizehandler)
{
  RIFFChunk *chunk = NULL;
  uint32_t id;
  bool success = false;

  // read chunk ID
  if (file && (file->fread(&id, sizeof(id), 1) == 1))
  {
    // treat ID as big-endian
    ByteSwap(id, SWAP_FOR_BE);

    // find provider to create RIFFChunk object
    std::map<uint32_t, PROVIDER>::iterator it = providermap.find(id);

    if (it != providermap.end())
    {
      // a provider is available
      const PROVIDER& provider = it->second;

      if ((chunk = (*provider.fn)(id, provider.context)) != NULL)
      {
        BBCDEBUG4(("Found provider for chunk '%s'", GetChunkName(id).c_str()));
                
        // let object handle the rest of the chunk
        if (chunk->ReadChunk(file, sizehandler))
        {
          BBCDEBUG4(("Read chunk '%s' successfully", GetChunkName(id).c_str()));
                    
          success = true;
        }
      }
    }
    else
    {
      // if no provider is available, use the base-class to provide basic functionality
      BBCDEBUG2(("No handler found for chunk '%s', creating empty one", GetChunkName(id).c_str()));

      if ((chunk = new RIFFChunk(id)) != NULL)
      {
        success = chunk->ReadChunk(file, sizehandler);
      }
    }
  }

  // if there was a failure, delete the object
  if (!success && chunk)
  {
    delete chunk;
    chunk = NULL;
  }

  return chunk;
}

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

RIFFChunk *RIFFChunk::Create(uint32_t id)
{
  RIFFChunk *chunk = NULL;

  // find provider to create RIFFChunk object
  std::map<uint32_t, PROVIDER>::iterator it = providermap.find(id);

  if (it != providermap.end())
  {
    // a provider is available
    const PROVIDER& provider = it->second;

    if ((chunk = (*provider.fn)(id, provider.context)) != NULL)
    {
      BBCDEBUG4(("Found provider for chunk '%s'", GetChunkName(id).c_str()));
    }
  }
  else
  {
    // if no provider is available, use the base-class to provide basic functionality
    BBCDEBUG2(("No handler found for chunk '%s', creating empty one", GetChunkName(id).c_str()));

    chunk = new RIFFChunk(id);
  }

  if (chunk && !chunk->InitialiseForWriting())
  {
    BBCERROR("Failed to initialise chunk '%s' for writing", GetChunkName(id).c_str());
    delete chunk;
    chunk = NULL;
  }

  return chunk;
}

RIFFChunk *RIFFChunk::Create(const char *name)
{
  return Create(IFFID(name));
}

bool RIFFChunk::SwapBigEndian()
{
  static const bool swap = !MACHINE_IS_BIG_ENDIAN;
  return swap;        // true if machine is little-endian and therefore big-endian data should be swapped
}

bool RIFFChunk::SwapLittleEndian()
{
  static const bool swap = MACHINE_IS_BIG_ENDIAN;
  return swap;        // true if machine is bit-endian and therefore little-endian data should be swapped
}

BBC_AUDIOTOOLBOX_END
