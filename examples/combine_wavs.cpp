#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-adm/ADMData.h>
#include <bbcat-fileio/ADMRIFFFile.h>
#include <bbcat-fileio/TinyXMLADMData.h>
#include <bbcat-fileio/RIFFChunk_Definitions.h>
#include <bbcat-fileio/register.h>

#define STANDARD_DEF_FILE "StandardDefinitionsADM.xml"
#define SAMPLE_RATE 48000

using namespace bbcat;

typedef struct
{
  std::string fname;
  float start;
  std::string track_id;
  std::string ch_name;
  float az;
  float el;
  std::string sp_label;
} FILE_PARAMS;

void AddPackObject(ADMData *adm, ADMAudioPackFormat **packFormat, std::vector<ADMAudioTrackFormat *> &trackFormat_list, uint64_t duration);
void AddNewADM(ADMData *adm, std::vector<FILE_PARAMS> file_params,
              std::vector<uint_t> tracks_left, std::vector<ADMAudioTrackFormat *> &trackFormat_list);
void SearchForStandardDefs(ADMData *adm, std::vector<FILE_PARAMS> file_params,
                           std::vector<FILE_PARAMS> *file_params_left,
                           std::vector<uint_t> &tracks_left, ADMAudioPackFormat **packFormat, std::vector<ADMAudioTrackFormat *> &trackFormat_list);
uint_t ReadAllWAVFiles(const char *dir, std::vector<FILE_PARAMS> file_params, uint_t num_tracks, std::vector<Sample_t> *samples_out);
std::vector<Sample_t> ReadWAVFile(std::string fname);
std::vector<FILE_PARAMS> ReadCSVFile(const char *fname);
std::string RemoveQuotes(std::string s);


int main(int argc, const char *argv[])
{
  // ensure libraries are set up
  bbcat_register_bbcat_fileio();

  if (argc != 4)
  {
    fprintf(stderr, "combine_wavs <input csv> <input audio dir> <output file>\n");
    exit(-1);
  }
  
  // Read the csv file containing the info on the files to be combined
  std::vector<FILE_PARAMS> file_params = ReadCSVFile(argv[1]);
  
  uint_t num_tracks = file_params.size();
  
  for (uint_t i = 0; i < num_tracks; i++)
  {
    printf("%s\t", file_params[i].fname.c_str());
    printf("%f\t", file_params[i].start);
    printf("%s\t", file_params[i].track_id.c_str());
    printf("%s\t", file_params[i].ch_name.c_str());
    printf("%f\t", file_params[i].az);
    printf("%f\t", file_params[i].el);
    printf("%s\n", file_params[i].sp_label.c_str());
  }    
    
  // Read all the input WAV files
  std::vector<Sample_t> samples_out;
  uint_t max_len;
  max_len = ReadAllWAVFiles(argv[2], file_params, num_tracks, &samples_out);
  uint64_t duration = (uint64_t)max_len * 1000000000 / SAMPLE_RATE;   // Converts samples to nanoseconds
  
  ADMData *adm;
  
  // ADM aware WAV file
  ADMRIFFFile file_out;
  const char *filename = argv[3];
  std::vector<FILE_PARAMS> file_params_left;
  std::vector<uint_t> tracks_left;
   
  file_out.CreateADM();

  // Create an output file
  printf("num_tracks: %d\n", num_tracks);
  if (file_out.Create(filename, SAMPLE_RATE, num_tracks))
  {
    ADMAudioPackFormat *packFormat = NULL;
    std::vector<ADMAudioTrackFormat *> trackFormat_list;
   
    if ((adm = file_out.GetADM()) != NULL)
    {
      SearchForStandardDefs(adm, file_params, &file_params_left, tracks_left, &packFormat, trackFormat_list);
      
      AddNewADM(adm, file_params_left, tracks_left, trackFormat_list);
        
      AddPackObject(adm, &packFormat, trackFormat_list, duration);
    }
    else BBCERROR("GetADM fail\n");
  }  
  
  // Output block of audio
  file_out.WriteSamples(&samples_out[0], 0, file_out.GetChannels(), max_len);  

  file_out.Close(); 
    
}


/*--------------------------------------------------------------------------------*/
/** Add an audioPackFormat and audioObject to the ADM metadata
 *
 * @param adm - ADM object
 * @param packFormat - if a packFormat can be determined here, this is it
 * @param trackFormat_list - list of audioTrackFormats already allocated
 */
/*--------------------------------------------------------------------------------*/

