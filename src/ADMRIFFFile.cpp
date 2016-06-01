
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <string>

#define BBCDEBUG_LEVEL 1
#include <bbcat-base/PerformanceMonitor.h>

#include "ADMRIFFFile.h"
#include "RIFFChunk_Definitions.h"

BBC_AUDIOTOOLBOX_START

ADMRIFFFile::ADMRIFFFile() : RIFFFile(),
                             adm(NULL)
{
}

ADMRIFFFile::~ADMRIFFFile()
{
  Close();
}

/*--------------------------------------------------------------------------------*/
/** Open a WAVE/RIFF file
 *
 * @param filename filename of file to open
 * @param standarddefinitionsfile filename of standard definitions XML file to use
 *
 * @return true if file opened and interpreted correctly (including any extra chunks if present)
 */
/*--------------------------------------------------------------------------------*/
bool ADMRIFFFile::Open(const char *filename, const std::string& standarddefinitionsfile)
{
  bool success = false;

  if ((adm = XMLADMData::CreateADM(standarddefinitionsfile)) != NULL)
  {
    success = RIFFFile::Open(filename);
  }
  else BBCERROR("No providers for ADM XML decoding!");

  return success;
}


/*--------------------------------------------------------------------------------*/
/** Optional stage to create extra chunks when writing WAV files
 */
