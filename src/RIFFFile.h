#ifndef __RIFF_FILE__
#define __RIFF_FILE__

#include <vector>
#include <map>

#include <bbcat-base/RefCount.h>

#include "RIFFChunks.h"

BBC_AUDIOTOOLBOX_START

/*--------------------------------------------------------------------------------*/
/** A WAVE/RIFF file handler
 * 
 * Provides the same functionality as most classes of the same type but ascts as a
 * base class for ADMRIFFFile which supports the ADM for audio objects
 */
/*--------------------------------------------------------------------------------*/
class RIFFFile
{
public:
  RIFFFile();
  virtual ~RIFFFile();

  /*--------------------------------------------------------------------------------*/
  /** Open a WAVE/RIFF file
   *
   * @param filename filename of file to open
   *
   * @return true if file opened and interpreted correctly (including any extra chunks if present)
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool Open(const char *filename);

  /*--------------------------------------------------------------------------------*/
  /** Enable/disable background file writing
   *
   * @note can be called at any time to enable/disable
   */
  /*--------------------------------------------------------------------------------*/
  virtual void EnableBackgroundWriting(bool enable);

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
  virtual bool Create(const char *filename, uint32_t samplerate = 48000, uint_t nchannels = 2, SampleFormat_t format = SampleFormat_24bit);

  /*--------------------------------------------------------------------------------*/
  /** Return whether a file is open
   *
   * @return true if file is open
   */
  /*--------------------------------------------------------------------------------*/
  bool IsOpen() const {return (fileref.Obj() && fileref.Obj()->isopen());}

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
  /** Return the underlying file object
   *
   * @return EnhancedFile object (or NULL)
   */
  /*--------------------------------------------------------------------------------*/
  EnhancedFile *GetFile() {return fileref;}

  /// file type enumeration
  enum
  {
    FileType_Unknown = 0,
    FileType_WAV,
    FileType_AIFF,
  };

  /*--------------------------------------------------------------------------------*/
  /** Return file type
   *
   * @return FileType_xxx enumeration of file type
   */
  /*--------------------------------------------------------------------------------*/
  uint8_t GetFileType() const {return filetype;}

  /*--------------------------------------------------------------------------------*/
  /** Return sample rate of file, if open
   *
   * @return sample rate of file or 0 if no file open
   */
  /*--------------------------------------------------------------------------------*/
  uint32_t GetSampleRate() const {return fileformat ? fileformat->GetSampleRate() : 0;}

  /*--------------------------------------------------------------------------------*/
  /** Return number of channels in the file
   *
   * @return number of channels or 0 if no file open
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetChannels() const {return fileformat ? fileformat->GetChannels() : 0;}

  /*--------------------------------------------------------------------------------*/
  /** Return bytes per sample for data in the file
   *
   * @return bytes per sample
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetBytesPerSample() const {return fileformat ? fileformat->GetBytesPerSample() : 0;}

  /*--------------------------------------------------------------------------------*/
  /** Return bits per sample for data in the file
   *
   * @return bits per sample
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetBitsPerSample() const {return fileformat ? fileformat->GetBitsPerSample() : 0;}

  /*--------------------------------------------------------------------------------*/
  /** Return sample format of samples in the file
   *
   * @return SampleFormat_xxx enumeration
   */
  /*--------------------------------------------------------------------------------*/
  SampleFormat_t GetSampleFormat() const {return fileformat ? fileformat->GetSampleFormat() : SampleFormat_Unknown;}

  /*--------------------------------------------------------------------------------*/
  /** Return sample format of samples in the file
   *
   * @return SampleFormat_xxx enumeration
   */
  /*--------------------------------------------------------------------------------*/
  uint64_t GetSamplePosition() const {return filesamples ? filesamples->GetSamplePosition() : 0;}

  /*--------------------------------------------------------------------------------*/
  /** Return sample format of samples in the file
   *
   * @return SampleFormat_xxx enumeration
   */
  /*--------------------------------------------------------------------------------*/
  uint64_t GetSampleLength() const {return filesamples ? filesamples->GetSampleLength() : 0;}

