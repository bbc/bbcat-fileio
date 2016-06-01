LDADD = $(BBCAT_BASE_LIBS)									\
		$(BBCAT_DSP_LIBS)									\
		$(BBCAT_ADM_LIBS)									\
		$(BBCAT_GLOBAL_FILEIO_LIBS)							\
        ../src/libbbcat-fileio-@BBCAT_FILEIO_MAJORMINOR@.la

AM_CPPFLAGS = $(BBCAT_BASE_CFLAGS)						\
			  $(BBCAT_DSP_CFLAGS)						\
			  $(BBCAT_ADM_CFLAGS)						\
			  $(BBCAT_FILEIO_CFLAGS)					\
			  $(BBCAT_GLOBAL_FILEIO_CFLAGS)				\
			  -I$(prefix)/share/bbcat-common/include	\
			  -I../src

AM_CXXFLAGS = $(AM_CPPFLAGS)					\
			  -std=c++11

check_PROGRAMS =
TESTS =

tests_SOURCES = testbase.cpp admxmltest.cpp
check_PROGRAMS += tests
TESTS += tests
