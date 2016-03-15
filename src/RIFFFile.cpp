
#include <math.h>

#define BBCDEBUG_LEVEL 3

#include <bbcat-base/BackgroundFile.h>

#include "RIFFFile.h"
#include "RIFFChunk_Definitions.h"

BBC_AUDIOTOOLBOX_START

RIFFFile::RIFFFile() : filetype(FileType_Unknown),
                       fileformat(NULL),
                       filesamples(NULL),
                       writing(false),
                       backgroundwriting(false)
{
  if (sizeof(off_t) < sizeof(uint64_t))
  {
    BBCERROR("System does *NOT* support 64-bit files!");
  }

  if (RIFFChunk::NoProvidersRegistered())
  {
    BBCDEBUG2(("No RIFF chunk providers registered, registering some..."));
    RegisterRIFFChunkProviders();
  }
}

RIFFFile::~RIFFFile()
{
  Close();
}

bool RIFFFile::ReadChunks(uint64_t maxlength)
{
  bool success = false;

  if (IsOpen())
  {
    EnhancedFile        *file = fileref;
    const RIFFds64Chunk *ds64 = NULL;
    RIFFChunk *chunk;
    uint64_t  startpos = file->ftell();

    success = true;

    while (success &&
           ((file->ftell() - startpos) < maxlength) &&
           ((chunk = RIFFChunk::Create(file, ds64)) != NULL))
    {

      chunklist.push_back(chunk);
      chunkmap[chunk->GetID()] = chunk;

      if ((chunk->GetID() == ds64_ID) && ((ds64 = dynamic_cast<const RIFFds64Chunk *>(chunk)) != NULL))
      {
        BBCDEBUG3(("Found ds64 chunk, will be used to set chunk sizes if necessary"));

        if (maxlength == RIFFChunk::RIFF_MaxSize)
        {
          maxlength = ds64->GetRIFFSize();

          BBCDEBUG2(("Updated RIFF size to %s bytes", StringFrom(maxlength).c_str()));
        }
      }

      if ((dynamic_cast<const SoundFormat *>(chunk)) != NULL)
      {
        fileformat = dynamic_cast<SoundFormat *>(chunk);
        if (filesamples) filesamples->SetFormat(fileformat);

        BBCDEBUG3(("Found format chunk (%s)", chunk->GetName()));
      }

      if ((dynamic_cast<const SoundFileSamples *>(chunk)) != NULL)
      {
        filesamples = dynamic_cast<SoundFileSamples *>(chunk);
        if (fileformat) filesamples->SetFormat(fileformat);

        BBCDEBUG3(("Found data chunk (%s)", chunk->GetName()));
      }

      success = ProcessChunk(chunk);
    }

    if (success)
    {
      success = PostReadChunks();
      if (!success) BBCERROR("Failed post read chunks processing");
    }
  }

  return success;
}