  /*--------------------------------------------------------------------------------*/
  /** Set position within sample data of file
   *
   * @param pos sample position
   *
   */
  /*--------------------------------------------------------------------------------*/
  virtual void SetSamplePosition(uint64_t pos) {if (filesamples) {filesamples->SetSamplePosition(pos); UpdateSamplePosition();}}

  /*--------------------------------------------------------------------------------*/
  /** Return number of chunks found in file
   *
   * @return chunk count
   */
  /*--------------------------------------------------------------------------------*/
  uint_t GetChunkCount() const {return (uint_t)chunklist.size();}

  /*--------------------------------------------------------------------------------*/
  /** Return chunk at specifiec index
   *
   * @param index chunk index 0 .. number of chunks returned above
   *
   * @return pointer to RIFFChunk object
   */
  /*--------------------------------------------------------------------------------*/
  RIFFChunk *GetChunkIndex(uint_t index) {return chunklist[index];}

  /*--------------------------------------------------------------------------------*/
  /** Return chunk specified by chunk ID
   *
   * @param id 32-bit representation of chunk name (big endian)
   *
   * @return pointer to RIFFChunk object
   */
  /*--------------------------------------------------------------------------------*/
  RIFFChunk *GetChunk(uint32_t id) const;

  /*--------------------------------------------------------------------------------*/
  /** Create and add a chunk to a file being written
   *
   * @param id chunk type ID
   *
   * @return pointer to chunk object or NULL
   */
  /*--------------------------------------------------------------------------------*/
  RIFFChunk *AddChunk(uint32_t id);

  /*--------------------------------------------------------------------------------*/
  /** Create and add a chunk to a file being written
   *
   * @param name chunk type name
   *
   * @return pointer to chunk object or NULL
   */
  /*--------------------------------------------------------------------------------*/
  RIFFChunk *AddChunk(const char *name);

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
  RIFFChunk *AddChunk(uint32_t id, const uint8_t *data, uint64_t length, bool beforesamples = false);

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
  RIFFChunk *AddChunk(const char *name, const uint8_t *data, uint64_t length, bool beforesamples = false);

  /*--------------------------------------------------------------------------------*/
  /** Return chunk specified by chunk ID
   *
   * @param name chunk type name as a 4 character string
   *
   * @return pointer to RIFFChunk object
   */
  /*--------------------------------------------------------------------------------*/
  RIFFChunk *GetChunk(const char *name) {return GetChunk(IFFID(name));}

  /*--------------------------------------------------------------------------------*/
  /** Return SoundFileSamples object of file
   *
   * @return pointer to SoundFileSamples object
   */
  /*--------------------------------------------------------------------------------*/
  SoundFileSamples *GetSamples() const {return filesamples;}

  /*--------------------------------------------------------------------------------*/
  /** Read sample frames
   *
   * @param buffer destination buffer
   * @param type desired sample buffer format
   * @param nframes number of sample frames to read
   *
   * @note all channels must be read
   *
   * @return number of frames read or -1 for an error (no open file for example)
   */
  /*--------------------------------------------------------------------------------*/
  sint_t ReadSamples(uint8_t *buffer, SampleFormat_t type, uint_t dstchannel, uint_t ndstchannels, uint_t nframes) {return filesamples ? filesamples->ReadSamples((uint8_t *)buffer, type, dstchannel, ndstchannels, nframes) : -1;}
  sint_t ReadSamples(int16_t *buffer, uint_t dstchannel, uint_t ndstchannels, uint_t nframes = 1) {return ReadSamples((uint8_t *)buffer, SampleFormatOf(buffer), dstchannel, ndstchannels, nframes);}
  sint_t ReadSamples(int32_t *buffer, uint_t dstchannel, uint_t ndstchannels, uint_t nframes = 1) {return ReadSamples((uint8_t *)buffer, SampleFormatOf(buffer), dstchannel, ndstchannels, nframes);}
  sint_t ReadSamples(float   *buffer, uint_t dstchannel, uint_t ndstchannels, uint_t nframes = 1) {return ReadSamples((uint8_t *)buffer, SampleFormatOf(buffer), dstchannel, ndstchannels, nframes);}
  sint_t ReadSamples(double  *buffer, uint_t dstchannel, uint_t ndstchannels, uint_t nframes = 1) {return ReadSamples((uint8_t *)buffer, SampleFormatOf(buffer), dstchannel, ndstchannels, nframes);}

