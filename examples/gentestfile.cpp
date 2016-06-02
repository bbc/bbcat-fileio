
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bbcat-base/LoadedVersions.h>

#include <bbcat-fileio/ADMRIFFFile.h>
#include <bbcat-fileio/ADMAudioFileSamples.h>
#include <bbcat-fileio/register.h>

using namespace bbcat;

typedef struct
{
  uint64_t t;   // ns time
  uint64_t pos; // sample position
  AudioObjectParameters objparameters;
} METADATAEVENT;

/*--------------------------------------------------------------------------------*/
/** Main entry point
 */
/*--------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  // ensure libraries are set up
  bbcat_register_bbcat_fileio();

  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s <input-wav-file> <input-csv-file> <output-adm-wav-file>\n\n", argv[0]);
    fprintf(stderr, "<input-csv-file> contains lines of the following format:\n");
    fprintf(stderr, "[<time>],<channel>[,'{<json-data>}'][,<coord-type>,<coord-1>,<coord-2>,<coord-3>[,<gain>[,<jump>[,<interpolation-time-s>]]]]\n\n");
    fprintf(stderr, "Where:\n");
    fprintf(stderr, "\t<time>\t\tTime of change in [+][[<hh>:]<mm>:]<ss>[.<SSSSS>] format, preceeding by '+' provides increment from previous value\n");
    fprintf(stderr, "\t<channel>\tChannel number (1-based)\n");
    fprintf(stderr, "\t<coord-type>\tCo-ordinate type 'cartesian' / 'cart' or 'spherical' / 'polar'.  Empty means spherical.  Spherical uses degrees.\n");
    fprintf(stderr, "\t<coord-<n>>\tCo-ordinate values, either x, y and z or azimuth, elevation and radius depending upon the above\n");
    fprintf(stderr, "\t<gain>\t\tOptional *linear* gain value\n");
    fprintf(stderr, "\t<jump>\t\tOptional 0 or 1 indicating whether the coefficients should jump (actually interpolated very quickly) or take the entire block to change\n");
    fprintf(stderr, "\t<interpolation-time-s> Optional time in seconds to interpolate over (ignored if <jump> = 0), defaults to 0\n");
    fprintf(stderr, "\t<json-data>\tOptional multiple JSON type entries (e.g. '{\"gain\":.001,\"channellock\":false}')\n");
    exit(1);
  }

  RIFFFile     inputfile;       // non-ADM aware WAV file
  ADMRIFFFile  outputfile;      // ADM aware WAV file
  EnhancedFile csvfile;         // CSV file containing positions

  if (inputfile.Open(argv[1]))
  {
    const uint32_t samplerate = inputfile.GetSampleRate();
    const uint_t   nchannels  = inputfile.GetChannels();
    
    if (csvfile.fopen(argv[2])) {
      // IMPORTANT: create basic ADM here - if this is not done, the file will be a plain WAV file!
      outputfile.CreateADM();

      // create output WAV file
      if (outputfile.Create(argv[3], samplerate, nchannels))
      {
        SoundFileSamples           *handler = outputfile.GetSamples();
        const UniversalTime&       timebase = handler->GetTimeBase();
        const uint_t               nframes  = 1024;
        std::vector<sint32_t>      audio(nframes * nchannels);
        std::vector<METADATAEVENT> events;
        bool success = true;
        
        ADMData *adm = outputfile.GetADM();
        if (adm)
        {
          static char buffer[1024];
          int  l;

          // create ADM structures and create an event list of metadata changes
          {
            ADMData::OBJECTNAMES names;
            
            // set programme name
            // if an audioProgramme object of this name doesn't exist, one will be created
            names.programmeName = "ADM WAV Test File";

            // set content name
            // if an audioContent object of this name doesn't exist, one will be created
            names.contentName   = "Audio source " + std::string(argv[1]) + ", metadata source " + std::string(argv[2]);

            // set object name
            // create a single object for all channels
            names.objectName = "Test Object";

            // set pack name from object name
            // create a single object
            names.packFormatName = "Pack 1";

            // set default typeLabel
            names.typeLabel = ADMObject::TypeLabel_Objects;

            {
              // create tracks, channels and streams
              uint_t tr;
              const ADMData::TRACKLIST& tracklist = adm->GetTrackList();
              for (tr = 0; tr < tracklist.size(); tr++)
              {
                std::string trackname;

                printf("------------- Track %2u -----------------\n", tr + 1);

                // create default audioTrackFormat name (used for audioStreamFormat objects as well)
                Printf(trackname, "Track %u", tr + 1);

                names.trackNumber = tr;

                // derive channel and stream names from track name
                names.channelFormatName = trackname;
                names.streamFormatName  = "PCM_" + trackname;
                names.trackFormatName   = "PCM_" + trackname;

                // create all necessary objects as required
                adm->CreateObjects(names);
              }
            }
          }

          // read lines from CSV file
          uint64_t t = 0;
          while (success && ((l = csvfile.readline(buffer, sizeof(buffer))) != EOF))
          {
            std::vector<std::string> columns;
            std::string line = buffer;
            size_t p;
            uint_t n;

            // strip anything beyond a ';' which can be used for comments
            if ((p = line.find(";")) < std::string::npos) line = line.substr(0, p);
            
            // split line into columns (keeping empty strings)
            SplitString(buffer, columns, ',', true);

            // check that there are enough columns
            if ((n = (uint_t)columns.size()) >= 2)
            {
              METADATAEVENT event;
              uint64_t inc;
              uint_t   i, col = 0;

              // extract event time
              if ((columns[col][0] == '+') && CalcTime(inc, columns[col].substr(1)))
              {
                col++;
                t += inc;
              }
              else if (columns[col].empty() || CalcTime(t, columns[col]))
              {
                col++;
              }
              else
              {
                fprintf(stderr, "Unable to evaluate time from '%s'\n", columns[col].c_str());
                success = false;
              }

              if (success)
              {
                event.t   = t;
                event.pos = timebase.Invert(event.t);    // calculate sample position of event
              }
              
              // extract channel number
              uint_t channel = 0;
              if (success && Evaluate(columns[col], channel))
              {
                col++;
                if ((channel > 0) && (channel <= nchannels))
                {
                  // channel specified as 1-based -> convert to 0-based
                  channel--;
                  event.objparameters.SetChannel(channel);
                }
                else
                {
                  fprintf(stderr, "Channel number %u out of range (should be 1..%u)\n", channel, nchannels);
                  success = false;
                }
              }
              else
              {
                fprintf(stderr, "Unable to evaluate channel number from '%s'\n", columns[col].c_str());
                success = false;
              }

              // look for any explicitly specified JSON columns
              for (i = col; success && (i < (uint_t)columns.size()); i++)
              {
                const std::string& str = columns[i];
                
                if ((str.length() >= 2) && (str[0] == '{') && (str[str.length() - 1] == '}'))
                {
                  // assume this column to be JSON
                  JSONValue obj;

                  if (json::FromJSONString(str, obj) && obj.isObject())
                  {
                    // do NOT reset other values to allow multiple, modifying entries
                    event.objparameters.FromJSON(obj, false);
                  }
                  else
                  {
                    fprintf(stderr, "Failed to decode JSON string '%s'\n", str.c_str());
                    success = false;
                  }
                  
                  // remove string from column list
                  columns.erase(columns.begin() + i);
                }
              }

              // need 4 more columns for position
              if (success && ((col + 4) <= (uint_t)columns.size()))
              {
                // extract co-ordinate system
                Position pos;
                if (success)
                {
                  if ((columns[col] == "cart") || (columns[col] == "cartesian"))
                  {
                    pos.polar = false;
                    col++;
                  }
                  else if ((columns[col] == "spherical") || (columns[col] == "polar") || (columns[col] == ""))
                  {
                    pos.polar = true;
                    col++;
                  }
                  else
                  {
                    fprintf(stderr, "Invalid co-ordinate type specifier '%s'\n", columns[col].c_str());
                    success = false;
                  }
                }

                // extract co-ordinates
                if (success)
                {
                  if (Evaluate(columns[col],     pos.pos.elements[0]) &&
                      Evaluate(columns[col + 1], pos.pos.elements[1]) &&
                      Evaluate(columns[col + 2], pos.pos.elements[2]))
                  {
                    col += 3;
                
                    event.objparameters.SetPosition(pos);
                  }
                  else
                  {
                    fprintf(stderr, "Unable to evaluate at least one of the co-ordinates ('%s', '%s' and '%s')\n", columns[col].c_str(), columns[col + 1].c_str(), columns[col + 2].c_str());
                    success = false;
                  }
                }
              }
              
              // look for optional <gain> parameter
              if (success && (col < (uint_t)columns.size()))
              {
                double gain = 1.0;
                if (Evaluate(columns[col], gain))
                {
                  event.objparameters.SetGain(gain);
                  col++;
                }
                else
                {
                  fprintf(stderr, "Unable to evaluate gain parameter from '%s'\n", columns[col].c_str());
                  success = false;
                }
              }

              {
                // look for optional <jump> parameter
                bool jump = false;
                if (success && (col < (uint_t)columns.size()))
                {
                  if (Evaluate(columns[col], jump)) col++;
                  else
                  {
                    fprintf(stderr, "Unable to evaluate jump parameter from '%s'\n", columns[col].c_str());
                    success = false;
                  }
                }
              
                // look for optional <interpolation-time-s> parameter
                double interpolationtime = 0.0;
                if (success && (col < (uint_t)columns.size()))
                {
                  if (Evaluate(columns[col], interpolationtime)) col++;
                  else
                  {
                    fprintf(stderr, "Unable to evaluate interpolation time from '%s'\n", columns[col].c_str());
                    success = false;
                  }
                }
              
                if (success)
                {
                  // if <jump> specified as 1, set jumpPosition and interpolationLength accordingly
                  if (jump) event.objparameters.SetJumpPosition(jump, interpolationtime);
                }
              }

              if (success)
              {
                //printf("Adding event %s at %sns (%s)\n", event.objparameters.ToString().c_str(), StringFrom(event.t).c_str(), StringFrom(event.pos).c_str());

                // add event to list of events
                events.push_back(event);
              }
            }
            else if (n > 0)
            {
              fprintf(stderr, "Line '%s' needs at least 2 columns (found %u)\n", buffer, n);
              success = false;
            }
          }
        }
        else
        {
          fprintf(stderr, "No ADM associated with created ADM WAV file, output will be plain WAV file!\n");
          success = false;
        }

        if (success)
        {
          // copy samples
          uint_t evindex = 0; // index into event list
          while (true)
          {
            uint_t n = nframes, n1;

            // calculate number of samples than need to be copied *before* next event is due to happen
            if (evindex < events.size())
            {
              uint64_t nsamples  = limited::subz(events[evindex].pos, handler->GetSamplePosition());    // calculate number of samples from now until then

              // number of sample frames to transfer this round
              n = (uint_t)std::min((uint64_t)n, nsamples);
            }

            // copy enough samples to get to current event
            if (n)
            {
              if ((n1 = inputfile.ReadSamples(&audio[0], 0, nchannels, n)) > 0)
              {
                outputfile.WriteSamples(&audio[0], 0, nchannels, n1);
              }
              else break;
            }

            // process all events that are at this position
            while ((evindex < events.size()) && (handler->GetSamplePosition() >= events[evindex].pos))
            {
              const AudioObjectParameters& objparameters = events[evindex].objparameters;

              // set ADM parameters
              outputfile.SetObjectParameters(objparameters.GetChannel(), objparameters);

              // move onto next event
              evindex++;
            }
          }
        }
        
        // finalise ADM and close file
        outputfile.Close();

        if (!success) remove(argv[3]);
      }
      else fprintf(stderr, "Failed to create ADM WAV file '%s'\n", argv[3]);

      csvfile.fclose();
    }
    else fprintf(stderr, "Failed to open CSV file '%s' for reading\n", argv[2]);

    inputfile.Close();
  }
  else fprintf(stderr, "Failed to open WAV file '%s' for reading\n", argv[1]);
  
  return 0;
}