void AddPackObject(ADMData *adm, ADMAudioPackFormat **packFormat, std::vector<ADMAudioTrackFormat *> &trackFormat_list, uint64_t duration)
{
  ADMAudioObject *object;
  
  object = adm->CreateObject("Main");     // create audio object for entire file
  object->SetStartTime(0);
  object->SetDuration(duration);
 
  printf("AddPackObject: duration=%llu\n", object->GetDuration());
 
  // If no packFormat exists then make one.
  if (!(*packFormat))
  {
    ADMData::OBJECTNAMES names;
    names.packFormatName = "MainPack";
    names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;
    adm->CreateObjects(names);
    *packFormat = names.objects.packFormat;

    // Add the tracks to the new packFormat
    for (uint_t k = 0; k < trackFormat_list.size(); k++)
    {
      ADMAudioStreamFormat *streamformat;
      ADMAudioChannelFormat *channelformat;
      streamformat = trackFormat_list[k]->GetStreamFormatRefs()[0];
      channelformat = streamformat->GetChannelFormatRefs()[0];
      (*packFormat)->Add(channelformat);
    }
  }

  // Add pack to object
  if ((*packFormat) && object)
  {
    object->Add(*packFormat);
  }

  // Add formats to tracks which will be inserted into chna
  const ADMData::TRACKLIST tracklist = adm->GetTrackList();
  for (uint_t k = 0; k < tracklist.size(); k++)
  {
    ADMAudioTrack *track;
    track = const_cast<ADMAudioTrack *>(tracklist[k]);
    track->Add(*packFormat);
    track->Add(trackFormat_list[k]);
    track->SetTrackNum(k);
    if (object) object->Add(track); 
  }
}

/*--------------------------------------------------------------------------------*/
/** Read non-standard definitions from the config file and make ADM metadata for them
 *
 * @param adm - ADM object
 * @param file_params - parameter for the remaining tracks in the file
 * @param tracks_left - table of track numbers not yet used
 * @param trackFormat_list - list of audioTrackFormats already allocated
 */
/*--------------------------------------------------------------------------------*/

void AddNewADM(ADMData *adm, std::vector<FILE_PARAMS> file_params,
               std::vector<uint_t> tracks_left, std::vector<ADMAudioTrackFormat *> &trackFormat_list)
{
  for (uint_t i = 0; i < file_params.size(); i++)
  {
    ADMData::OBJECTNAMES names;
    AudioObjectParameters params;
    Position pos;

    names.trackNumber = tracks_left[i] + 1;

    // derive channel and stream names from track name
    names.channelFormatName = file_params[i].ch_name;
    names.streamFormatName  = "PCM_" + file_params[i].ch_name;
    names.trackFormatName   = "PCM_" + file_params[i].ch_name;

    // set default typeLabel
    names.typeLabel = ADMObject::TypeLabel_DirectSpeakers;
    
    adm->CreateObjects(names);
    
    pos.polar  = true;
    pos.pos.az = file_params[i].az;
    pos.pos.el = file_params[i].el;
    pos.pos.d  = 1.0;
    params.SetPosition(pos);
    
    ADMAudioBlockFormat *blockformat = dynamic_cast<ADMAudioBlockFormat *>(adm->Create(ADMAudioBlockFormat::Type, "", "Block format 1"));
    blockformat->GetObjectParameters() = params;
    names.objects.channelFormat->Add(blockformat);

    trackFormat_list.push_back(names.objects.trackFormat);    
  }
}


/*--------------------------------------------------------------------------------*/
/** Search for matching packformats and channels in the standard defs and allocate
 * if found
 *
 * @param adm - ADM object
 * @param file_params - parameters for each track in the file
 * @param file_params_left - return parameters for each track not assigned here
 * @param tracks_left - return the track numbers not assigned here
 * @param packFormat - if a packFormat can be determined here, this is it
 * @param trackFormat_list - list of audioTrackFormats assigned here
 */
/*--------------------------------------------------------------------------------*/