bool RIFFFile::Open(const char *filename)
{
  bool success = false;

  if (!IsOpen())
  {
    EnhancedFile *file;

    if (((file = (fileref = new EnhancedFile)) != NULL) && file->fopen(filename, "rb"))
    {
      RIFFChunk *chunk;

      if ((chunk = RIFFChunk::Create(file)) != NULL)
      {
        chunklist.push_back(chunk);
        chunkmap[chunk->GetID()] = chunk;

        if ((chunk->GetID() == RIFF_ID) || (chunk->GetID() == RF64_ID))
        {
          filetype = FileType_WAV;
          success  = ReadChunks(chunk->GetLength());
        }
      }
    }

    if (!success) Close();
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Enable/disable background file writing
 *
 * @note can be called at any time to enable/disable
 */
/*--------------------------------------------------------------------------------*/
void RIFFFile::EnableBackgroundWriting(bool enable)
{
  BackgroundFile *file;

  backgroundwriting = enable;

  // if we're writing a file, and it is a background capable file, enable or disable background mode
  if (writing && ((file = dynamic_cast<BackgroundFile *>(fileref.Obj())) != NULL))
  {
    file->EnableBackground(backgroundwriting);
  }
}

/*--------------------------------------------------------------------------------*/
/** Create a WAVE/RIFF file
 *
 * @param filename filename of file to create
 * @param samplerate sample rate of audio
 * @param nchannels number of audio channels
 * @param format sample format of audio in file
 *
 * @return true if file created properly
 */
/*--------------------------------------------------------------------------------*/
bool RIFFFile::Create(const char *filename, uint32_t samplerate, uint_t nchannels, SampleFormat_t format)
{
  bool success = false;

  if (!IsOpen())
  {
    // use background file writing mechanism
    BackgroundFile *file;

    if (samplerate && nchannels &&
        ((file = (new BackgroundFile)) != NULL) && file->fopen(filename, "wb+"))
    {
      const uint32_t ids[] = {RIFF_ID, WAVE_ID, ds64_ID, fmt_ID, data_ID};
      uint_t i;

      // NOTE: file starts in foreground writing mode!
      fileref = file;

      writing = true;

      for (i = 0; i < NUMBEROF(ids); i++)
      {
        if (!AddChunk(ids[i])) break;
      }

      if (i == NUMBEROF(ids))
      {
        if (fileformat && filesamples)
        {
          fileformat->SetSampleRate(samplerate);
          fileformat->SetChannels(nchannels);
          fileformat->SetSampleFormat(format);
          fileformat->SetSamplesBigEndian(false);       // WAVE is little-endian

          filesamples->SetFormat(fileformat);

          filetype = FileType_WAV;

          if (CreateExtraChunks())
          {
            RIFFds64Chunk *ds64;
            if ((ds64 = dynamic_cast<RIFFds64Chunk *>(chunkmap[ds64_ID])) != NULL)
            {
              // tell ds64 chunk the maximum number of chunks that might need a table entry
              // this can be calculated from the number of chunks created
              ds64->SetTableCount((uint32_t)chunklist.size() - 5);        // none of the above chunks need a table entry (RIFF and data chunks have dedicated entries in the ds64 chunk)
            }

            WriteChunks(false);

            success  = true;
          }
          else BBCERROR("Failed to create extra chunks for file writing");
        }
        else BBCERROR("No file format and/or file samples chunks created");
      }
      else BBCERROR("Failed to create chunks (failed chunk: %08lx)", (ulong_t)ids[i]);
    }

    if (!success) Close();
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Write all chunks necessary
 *
 * @param closing true if file is closing and so chunks can be written after data chunk
 */
/*--------------------------------------------------------------------------------*/
void RIFFFile::WriteChunks(bool closing)
{
  EnhancedFile   *file  = fileref;
  BackgroundFile *bfile = dynamic_cast<BackgroundFile *>(file);
  RIFFChunk *chunk;
  uint_t i;

  // ensure file writing is foreground for this purpose
  if (bfile) bfile->EnableBackground(false);

  file->rewind();

  if (!closing)
  {
    // tell each chunk to create it's data
    for (i = 0; i < chunklist.size(); i++)
    {
      chunklist[i]->CreateWriteData();
    }
  }

  // write chunks that need to be written before samples chunk
  for (i = 0; i < chunklist.size(); i++)
  {
    chunk = chunklist[i];

    // write chunk if it can be (ALL chunks are re-written on close anyway)
    if ((chunk->GetID() != data_ID) && chunk->WriteChunkBeforeSamples())
    {
      BBCDEBUG2(("%s: %s chunk '%s' size %s bytes at %s (actually %s bytes)", closing ? "Closing" : "Creating", chunk->WriteThisChunk() ? "Writing" : "SKIPPING", chunk->GetName(), StringFrom(chunk->GetLength()).c_str(), StringFrom(file->ftell()).c_str(), StringFrom(chunk->GetLengthOnFile()).c_str()));
      chunk->WriteChunk(file);
    }
  }

  // now write (proto) data chunk
  if ((chunk = GetChunk(data_ID)) != NULL)
  {
    BBCDEBUG2(("%s: %s chunk '%s' size %s bytes at %s (actually %s bytes)", closing ? "Closing" : "Creating", chunk->WriteThisChunk() ? "Writing" : "SKIPPING", chunk->GetName(), StringFrom(chunk->GetLength()).c_str(), StringFrom(file->ftell()).c_str(), StringFrom(chunk->GetLengthOnFile()).c_str()));
    chunk->WriteChunk(file);
  }

  if (closing)
  {
    // write chunks that need to be written after samples
    for (i = 0; i < chunklist.size(); i++)
    {
      chunk = chunklist[i];

      if ((chunk->GetID() != data_ID) && !chunk->WriteChunkBeforeSamples())
      {
        BBCDEBUG2(("%s: %s chunk '%s' size %s bytes at %s (actually %s bytes)", closing ? "Closing" : "Creating", chunk->WriteThisChunk() ? "Writing" : "SKIPPING", chunk->GetName(), StringFrom(chunk->GetLength()).c_str(), StringFrom(file->ftell()).c_str(), StringFrom(chunk->GetLengthOnFile()).c_str()));
        chunk->WriteChunk(file);
      }
    }
  }

  if (!closing && bfile)
  {
    // now switch to background writing mode if enabled
    bfile->EnableBackground(backgroundwriting);
  }
}

/*--------------------------------------------------------------------------------*/
/** Close RIFF file, writing chunks if file was opened for writing
 *
 * @param abortwrite true to abort the writing of file
 *
 * @note this may take some time because it copies sample data from a temporary file
 */
/*--------------------------------------------------------------------------------*/
void RIFFFile::Close(bool abortwrite)
{
  EnhancedFile *file = fileref;
  uint_t i;

  if (file)
  {
    if (writing && !abortwrite)
    {
      RIFFds64Chunk  *ds64  = dynamic_cast<RIFFds64Chunk *>(GetChunk(ds64_ID));
      RIFFChunk      *chunk;
      const uint64_t maxsize = RIFFChunk::RIFF_MaxSize; // max size of chunks and file before switching to RF64

      BBCDEBUG1(("Closing file '%s'...", file->getfilename().c_str()));

      // now total up all the bytes for each chunk
      uint64_t totalbytes = 0;
      for (i = 0; i < chunklist.size(); i++)
      {
        uint64_t bytes;

        chunk = chunklist[i];

        // update data chunk size
        if (chunk->GetID() == data_ID) chunk->CreateWriteData();

        // ignore RIFF ID since the RIFF size refers to the rest of the content
        if (chunk->GetID() != RIFF_ID)
        {
          bytes = chunk->GetLengthOnFile();

          BBCDEBUG3(("Chunk '%s' has length %s bytes", chunk->GetName(), StringFrom(bytes).c_str()));
          totalbytes += bytes;
        }

        // check whether chunk length (NOT length on file) is too big
        // and needs an entry in the ds64 chunk
        // (obviously if *any* chunk exceeds the maximum size, the file will as well)
        if (((bytes = chunk->GetLength()) >= maxsize) ||
            (ds64 && ((chunk->GetID() == RIFF_ID) ||  // RIFF and data chunks should *always* be in the ds64, if it exists
                      (chunk->GetID() == data_ID))))
        {
          BBCDEBUG3(("Chunk '%s' needs to be in ds64 chunk", chunk->GetName()));
          if (ds64)
          {
            if (!ds64->SetChunkSize(chunk->GetID(), bytes)) BBCERROR("Failed to set chunk size for '%s' in ds64 chunk", chunk->GetName());

            // set the sample count from the size of the data chunk
            if (chunk->GetID() == data_ID)
            {
              ds64->SetSampleCount(bytes / fileformat->GetBytesPerFrame());
            }
          }
          else BBCERROR("ds64 chunk needed but doesn't exist");
        }
      }

      // test whether RIFF64 file is needed
      if (totalbytes >= maxsize)
      {
        BBCDEBUG1(("Switching file to RF64 type"));

        // tell each chunk (that's interested) that the file is going to be a RIFF64
        for (i = 0; i < chunklist.size(); i++)
        {
          chunklist[i]->EnableRIFF64();
        }
      }

      BBCDEBUG3(("Total size %s bytes", StringFrom(totalbytes).c_str()));

      // set total length of RIFF chunk
      chunkmap[RIFF_ID]->CreateChunkData(NULL, totalbytes);

      // write/re-write all chunks
      WriteChunks(true);

      BBCDEBUG1(("Closed file '%s'", file->getfilename().c_str()));
    }

    fileref = NULL;
  }

  filetype    = FileType_Unknown;
  fileformat  = NULL;
  filesamples = NULL;
  writing     = false;

  for (i = 0; i < chunklist.size(); i++)
  {
    delete chunklist[i];
  }

  chunklist.clear();
  chunkmap.clear();
}

/*--------------------------------------------------------------------------------*/
/** Create and add a chunk to a file being written
 *
 * @param id chunk type ID
 *
 * @return pointer to chunk object or NULL
 */
/*--------------------------------------------------------------------------------*/
RIFFChunk *RIFFFile::AddChunk(uint32_t id)
{
  RIFFChunk *chunk = NULL;

  if (writing)
  {
    // ensure none of the chunk types specified below are duplicated
    if ((chunkmap.find(id) == chunkmap.end()) ||
        ((id != RIFF_ID) &&
         (id != WAVE_ID) &&
         (id != fmt_ID)  &&
         (id != ds64_ID) &&
         (id != data_ID)))
    {
      if ((chunk = RIFFChunk::Create(id)) != NULL)
      {
        chunklist.push_back(chunk);
        chunkmap[chunk->GetID()] = chunk;

        // if the chunk is a SoundFormat or SoundFileSamples chunk then use it as such

        SoundFormat *fmtchunk;
        if ((fmtchunk = dynamic_cast<SoundFormat *>(chunk)) != NULL)
        {
          fileformat = fmtchunk;
        }

        SoundFileSamples *smpschunk;
        if ((smpschunk = dynamic_cast<SoundFileSamples *>(chunk)) != NULL)
        {
          filesamples = smpschunk;
        }
      }
      else
      {
        BBCERROR("Failed to create chunk of type '%s'", RIFFChunk::GetChunkName(id).c_str());
      }
    }
    else
    {
      BBCERROR("Cannot create two copies of chunk type '%s'", RIFFChunk::GetChunkName(id).c_str());
    }
  }
  else
  {
    BBCERROR("Cannot add chunk to read only file");
  }

  return chunk;
}

/*--------------------------------------------------------------------------------*/
/** Create and add a chunk to a file being written
 *
 * @param name chunk type name
 *
 * @return pointer to chunk object or NULL
 */
/*--------------------------------------------------------------------------------*/
RIFFChunk *RIFFFile::AddChunk(const char *name)
{
  return AddChunk(IFFID(name));
}

/*--------------------------------------------------------------------------------*/
/** Add a chunk of data to the RIFF file
 *
 * @param id chunk type ID
 * @param data ptr to data for chunk
 * @param length length of data
 * @param beforesamples true to place chunk *before* data chunk, false to place it *after*
 *
 * @return pointer to chunk or NULL
 *
 * @note the data is copied into chunk so passed-in array is not required afterwards
 */
/*--------------------------------------------------------------------------------*/
RIFFChunk *RIFFFile::AddChunk(uint32_t id, const uint8_t *data, uint64_t length, bool beforesamples)
{
  RIFFChunk *chunk;

  if (beforesamples && filesamples && (filesamples->GetSamplePosition() != 0))
  {
    // create ASCII name from ID
    char _name[] = {(char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id};
    std::string name;
    
    name.assign(_name, sizeof(_name));

    BBCDEBUG("Warning: add chunk '%s' before samples requested when samples written, moving to after samples", name.c_str());
    
    beforesamples = false;
  }

  if ((chunk = new UserRIFFChunk(id, data, length, beforesamples)) != NULL)
  {
    // ensure data is valid
    if (chunk->GetData())
    {
      // add chunk to list
      chunklist.push_back(chunk);
      chunkmap[chunk->GetID()] = chunk;

      // if chunk needs to go to *before* samples, chunks need to be re-written
      if (IsOpen() && beforesamples) WriteChunks(false);
    }
    else
    {
      BBCERROR("Failed to copy data for chunk %08x", (uint_t)id);
      delete chunk;
      chunk = NULL;
    }
  }

  return chunk;
}

/*--------------------------------------------------------------------------------*/
/** Add a chunk of data to the RIFF file
 *
 * @param name chunk type name
 * @param data ptr to data for chunk
 * @param length length of data
 * @param beforesamples true to place chunk *before* data chunk, false to place it *after*
 *
 * @return pointer to chunk or NULL
 *
 * @note the data is copied into chunk so passed-in array is not required afterwards
 */
/*--------------------------------------------------------------------------------*/
RIFFChunk *RIFFFile::AddChunk(const char *name, const uint8_t *data, uint64_t length, bool beforesamples)
{
  return AddChunk(IFFID(name), data, length, beforesamples);
}

/*--------------------------------------------------------------------------------*/
/** Return chunk specified by chunk ID
 *
 * @param id 32-bit representation of chunk name (big endian)
 *
 * @return pointer to RIFFChunk object
 */
/*--------------------------------------------------------------------------------*/
RIFFChunk *RIFFFile::GetChunk(uint32_t id) const
{
  ChunkMap_t::const_iterator it;
  return ((it = chunkmap.find(id)) != chunkmap.end()) ? it->second : NULL;
}

BBC_AUDIOTOOLBOX_END
