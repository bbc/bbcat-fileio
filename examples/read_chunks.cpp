// Outputs the axml and chna chunks into separate output files from a given input BWF file 

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <bbcat-fileio/ADMRIFFFile.h>
#include <bbcat-fileio/ADMAudioFileSamples.h>
#include <bbcat-fileio/RIFFChunk_Definitions.h>
#include <bbcat-base/EnhancedFile.h>

using namespace bbcat;

void PrintFixed(EnhancedFile *file, std::string s, size_t len);


int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    fprintf(stderr, "Usage: read-chunks <bwf-file> <output xml> <output chna>\n");
    exit(1);
  }
  
  RIFFFile file;

  // Open input wav file
  if (file.Open(argv[1]))
  {
    printf("Opened '%s' okay, %u channels at %luHz (%u bytes per sample)\n", argv[1], file.GetChannels(), (ulong_t)file.GetSampleRate(), (uint_t)file.GetBytesPerSample());
    
    RIFFChunk *axml_chunk, *chna_chunk;
    
    // Read axml chunk if it exists
    if ((axml_chunk = file.GetChunk(axml_ID)) != NULL)
    {
      printf("axml chunk exists\n");
      
      // Send out xml to a file
      EnhancedFile axml_file;
      if (axml_file.fopen(argv[2], "wb"))
      {
        axml_file.fwrite((void *)axml_chunk->GetData(), sizeof(uint8_t), axml_chunk->GetLength());
        axml_file.fclose();
      }
    }
    // Read chna chunk if it exists
    if ((chna_chunk = file.GetChunk(chna_ID)) != NULL)
    {
      printf("chna chunk exists.\n");
      
      // Write out chna info to a file
      EnhancedFile chna_file;
      if (chna_file.fopen(argv[3], "wb"))
      {
        const CHNA_CHUNK& chna = *(const CHNA_CHUNK *)chna_chunk->GetData();
        uint_t maxuids = (chna_chunk->GetLength() - sizeof(CHNA_CHUNK)) / sizeof(chna.UIDs[0]);   // calculate maximum number of UIDs given chunk length

        if (maxuids < chna.UIDCount) BBCERROR("Warning: chna specifies %u UIDs but chunk has only length for %u", (uint_t)chna.UIDCount, maxuids);

        uint16_t i;
        for (i = 0; (i < chna.UIDCount) && (i < maxuids); i++)
        {
          // only handle non-zero track numbers
          if (chna.UIDs[i].TrackNum)
          {
            std::string id, track, pack;

            // Convert fields to strings
            id.assign(chna.UIDs[i].UID, sizeof(chna.UIDs[i].UID));
            track.assign(chna.UIDs[i].TrackRef, sizeof(chna.UIDs[i].TrackRef));
            pack.assign(chna.UIDs[i].PackRef, sizeof(chna.UIDs[i].PackRef));
            
            // Prints strings to the file
            chna_file.fprintf("%02d ", chna.UIDs[i].TrackNum);
            PrintFixed(&chna_file, id, sizeof(chna.UIDs[i].UID));
            PrintFixed(&chna_file, track, sizeof(chna.UIDs[i].TrackRef));
            PrintFixed(&chna_file, pack, sizeof(chna.UIDs[i].PackRef));
            chna_file.fprintf("\n");
          }
        }
    
        chna_file.fclose();
      }
      
    }
    file.Close();
    
  }
  else fprintf(stderr, "Failed to open file '%s' for reading!\n", argv[1]);

  return 0;
}


/*--------------------------------------------------------------------------------*/
/** Print a string in a tidy way with a separator 
 *
 * @param file - file handler
 * @param s - input string
 * @param len - length of the string to be printed
 *
 */
/*--------------------------------------------------------------------------------*/

void PrintFixed(EnhancedFile *file, std::string s, size_t len)
{
  for (uint_t j = 0; j < len; j++)
  {
    if (s.c_str()[j] >= 32) file->fprintf("%c", s.c_str()[j]);
    else file->fprintf(".");
  }
  file->fprintf(" ");
}