void SearchForStandardDefs(ADMData *adm, std::vector<FILE_PARAMS> file_params,
                          std::vector<FILE_PARAMS> *file_params_left,
                          std::vector<uint_t> &tracks_left, ADMAudioPackFormat **packFormat, std::vector<ADMAudioTrackFormat *> &trackFormat_list)
{
  // attempt to find a single audioPackFormat from the standard definitions with the correct number of channels
  std::vector<const ADMObject *> packFormats, streamFormats, channelFormats;
  // *object = adm->CreateObject("Main");     // create audio object for entire file
  uint_t i;

  // get a list of pack formats - these will be searched for the pack format with the correct number of channels
  adm->GetObjects(ADMAudioPackFormat::Type, packFormats);

  // get a list of channel formats - these will be searched for the matching channels with trackss
  adm->GetObjects(ADMAudioChannelFormat::Type, channelFormats);

  // get a list of stream formats - these will be used to search for track formats and channel formats
  adm->GetObjects(ADMAudioStreamFormat::Type, streamFormats);

  std::vector<std::string> chid_list;
  std::vector<ADMAudioTrackFormat *> track_list;

  uint_t found_co = 0;  // Counter for the number of stddef tracks in input files
  std::vector<bool> remove_ind;
  for (uint_t j = 0; j < file_params.size(); j++)
  {
    remove_ind.push_back(false);
  }
  for (uint_t j = 0; j < file_params.size(); j++)   // This must be the outer loop to ensure track order is maintained.
  {
    for (uint_t k = 0; k < streamFormats.size(); k++)
    {
      ADMAudioStreamFormat *streamFormat;

      // stream format points to channel format and track format so look for stream format with the correct channel format ref
      if (((streamFormat = dynamic_cast<ADMAudioStreamFormat *>(const_cast<ADMObject *>(streamFormats[k]))) != NULL) &&
          streamFormat->GetChannelFormatRefs().size() &&
          streamFormat->GetTrackFormatRefs().size())                      // make sure there are some track formats ref'd as well
      {
        // get track format ref
        ADMAudioTrackFormat *trackFormat = streamFormat->GetTrackFormatRefs()[0];

        // Does input track match standard one?
        if (trackFormat->GetID() == file_params[j].track_id)
        {
          for (uint_t ch = 0; ch < channelFormats.size(); ch++)
          {
            ADMAudioChannelFormat *channelFormat;

            if (((streamFormat = dynamic_cast<ADMAudioStreamFormat *>(const_cast<ADMObject *>(streamFormats[k]))) != NULL) &&
                ((channelFormat = dynamic_cast<ADMAudioChannelFormat *>(const_cast<ADMObject *>(channelFormats[ch]))) != NULL) &&
              streamFormat->GetChannelFormatRefs().size() &&
              (streamFormat->GetChannelFormatRefs()[0] == channelFormat) &&   // check for correct channel format ref
              streamFormat->GetTrackFormatRefs().size())                      // make sure there are some track formats ref'd as well
            {
              found_co++;
              chid_list.push_back(channelFormat->GetID());
              track_list.push_back(trackFormat);
              trackFormat_list.push_back(trackFormat);
              remove_ind[j] = true;
            }
          }
        }
      }
    }
  }
  

  // Copy over all the file_params to file_params_left that haven't been removed.
  for (uint_t j = 0; j < file_params.size(); j++)
  {
    if (!remove_ind[j])
    {
      file_params_left->push_back(file_params[j]);
      tracks_left.push_back(j);
    }
  }
  
  (*packFormat) = NULL;
  if (file_params_left->size() == 0)   // All inputs are std defs, so looks for std def pack.
  {
    // search all pack formats
    for (i = 0; i < packFormats.size(); i++)
    {
      uint_t matched_ch = 0;

      if (((*packFormat) = dynamic_cast<ADMAudioPackFormat *>(const_cast<ADMObject *>(packFormats[i]))) != NULL)
      {
        // get channel format ref list - the size of this dictates the number of channels supported by the pack format
        const std::vector<ADMAudioChannelFormat *>& channelFormatRefs = (*packFormat)->GetChannelFormatRefs();

        // if the pack has the correct number of channels
        if (channelFormatRefs.size() == found_co)
        {
          for (uint_t j = 0; j < channelFormatRefs.size(); j++)
          {
            ADMAudioChannelFormat *channelFormat = channelFormatRefs[j];
            for (uint_t k = 0; k < chid_list.size(); k++)
            {
              if (chid_list[k] == channelFormat->GetID())
              {
                matched_ch++;
              }
            }
          }
        }
      }
      // If number of matched channels is same as in input file then valid pack exists.
      if (matched_ch == found_co)
      {
        break;
      }
    }
  }
}

        
/*--------------------------------------------------------------------------------*/
/** Read a CSV file and place contents in a vector of structs
 * CSV columns (each row for each output track):
 * 1 - Filename of wav file to be read
 * 2 - Position to place input in seconds
 * 3 - audioTrackFormat ID (only use for common definitions)
 * 4 - channel name (only use for customs defs)
 * 5 - azimuth (custom defs)
 * 6 - elevation (custom defs)
 * 7 - speaker label (custom defs)
 *
 * @param file_out - input CSV file 
 */
/*--------------------------------------------------------------------------------*/