  /*--------------------------------------------------------------------------------*/
  /** Write sample frames
   *
   * @param buffer source buffer
   * @param type desired sample buffer format
   * @param nframes number of sample frames to write
   *
   * @note all channels must be written
   *
   * @return number of frames written or -1 for an error (no open file for example)
   */
  /*--------------------------------------------------------------------------------*/
  sint_t WriteSamples(const uint8_t *buffer, SampleFormat_t type, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1) {return filesamples ? filesamples->WriteSamples((const uint8_t *)buffer, type, srcchannel, nsrcchannels, nsrcframes) : -1;}
  sint_t WriteSamples(const int16_t *buffer, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1) {return WriteSamples((const uint8_t *)buffer, SampleFormatOf(buffer), srcchannel, nsrcchannels, nsrcframes);}
  sint_t WriteSamples(const int32_t *buffer, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1) {return WriteSamples((const uint8_t *)buffer, SampleFormatOf(buffer), srcchannel, nsrcchannels, nsrcframes);}
  sint_t WriteSamples(const float   *buffer, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1) {return WriteSamples((const uint8_t *)buffer, SampleFormatOf(buffer), srcchannel, nsrcchannels, nsrcframes);}
  sint_t WriteSamples(const double  *buffer, uint_t srcchannel, uint_t nsrcchannels, uint_t nsrcframes = 1) {return WriteSamples((const uint8_t *)buffer, SampleFormatOf(buffer), srcchannel, nsrcchannels, nsrcframes);}

protected:
  /*--------------------------------------------------------------------------------*/
  /** Read as many chunks as possible
   *
   * @param maxlength maximum number of bytes to read or skip over
   *
   * @return true if read chunks processed correctly
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool ReadChunks(uint64_t maxlength);

  /*--------------------------------------------------------------------------------*/
  /** Process a chunk (overridable)
   *
   * @param chunk pointer to chunk
   *
   * @return true if chunk processed correctly
   *
   * @note this is one of the mechanisms to handle of extra chunk types
   * @note but see PostReadChunks below as well
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool ProcessChunk(RIFFChunk *chunk) {UNUSED_PARAMETER(chunk); return true;}

  /*--------------------------------------------------------------------------------*/
  /** Post chunk reading processing (called after all chunks read)
   *
   * @return true if processing successful
   *
   * @note this is the other way of handling extra chunk types, especially if there
   * @note are dependencies between chunk types
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool PostReadChunks() {return true;}

  /*--------------------------------------------------------------------------------*/
  /** Optional stage to create extra chunks when writing WAV files
   */
  /*--------------------------------------------------------------------------------*/
  virtual bool CreateExtraChunks() {return true;}

  /*--------------------------------------------------------------------------------*/
  /** Write all chunks necessary
   *
   * @param closing true if file is closing and so chunks can be written after data chunk
   */
  /*--------------------------------------------------------------------------------*/
  virtual void WriteChunks(bool closing);

  /*--------------------------------------------------------------------------------*/
  /** Overrideable called whenever sample position changes
   */
  /*--------------------------------------------------------------------------------*/
  virtual void UpdateSamplePosition() {}

  typedef std::vector<RIFFChunk *>        ChunkList_t;
  typedef std::map<uint32_t, RIFFChunk *> ChunkMap_t;

protected:
  RefCount<EnhancedFile> fileref;
  uint8_t                filetype;
  SoundFormat            *fileformat;
  SoundFileSamples       *filesamples;
  ChunkList_t            chunklist;
  ChunkMap_t             chunkmap;
  bool                   writing;
  bool                   backgroundwriting;
};

BBC_AUDIOTOOLBOX_END

#endif
