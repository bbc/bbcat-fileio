pkgincludedir = $(includedir)/bbcat-fileio-@BBCAT_FILEIO_MAJORMINOR@/bbcat-fileio

lib_LTLIBRARIES = libbbcat-fileio-@BBCAT_FILEIO_MAJORMINOR@.la

libbbcat_fileio_@BBCAT_FILEIO_MAJORMINOR@_la_LDFLAGS =	\
	-no-undefined										\
	-version-info $(BBCAT_FILEIO_LIBVERSION)			\
     $(BBCAT_FILEIO_LDFLAGS)

libbbcat_fileio_@BBCAT_FILEIO_MAJORMINOR@_la_LIBADD =	\
	$(BBCAT_BASE_LIBS)									\
	$(BBCAT_DSP_LIBS)									\
	$(BBCAT_ADM_LIBS)									\
	$(BBCAT_GLOBAL_FILEIO_LIBS)							\
	$(TINYXML_LIBS)

libbbcat_fileio_@BBCAT_FILEIO_MAJORMINOR@_la_CPPFLAGS =	\
	$(BBCAT_BASE_CFLAGS)								\
	$(BBCAT_DSP_CFLAGS)									\
	$(BBCAT_ADM_CFLAGS)									\
	$(BBCAT_FILEIO_CFLAGS)								\
	$(BBCAT_GLOBAL_FILEIO_CFLAGS)						\
	$(TINYXML_CFLAGS)

libbbcat_fileio_@BBCAT_FILEIO_MAJORMINOR@_la_SOURCES =	\
	ADMAudioFileSamples.cpp								\
	ADMRIFFFile.cpp										\
	Playlist.cpp										\
	RIFFChunk.cpp										\
	RIFFChunks.cpp										\
	RIFFFile.cpp										\
	SoundFileAttributes.cpp								\
	SoundObjectFile.cpp									\
	TinyXMLADMData.cpp									\
	XMLADMData.cpp										\
	version.cpp

pkginclude_HEADERS =							\
	ADMAudioFileSamples.h						\
	ADMRIFFFile.h								\
	Playlist.h									\
	RIFFChunk.h									\
	RIFFChunk_Definitions.h						\
	RIFFChunks.h								\
	RIFFFile.h									\
	SoundFileAttributes.h						\
	SoundObjectFile.h							\
	TinyXMLADMData.h							\
	XMLADMData.h								\
	version.h

CLEANFILES = version.h version.cpp

noinst_HEADERS =

version.h: Makefile ../config.h ../gitbranch.txt
	echo "{\"@PACKAGE@\", \"@VERSION@-`cat ../gitbranch.txt`\"}," >version.h

version.cpp: Makefile ../config.h ../gitbranch.txt
	echo "/* Auto-generated: DO NOT EDIT! */" >version.cpp
	echo "#include <bbcat-base/LoadedVersions.h>" >>version.cpp
	echo "BBC_AUDIOTOOLBOX_START" >>version.cpp
	echo "volatile uint_t __keep_@PACKAGE@_version = bbcat::LoadedVersions::Get().Register(\"@PACKAGE@\", \"@VERSION@-`cat ../gitbranch.txt`\");" >>version.cpp
	echo "BBC_AUDIOTOOLBOX_END" >>version.cpp
