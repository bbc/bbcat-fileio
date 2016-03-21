# Arbitrarily set minimum version.
# This specific version is used to avoid complaints about the 
# cygwin platform not defining WIN32 anymore.
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12.2)

set(WIN32_LIB_DESTINATION "c:/local")

if(WIN32 AND NOT CYGWIN)
	set(INSTALL_CMAKE_DIR ${WIN32_LIB_DESTINATION}/cmake)
else()
	set(INSTALL_CMAKE_DIR lib/cmake)
endif()

if(NOT IS_ABSOLUTE "${INSTALL_CMAKE_DIR}")
	set(INSTALL_CMAKE_DIR "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${INSTALL_CMAKE_DIR})
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${INSTALL_CMAKE_DIR})

if(MSVC)
	# enable /bigobj so Visual Studio doesn't whinge
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /bigobj")
	# stop unsafe warnings, ensure STL is exported and prevent std::min()/std::max() causing errors
	ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS -DEXP_STL -DNOMINMAX")
endif()

# TBD: Switch between GNU compiler and clang on Linux platforms
OPTION( SET_CLANG_COMPILER "Use the clang compiler on Linux platforms" OFF )

# tell code that this is a CMake build
ADD_DEFINITIONS("-DCMAKE_BUILD")

############################################################
# MacOS / Linux compiler flags

if(NOT MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wextra -msse2")
endif()

############################################################
# Adm optimisation

OPTION( BUILD_OPTIMISE "Optimisation" ON )

if(MSVC AND BUILD_OPTIMISE)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2")
endif()

if(NOT MSVC AND BUILD_OPTIMISE)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
endif()

############################################################
# Pull C flags into C++ compile

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")

find_package(bbcat-base REQUIRED)
include_directories(${BBCAT_BASE_INCLUDE_DIRS})

find_package(bbcat-dsp REQUIRED)
include_directories(${BBCAT_DSP_INCLUDE_DIRS})

find_package(bbcat-adm REQUIRED)
include_directories(${BBCAT_ADM_INCLUDE_DIRS})

find_package(bbcat-fileio REQUIRED)
include_directories(${BBCAT_FILEIO_INCLUDE_DIRS})

ADD_EXECUTABLE(write-adm-bwf write-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(write-adm-bwf bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(read-adm-bwf read-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(read-adm-bwf bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(create-adm create-adm.cpp)
TARGET_LINK_LIBRARIES(create-adm bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(write-separate-adm write-separate-adm.cpp)
TARGET_LINK_LIBRARIES(write-separate-adm bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(udp-demo udp-demo.cpp)
TARGET_LINK_LIBRARIES(udp-demo bbcat-base)

ADD_EXECUTABLE(load-xml load-xml.cpp)
TARGET_LINK_LIBRARIES(load-xml bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(map-adm-bwf map-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(map-adm-bwf bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(modify-adm-bwf modify-adm-bwf.cpp)
TARGET_LINK_LIBRARIES(modify-adm-bwf bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(play-metadata play-metadata.cpp)
TARGET_LINK_LIBRARIES(play-metadata bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

ADD_EXECUTABLE(write-4gb-file write-4gb-file.cpp)
TARGET_LINK_LIBRARIES(write-4gb-file bbcat-base bbcat-dsp bbcat-adm bbcat-fileio)

set(GLOBAL_FLAGS
	${BBCAT_FILEIO_FLAGS})

ADD_DEFINITIONS(${GLOBAL_FLAGS})
