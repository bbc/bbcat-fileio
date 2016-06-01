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
	$(BBCAT_GLOBAL_FILEIO_LIBS)

libbbcat_fileio_@BBCAT_FILEIO_MAJORMINOR@_la_CPPFLAGS =	\
	$(BBCAT_BASE_CFLAGS)								\
	$(BBCAT_DSP_CFLAGS)									\
	$(BBCAT_ADM_CFLAGS)									\
	$(BBCAT_FILEIO_CFLAGS)								\
	$(BBCAT_GLOBAL_FILEIO_CFLAGS)

libbbcat_fileio_sources =						\
	ADMAudioFileSamples.cpp						\
	ADMRIFFFile.cpp								\
	Playlist.cpp								\
	RIFFChunk.cpp								\
	RIFFChunks.cpp								\
	RIFFFile.cpp								\
	SoundFileAttributes.cpp						\
	TinyXMLADMData.cpp							\
	XMLADMData.cpp

pkginclude_HEADERS =							\
	ADMAudioFileSamples.h						\
	ADMRIFFFile.h								\
	Playlist.h									\
	RIFFChunk.h									\
	RIFFChunk_Definitions.h						\
	RIFFChunks.h								\
	RIFFFile.h									\
	SoundFileAttributes.h						\
	TinyXMLADMData.h							\
	XMLADMData.h

#CLEANFILES = register.cpp

noinst_HEADERS =

# register.cpp is included in repo but will be updated by the script below 
register.cpp: Makefile ../config.h ../gitbranch.txt $(libbbcat_fileio_sources)
	sh $(prefix)/share/bbcat-common/findregisterfunctions.sh "bbcat-fileio" "@VERSION@-`cat ../gitbranch.txt`" "$(libbbcat_fileio_sources)" "bbcat_adm" >$@

libbbcat_fileio_@BBCAT_FILEIO_MAJORMINOR@_la_SOURCES = $(libbbcat_fileio_sources) register.cpp
