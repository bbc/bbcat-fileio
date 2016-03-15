#ifndef __RIFF_CHUNK_DEFINITIONS__
#define __RIFF_CHUNK_DEFINITIONS__

#include <bbcat-base/misc.h>

#ifdef COMPILER_MSVC
#pragma warning( push )
#pragma warning( disable : 4200 )
#endif

BBC_AUDIOTOOLBOX_START

typedef PACKEDSTRUCT
{
  uint8_t Count;
  uint8_t String[1];
} PSTRING;

#define RIFF_ID IFFID("RIFF")
#define RF64_ID IFFID("RF64")
#define WAVE_ID IFFID("WAVE")

#define ds64_ID IFFID("ds64")

typedef PACKEDSTRUCT
{
  char         ChunkId[4];
  uint32_t     ChunkSizeLow;
  uint32_t     ChunkSizeHigh;
} CHUNKSIZE64;

typedef PACKEDSTRUCT
{
  uint32_t     RIFFSizeLow;
  uint32_t     RIFFSizeHigh;
  uint32_t     dataSizeLow;
  uint32_t     dataSizeHigh;
  uint32_t     SampleCountLow;
  uint32_t     SampleCountHigh;
  uint32_t     TableEntryCount;
  CHUNKSIZE64  Table[0];
} ds64_CHUNK;

typedef PACKEDSTRUCT
{
  uint16_t     NChannels;
  uint32_t     SampleFrames;
  uint16_t     SampleSize;
  IEEEEXTENDED SampleRate;
} COMM_CHUNK;

#define COMM_ID IFFID("COMM")

typedef PACKEDSTRUCT
{
  uint16_t     NChannels;
  uint32_t     SampleFrames;
  uint16_t     SampleSize;
  IEEEEXTENDED SampleRate;
  uint32_t     CompressionID;
  PSTRING      CompressionString;
  uint8_t      __pad[254];
} COMM_CHUNK_EX;

typedef PACKEDSTRUCT
{
  uint32_t     TimestampHigh;
  uint32_t     TimestampLow;
} TMST_CHUNK;

#define TMST_ID IFFID("TMST")

typedef PACKEDSTRUCT
{
  uint32_t     Offset;
  uint32_t     BlockSize;
} SSND_CHUNK;

#define SSND_ID IFFID("SSND")

typedef PACKEDSTRUCT
{
  char         Description[256];                      /* ASCII : «Description of the sound sequence» */
  char         Originator[32];                        /* ASCII : «Name of the originator» */
  char         OriginatorReference[32];               /* ASCII : «Reference of the originator» */
  char         OriginationDate[10];                   /* ASCII : «yyyy:mm:dd» */
  char         OriginationTime[8];                    /* ASCII : «hh:mm:ss» */
  uint32_t     TimeReferenceLow;                      /* First sample count since midnight, low word */
  uint32_t     TimeReferenceHigh;                     /* First sample count since midnight, high word */
  uint16_t     Version;                               /* Version of the BWF; unsigned binary number */
  uint8_t      UMID[64];                              /* Binary bytes of SMPTE UMID */ 
  uint16_t     LoudnessValue;                         /* uint16_t : «Integrated Loudness Value of the file in LUFS (multiplied by 100) » */ 
  uint16_t     LoudnessRange;                         /* uint16_t : «Loudness Range of the file in LU (multiplied by 100) » */ 
  uint16_t     MaxTruePeakLevel;                      /* uint16_t : «Maximum True Peak Level of the file expressed as dBTP (multiplied by 100) » */ 
  uint16_t     MaxMomentaryLoudness;                  /* uint16_t : «Highest value of the Momentary Loudness Level of the file in LUFS (multiplied by 100) » */ 
  uint16_t     MaxShortTermLoudness;                  /* uint16_t : «Highest value of the Short-Term Loudness Level of the file in LUFS (multiplied by 100) » */ 
  uint8_t      Reserved[180];                         /* 180 bytes, reserved for future use, set to "NULL" */ 
  char         CodingHistory[0];                      /* ASCII : «History coding » */
} BROADCAST_CHUNK;

#define bext_ID IFFID("bext")

typedef PACKEDSTRUCT
{
  uint16_t     Format;
  uint16_t     Channels;
  uint32_t     SampleRate;
  uint32_t     BytesPerSecond;
  uint16_t     BlockAlign;
  uint16_t     BitsPerSample;
} WAVEFORMAT_CHUNK;

typedef PACKEDSTRUCT
{
  uint16_t     Format;
  uint16_t     Channels;
  uint32_t     SampleRate;
  uint32_t     BytesPerSecond;
  uint16_t     BlockAlign;
  uint16_t     BitsPerSample;
  uint16_t     ExtensionSize;
  union {
    uint16_t   ValidBitsPerSample;
    uint16_t   SamplesPerBlock;
    uint16_t   Reserved;
  } Samples;
  uint32_t     ChannelMask;
  uint8_t      GUID[16];
} WAVEFORMAT_EXTENSIBLE_CHUNK;

enum
{
  BBCAT_WAVE_FORMAT_PCM        = 0x0001,
  BBCAT_WAVE_FORMAT_IEEE       = 0x0003,
  BBCAT_WAVE_FORMAT_EXTENSIBLE = 0xfffe,
};
#define fmt_ID                 IFFID("fmt ")

#define data_ID IFFID("data")

#define chna_ID IFFID("chna")
typedef PACKEDSTRUCT
{
  uint16_t   TrackCount;
  uint16_t   UIDCount;
  PACKEDSTRUCT
  {
    uint16_t TrackNum;
    char     UID[12];
    char     TrackRef[14];
    char     PackRef[11];
    uint8_t  _pad;
  } UIDs[0];
} CHNA_CHUNK;

#define axml_ID IFFID("axml")

#define JUNK_ID IFFID("JUNK")

BBC_AUDIOTOOLBOX_END

#ifdef COMPILER_MSVC
#pragma warning( pop )
#endif

#endif
