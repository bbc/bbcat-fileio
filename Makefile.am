
AUTOMAKE_OPTIONS = foreign

include doxygen.am

SUBDIRS = src test

fileio_DATA  = share/licences.txt share/standarddefinitions.xml

fileiodir = $(prefix)/share/$(PACKAGE)

EXTRA_DIST = README COPYING autogen.sh $(fileio_DATA)

pkgconfig_DATA = bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR).pc

bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR).pc: bbcat-fileio.pc
	cp bbcat-fileio.pc bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR).pc

bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR)-uninstalled.pc: bbcat-fileio-uninstalled.pc
	cp bbcat-fileio-uninstalled.pc bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR)-uninstalled.pc

BUILT_SOURCES = bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR)-uninstalled.pc gitbranch.txt

CLEANFILES = bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR).pc bbcat-fileio-$(BBCAT_FILEIO_MAJORMINOR)-uninstalled.pc src/SelfRegisteringObjects.h src/version.h src/version.cpp gitbranch.txt .branch

ACLOCAL_FLAGS = -I m4

.PHONY: gitbranch.txt

gitbranch.txt:
	@-git rev-parse --abbrev-ref HEAD >.branch && (diff gitbranch.txt .branch >/dev/null || cp .branch gitbranch.txt)
	@if [ ! -e "gitbranch.txt" ] ; then touch "gitbranch.txt" ; fi
	@echo "Branch: `cat gitbranch.txt`"
