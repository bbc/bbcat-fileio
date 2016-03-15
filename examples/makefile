

BBCAT_BASE_CFLAGS=$(shell pkg-config --cflags bbcat-base-0.1 | sed -E 's/\\\"/\"/g')
BBCAT_BASE_LDFLAGS=$(shell pkg-config --libs bbcat-base-0.1)
BBCAT_DSP_CFLAGS=$(shell pkg-config --cflags bbcat-dsp-0.1 | sed -E 's/\\\"/\"/g')
BBCAT_DSP_LDFLAGS=$(shell pkg-config --libs bbcat-dsp-0.1)
BBCAT_ADM_CFLAGS=$(shell pkg-config --cflags bbcat-adm-0.1 | sed -E 's/\\\"/\"/g')
BBCAT_ADM_LDFLAGS=$(shell pkg-config --libs bbcat-adm-0.1)
BBCAT_FILEIO_CFLAGS=$(shell pkg-config --cflags bbcat-fileio-0.1 | sed -E 's/\\\"/\"/g')
BBCAT_FILEIO_LDFLAGS=$(shell pkg-config --libs bbcat-fileio-0.1)

CFLAGS=-Wall -Werror -Wextra -msse2 -O0 -g
LDFLAGS=

CXX = g++
LD = g++

APPLICATIONS=read-adm-bwf write-adm-bwf create-adm map-adm-bwf load-xml write-separate-adm udp-demo play-metadata modify-adm-bwf

all: $(APPLICATIONS)

clean:
	-rm -f $(APPLICATIONS)

%: %.cpp
	$(CXX) $(BBCAT_BASE_CFLAGS) $(BBCAT_DSP_CFLAGS) $(BBCAT_ADM_CFLAGS) $(BBCAT_FILEIO_CFLAGS) $(CFLAGS) $< $(LDFLAGS) $(BBCAT_FILEIO_LDFLAGS) $(BBCAT_ADM_LDFLAGS) $(BBCAT_DSP_LDFLAGS) $(BBCAT_BASE_LDFLAGS) -o $@