/*--------------------------------------------------------------------------------*/
bool ADMRIFFFile::CreateExtraChunks()
{
  bool success = true;

  if (adm)
  {
    RIFFChunk *chunk;
    uint64_t  chnalen;
    uint8_t   *chna;
    uint_t i, nchannels = GetChannels();

    success = true;

    for (i = 0; i < nchannels; i++)
    {
      ADMAudioTrack *track;

      // create chna track data
      if ((track = adm->CreateTrack(i)) != NULL)
      {
        track->SetSampleRate(GetSampleRate());
        track->SetBitDepth(GetBitsPerSample());
      }
    }

    if (!admfile.empty())
    {
      // create ADM structure (content and objects from file)
      if (adm->CreateFromFile(admfile.c_str()))
      {
        // can prepare cursors now since all objects have been created
        PrepareCursors();
      }
      else
      {
        BBCERROR("Unable to create ADM structure from '%s'", admfile.c_str());
        success = false;
      }
    }

    // TODO: this cannot be expanded upon!
    
    // get ADM object to create chna chunk
    if ((chna = adm->GetChna(chnalen)) != NULL)
    {
      // and add it to the RIFF file
      if ((chunk = AddChunk(chna_ID)) != NULL)
      {
        success &= chunk->CreateChunkData(chna, chnalen);
      }
      else BBCERROR("Failed to add chna chunk");

      // don't need the raw data any more
      free(chna);
    }
    else BBCERROR("No chna data available");

    success &= (AddChunk(axml_ID) != NULL);
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Create empty ADM and populate basic track information
 *
 * @param standarddefinitionsfile filename of standard definitions XML file to use
 *
 * @return true if successful
 */
/*--------------------------------------------------------------------------------*/
bool ADMRIFFFile::CreateADM(const std::string& standarddefinitionsfile)
{
  bool success = false;

  if (!adm)
  {
    if ((adm = XMLADMData::CreateADM(standarddefinitionsfile)) != NULL)
    {
      success = true;
    }
    else BBCERROR("No providers for ADM XML decoding!");
  }

  return success;
}

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
bool ADMRIFFFile::CreateADMFromFile(const std::string& filename, const std::string& standarddefinitionsfile)
{
  bool success = false;

  if (CreateADM(standarddefinitionsfile))
  {
    // save filename until extra chunks are created
    admfile = filename;

    success = true;
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Close RIFF file, writing chunks if file was opened for writing
 *
 * @param abortwrite true to abort the writing of file
 *
 * @note this may take some time because it copies sample data from a temporary file
 */
/*--------------------------------------------------------------------------------*/
void ADMRIFFFile::Close(bool abortwrite)
{
  EnhancedFile *file = fileref;
  uint_t i;

  if (file && adm && writing && !abortwrite)
  {
    RIFFChunk *chunk;
    uint64_t  endtime = filesamples ? filesamples->GetAbsolutePositionNS() : 0;
    uint64_t  chnalen;
    uint8_t   *chna;

    BBCDEBUG1(("Finalising ADM for '%s'...", file->getfilename().c_str()));

    BBCDEBUG1(("Finishing all blockformats"));

    // complete BlockFormats on all channels
    for (i = 0; i < cursors.size(); i++)
    {
      cursors[i]->Seek(endtime);
      cursors[i]->EndChanges();
    }

    // finalise ADM
    adm->Finalise();

    // update audio object time limits
    adm->UpdateAudioObjectLimits();

    BBCDEBUG1(("Creating ADM RIFF chunks"));

    // get ADM object to create chna chunk
    if ((chna = adm->GetChna(chnalen)) != NULL)
    {
      // and add it to the RIFF file
      if ((chunk = GetChunk(chna_ID)) != NULL)
      {
        if (!chunk->UpdateChunkData(chna, chnalen)) BBCERROR("Failed to update chna data (possibly length has changed)");
      }
      else BBCERROR("Failed to add chna chunk");

      // don't need the raw data any more
      delete[] chna;
    }
    else BBCERROR("No chna data available");

    // add axml chunk
    if ((chunk = GetChunk(axml_ID)) != NULL)
    {
      // first, calculate size of ADM (to save lots of memory allocations)
      uint64_t admlen = adm->GetAxmlBuffer(NULL, 0);

      BBCDEBUG1(("ADM size is %s bytes", StringFrom(admlen).c_str()));
      
      // allocate chunk data
      if (chunk->CreateChunkData(admlen))
      {
        // finally, generate XML into buffer
        uint64_t admlen1 = adm->GetAxmlBuffer(chunk->GetDataWritable(), admlen);
        if (admlen1 != admlen) BBCERROR("Generating axml data for real resulted in different size (%s vs %s)", StringFrom(admlen1).c_str(), StringFrom(admlen).c_str());
      }
      else BBCERROR("Failed to allocate %s bytes for axml data", StringFrom(admlen).c_str());
    }
    else BBCERROR("Failed to add axml chunk");
  }

  // write chunks and close file
  RIFFFile::Close(abortwrite);

  for (i = 0; i < cursors.size(); i++)
  {
    delete cursors[i];
  }
  cursors.clear();

  if (adm)
  {
    adm->Delete();
    delete adm;
    adm = NULL;
  }
}

/*--------------------------------------------------------------------------------*/
/** Create cursors and add all objects to each cursor
 *
 * @note this can be called prior to writing samples or setting positions but it
 * @note *will* be called by SetPositions() if not done so already
 */
/*--------------------------------------------------------------------------------*/
void ADMRIFFFile::PrepareCursors()
{
  if (adm && writing && !cursors.size())
  {
    std::vector<const ADMAudioObject *> objects;
    ADMTrackCursor *cursor;
    uint_t i, nchannels = GetChannels();

    // get list of ADMAudioObjects
    adm->GetAudioObjectList(objects);

    // add all objects to all cursors
    for (i = 0; i < nchannels; i++)
    {
      // create track cursor for tracking position during writing
      if ((cursor = new ADMTrackCursor(i)) != NULL)
      {
        cursor->Add(objects);
        cursors.push_back(cursor);
      }
    }
  }
}

bool ADMRIFFFile::PostReadChunks()
{
  bool success = RIFFFile::PostReadChunks();

  // after reading of chunks, find chna and axml chunks and decode them
  // to create an ADM
  if (success)
  {
    RIFFChunk *chna = GetChunk(chna_ID);
    RIFFChunk *axml = GetChunk(axml_ID);

    // ensure each chunk is valid
    if (adm &&
        chna && chna->GetData() &&
        axml && axml->GetData())
    {
      // decode chunks
      success = adm->Set(chna->GetData(), chna->GetLength(), (const char *)axml->GetData());

#if BBCDEBUG_LEVEL >= 4
      { // dump ADM as text
        std::string str;
        adm->Dump(str);

        BBCDEBUG("%s", str.c_str());
      }

      { // dump ADM as XML
        std::string str;
        adm->GetAxml(str);

        BBCDEBUG("%s", str.c_str());
      }

      BBCDEBUG("Audio objects:");
      std::vector<const ADMObject *> list;
      adm->GetObjects(ADMAudioObject::Type, list);
      uint_t i;
      for (i = 0; i < list.size(); i++)
      {
        BBCDEBUG("%s", list[i]->ToString().c_str());
      }
#endif
    }
    // test for different types of failure
    else if (!adm)
    {
      BBCERROR("Cannot decode ADM, no ADM decoder available");
      success = false;
    }
    else if (!chna || !axml)
    {
      // acceptable failure - chna and/or axml chunk not specified - not an ADM compatible BWF file but open anyway
      BBCDEBUG("Warning: no chna/axml chunks!");

      // if no chna supplied, create default channel mapping using standard definitions
      if (adm && !chna)
      {
        // attempt to find a single audioPackFormat from the standard definitions with the correct number of channels
        std::vector<const ADMObject *> packFormats, streamFormats;
        ADMAudioObject *object = adm->CreateObject("Main");     // create audio object for entire file
        uint_t i;

        // get a list of pack formats - these will be searched for the pack format with the correct number of channels
        adm->GetObjects(ADMAudioPackFormat::Type, packFormats);

        // get a list of stream formats - these will be used to search for track formats and channel formats
        adm->GetObjects(ADMAudioStreamFormat::Type, streamFormats);

        // search all pack formats
        for (i = 0; i < packFormats.size(); i++)
        {
          ADMAudioPackFormat *packFormat;

          if ((packFormat = dynamic_cast<ADMAudioPackFormat *>(const_cast<ADMObject *>(packFormats[i]))) != NULL)
          {
            // get channel format ref list - the size of this dictates the number of channels supported by the pack format
            const std::vector<ADMAudioChannelFormat *>& channelFormatRefs = packFormat->GetChannelFormatRefs();

            // if the pack has the correct number of channels
            if (channelFormatRefs.size() == GetChannels())
            {
              uint_t j;

              BBCDEBUG("Found pack format '%s' ('%s') for %u channels", packFormat->GetName().c_str(), packFormat->GetID().c_str(), GetChannels());

              // add pack format to audio object
              if (object) object->Add(packFormat);

              // for each channel, create a track and link pack format to each
              for (j = 0; j < channelFormatRefs.size(); j++)
              {
                ADMAudioChannelFormat *channelFormat = channelFormatRefs[j];
                ADMAudioTrack *track;
                std::string name;

                // create track
                if ((track = adm->CreateTrack(j)) != NULL)
                {
                  uint_t k;

                  track->Add(packFormat);

                  // find stream format that points to the correct channel format and use that to find the trackFormat
                  for (k = 0; k < streamFormats.size(); k++)
                  {
                    ADMAudioStreamFormat *streamFormat;

                    // stream format points to channel format and track format so look for stream format with the correct channel format ref
                    if (((streamFormat = dynamic_cast<ADMAudioStreamFormat *>(const_cast<ADMObject *>(streamFormats[k]))) != NULL) &&
                        streamFormat->GetChannelFormatRefs().size() &&
                        (streamFormat->GetChannelFormatRefs()[0] == channelFormat) &&   // check for correct channel format ref
                        streamFormat->GetTrackFormatRefs().size())                      // make sure there are some track formats ref'd as well
                    {
                      // get track format ref
                      ADMAudioTrackFormat *trackFormat = streamFormat->GetTrackFormatRefs()[0];

                      BBCDEBUG("Found stream format '%s' ('%s') which refs channel format '%s' ('%s')", streamFormat->GetName().c_str(), streamFormat->GetID().c_str(), channelFormat->GetName().c_str(), channelFormat->GetID().c_str());
                      BBCDEBUG("Found stream format '%s' ('%s') which refs track   format '%s' ('%s')", streamFormat->GetName().c_str(), streamFormat->GetID().c_str(), trackFormat->GetName().c_str(), trackFormat->GetID().c_str());

                      // add track format to track
                      track->Add(trackFormat);
                      break;
                    }
                  }

                  // add track to audio object
                  if (object) object->Add(track);
                }
              }
              break;
            }
          }
        }

        // set default time limits on audio object
        if (object && filesamples)
        {
          BBCDEBUG("Setting duration to %sns", StringFrom(filesamples->GetLengthNS()).c_str());
          object->SetDuration(filesamples->GetLengthNS());
        }
      }

      success = true;
    }
    else {
      // unacceptible failures: empty chna or empty axml chunks
      if (chna && !chna->GetData()) BBCERROR("Cannot decode ADM, chna chunk not available");
      if (axml && !axml->GetData()) BBCERROR("Cannot decode ADM, axml chunk not available");
      success = false;
    }

    // now that the data is dealt with, the chunk data can be deleted
    if (axml) axml->DeleteData();
    if (chna) chna->DeleteData();
  }

  return success;
}

/*--------------------------------------------------------------------------------*/
/** Update current position within the file
 */
/*--------------------------------------------------------------------------------*/
void ADMRIFFFile::UpdateSamplePosition()
{
}
  
/*--------------------------------------------------------------------------------*/
/** Set parameters of channel during writing
 *
 * @param channel channel to change the position of
 * @param objparameters object parameters
 */
/*--------------------------------------------------------------------------------*/
void ADMRIFFFile::SetObjectParameters(uint_t channel, const AudioObjectParameters& objparameters)
{
  // create cursors if necessary and add all objects to them
  PrepareCursors();

  if (writing && (channel < cursors.size()))
  {
    PERFMON("Write ADM Channel Parameters");
    uint64_t t = filesamples ? filesamples->GetAbsolutePositionNS() : 0;

    cursors[channel]->Seek(t);
    cursors[channel]->SetObjectParameters(objparameters);
  }
}

BBC_AUDIOTOOLBOX_END
