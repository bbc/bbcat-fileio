#sources
set(_sources
	ADMAudioFileSamples.cpp
	ADMRIFFFile.cpp
	Playlist.cpp
	RIFFChunk.cpp
	RIFFChunks.cpp
	RIFFFile.cpp
	SoundFileAttributes.cpp
	TinyXMLADMData.cpp
	XMLADMData.cpp
)

# public headers
set(_headers
	ADMAudioFileSamples.h
	ADMRIFFFile.h
	PlaybackTracker.h
	Playlist.h
	RIFFChunk.h
	RIFFChunk_Definitions.h
	RIFFChunks.h
	RIFFFile.h
	SoundFileAttributes.h
	TinyXMLADMData.h
	XMLADMData.h
	register.h
)

# auto-generate register.cpp 
bbcat_generate_register_cpp(DEPENDENCIES "bbcat_adm")

#include all the parts that are consistent across all projects
include(${BBCAT_COMMON_DIR}/CMakeLibrary.txt)

TARGET_LINK_LIBRARIES(bbcat-fileio bbcat-adm bbcat-dsp bbcat-base)
