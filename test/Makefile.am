LDADD = $(BBCAT_BASE_LIBS)										\
		$(BBCAT_DSP_LIBS)										\
		$(BBCAT_AUDIOOBJECTS_LIBS)								\
		$(BBCAT_GLOBAL_FILEIO_LIBS)								\
        $(BOOST_UNIT_TEST_FRAMEWORK_LIB)						\
		$(PTHREAD_LIBS)											\
        ../src/libbbcat-fileio-@BBCAT_FILEIO_MAJORMINOR@.la

AM_CPPFLAGS = $(BBCAT_BASE_CFLAGS)					\
			  $(BBCAT_DSP_CFLAGS)					\
			  $(BBCAT_AUDIOOBJECTS_CFLAGS)			\
			  $(BBCAT_FILEIO_CFLAGS)				\
			  $(BBCAT_GLOBAL_FILEIO_CFLAGS)			\
			  $(PTHREAD_CFLAGS)						\
			  -std=c++11

check_PROGRAMS =
TESTS =

admxmltest_SOURCES = admxmltest.cpp
check_PROGRAMS += admxmltest
TESTS += admxmltest
