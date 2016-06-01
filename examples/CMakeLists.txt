# Arbitrarily set minimum version.
# This specific version is used to avoid complaints about the 
# cygwin platform not defining WIN32 anymore.
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12.2)

# The name of our top-level project.
PROJECT( bbcat-fileio-examples C CXX )

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_RELEASE 2)
set(PROJECT_VERSION_BUILD 0)

# use bbcat-common as installed by bbcat-base
set(BBCAT_COMMON_DIR ${CMAKE_INSTALL_PREFIX}/share/bbcat-common)

# link type: shared or static (default)
if(NOT DEFINED LINKTYPE)
	set(LINKTYPE "static")
endif()

#include all the parts that are consistent across all projects
include(${BBCAT_COMMON_DIR}/CMakeProject.txt)

find_package(bbcat-fileio REQUIRED)
bbcat_add_library(BBCAT_FILEIO)

# find JSON option in flags
is_feature_enabled(ENABLE_JSON "JSON support")

# set flags for compiling
add_definitions(${GLOBAL_FLAGS})

# set include directories for compiling
include_directories(${GLOBAL_INCLUDE_DIRS})

# set library directories for linking
link_directories(${GLOBAL_LIB_DIRS})

# set link libraries
set(LIBS bbcat-fileio${LINKTYPE} bbcat-adm${LINKTYPE} bbcat-dsp${LINKTYPE} bbcat-base${LINKTYPE})

ADD_EXECUTABLE(write-adm-bwf write-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(write-adm-bwf ${LIBS})

ADD_EXECUTABLE(read-adm-bwf read-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(read-adm-bwf ${LIBS})

ADD_EXECUTABLE(create-adm create-adm.cpp)
TARGET_LINK_LIBRARIES(create-adm ${LIBS})

ADD_EXECUTABLE(write-separate-adm write-separate-adm.cpp)
TARGET_LINK_LIBRARIES(write-separate-adm ${LIBS})

ADD_EXECUTABLE(udp-demo udp-demo.cpp)
TARGET_LINK_LIBRARIES(udp-demo ${LIBS})

ADD_EXECUTABLE(play-metadata play-metadata.cpp)
TARGET_LINK_LIBRARIES(play-metadata ${LIBS})

ADD_EXECUTABLE(read_chunks read_chunks.cpp)
TARGET_LINK_LIBRARIES(read_chunks ${LIBS})

ADD_EXECUTABLE(wav2bwav wav2bwav.cpp)
TARGET_LINK_LIBRARIES(wav2bwav ${LIBS})

ADD_EXECUTABLE(load-xml load-xml.cpp)
TARGET_LINK_LIBRARIES(load-xml ${LIBS})

ADD_EXECUTABLE(map-adm-bwf map-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(map-adm-bwf ${LIBS})

ADD_EXECUTABLE(modify-adm-bwf modify-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(modify-adm-bwf ${LIBS})

if(ENABLE_JSON)
	ADD_EXECUTABLE(gentestfile gentestfile.cpp)
	TARGET_LINK_LIBRARIES(gentestfile ${LIBS})
endif()
