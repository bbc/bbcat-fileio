// Reads in plain WAV file, plus an XML file and chna file, and combines them to output a BWF file

#include <stdio.h>
#include <stdlib.h>

#include <bbcat-fileio/ADMRIFFFile.h>

using namespace bbcat;

int main(int argc, const char *argv[])
{
  if (argc != 5)
  {
    fprintf(stderr, "Usage: wav2bwav <wav-file-in> <xml-in> <chna-in> <bwav-file-out>\n");
    exit(1);
  }

  RIFFFile file_in;
  ADMRIFFFile file_out;
  std::vector<Sample_t> samples;

  // Open input file
  if (file_in.Open(argv[1]))
  {
    XMLADMData *adm;
    if ((adm = XMLADMData::CreateADM()) != NULL)
    {
      if (!adm->ReadXMLFromFile(argv[2])) fprintf(stderr, "Failed to read XML from '%s'\n", argv[2]);
      if (!adm->ReadChnaFromFile(argv[3])) fprintf(stderr, "Failed to read Chna from '%s''\n", argv[3]);

      SoundFileSamples *handler = file_in.GetSamples();
      std::vector<Sample_t> buf_in;
      ulong_t j = 0;
      uint_t n, nsamples = 1024;

      // make buffer for 1024 frames worth of samples
      buf_in.resize(handler->GetChannels() * nsamples);
      ulong_t ssize = file_in.GetSampleLength() * handler->GetChannels();
      samples.resize(ssize);
   
      // Read samples into 'samples' buffer
      while ((n = handler->ReadSamples(&buf_in[0], 0, handler->GetChannels(), nsamples)) > 0)
      {
        for (uint_t i = 0; i < n * handler->GetChannels(); i++)
        {
          samples[j++] = buf_in[i];   
        }
      }
      ulong_t max_len = j;
          
      // Open output file
      if (file_out.Create(argv[4], file_in.GetSampleRate(), file_in.GetChannels()))
      {
        uint64_t len;
        const uint8_t *data;
        std::string axml = adm->GetAxml();

        // Write out samples
        file_out.WriteSamples(&samples[0], 0, file_out.GetChannels(), max_len / file_out.GetChannels());  

        // add chna chunk
        if ((data = adm->GetChna(len)) != NULL) file_out.AddChunk("chna", data, len);
        else fprintf(stderr, "No chna chunk to add!\n");

        // add axml chunk
        file_out.AddChunk("axml", (const uint8_t *)axml.c_str(), axml.size());

        // write everything
        file_out.Close();        
      }
      else fprintf(stderr, "Failed to open output file\n");

      delete adm;
    }
    else fprintf(stderr, "Failed to create blank ADM!\n");
  }
  else fprintf(stderr, "Failed to open input file\n");
   
 
 
  return 0;
}