std::vector<FILE_PARAMS> ReadCSVFile(const char *fname)
{
  std::vector<FILE_PARAMS> file_params;
  std::vector<std::string> valueline;
  std::ifstream fin(fname);   // Open CSV file
  std::string item;
  for (std::string line; getline(fin, line); )   // Read a line of CSV
  {
    std::istringstream in(line);
    FILE_PARAMS params;
  
    while (getline(in, item, ','))  // Read items in the CSV line
    {
      valueline.push_back(item.c_str());
    }
    // Fill up the params struct with values;
    if (valueline.size() == 6 || valueline.size() == 7)  // If the last comma hasn't got anything after it, it doesn't count it hence the 6.
    {
      params.fname = RemoveQuotes(valueline[0]);
      params.start = atof(valueline[1].c_str());
      params.track_id = RemoveQuotes(valueline[2]);
      params.ch_name = RemoveQuotes(valueline[3]);
      params.az = atof(valueline[4].c_str());
      params.el = atof(valueline[5].c_str());
      params.sp_label = RemoveQuotes(valueline[6]);
      file_params.push_back(params);
    }
    valueline.clear();
  }
  
  return file_params;
}


/*--------------------------------------------------------------------------------*/
/** Remote quotes from a string (only if they are at the start and end) 
 *
 * @param s - input string 
 */
/*--------------------------------------------------------------------------------*/


std::string RemoveQuotes(std::string s)
{
  std::string r(s);
  if (r.front() == '"')   // remove quotes if they are there
  {
    r.erase(0, 1); 
    r.erase(r.size() - 1); 
  }
  return r;
}


/*--------------------------------------------------------------------------------*/
/** Reads in all the WAV files and combines them into a multichannel vector 
 *
 * @param file_params - info about each input file
 * @param num_tracks - number of tracks in the output file
 * @samples_out - vector of the output samples
 */
/*--------------------------------------------------------------------------------*/

uint_t ReadAllWAVFiles(const char *dir, std::vector<FILE_PARAMS> file_params, uint_t num_tracks, std::vector<Sample_t> *samples_out)
{
  std::vector<std::vector<Sample_t> > samples;
  uint_t max_len = 0;  // Max end sample of all input files
  for (uint_t i = 0; i < num_tracks; i++)
  {
    std::string fname = std::string(dir); 
    fname = fname + "/" + file_params[i].fname;
    std::vector<Sample_t> fsamples = ReadWAVFile(fname);
    samples.push_back(fsamples);
    uint_t f_end = (uint_t)(file_params[i].start * (float)SAMPLE_RATE) + samples[i].size();    // Find the position of the last sample
    if (f_end > max_len) max_len = f_end;
  }
  
  uint_t len_out = max_len * num_tracks;
  samples_out->resize(len_out);
  
  for (uint_t i = 0; i < max_len; i++)
  {
    for (uint_t j = 0; j < num_tracks; j++)
    {
      uint_t f_start = (uint_t)(file_params[j].start * (float)SAMPLE_RATE);
      uint_t f_end = f_start + samples[j].size();

      if (i >= f_start && i < f_end)
      {
        uint_t k = i - f_start;
        (*samples_out)[(i * num_tracks) + j] = samples[j][k];
      }
    }
  }
  printf("samples_out: %d\n", (int)samples_out->size());
  
  return max_len;
}


/*--------------------------------------------------------------------------------*/
/** Read a plain WAV file and put the samples in a vector
 *
 * @param fname - filename 
 */
/*--------------------------------------------------------------------------------*/

std::vector<Sample_t> ReadWAVFile(std::string fname)
{
  // non-ADM aware WAV file
  RIFFFile file_in;
  std::vector<Sample_t> samples;

  if (file_in.Open(fname.c_str()))
  {
    SoundFileSamples *handler = file_in.GetSamples();
    std::vector<Sample_t> buf_in;

    uint_t n, nsamples = 1024;
    
    // make buffer for 1024 frames worth of samples
    buf_in.resize(handler->GetChannels() * nsamples);
    samples.resize(handler->GetChannels() * file_in.GetSampleLength());

    while ((n = handler->ReadSamples(&buf_in[0], 0, handler->GetChannels(), nsamples)) > 0)
    {
      //printf("%u/%u sample frames read, sample position %s/%s\n", n, nsamples, StringFrom(handler->GetSamplePosition()).c_str(), StringFrom(handler->GetSampleLength()).c_str());
      for (uint_t i = 0; i < n; i++)
      {
        samples[(ulong_t)handler->GetSamplePosition() - nsamples + i] = buf_in[i];
      }
    }
  }
  
  return samples;
}

