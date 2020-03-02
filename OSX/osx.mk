#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#

# Makefile to build bundle applications for Mac OS X
# bundle      makes a bundle designed to work on the system it was build on
# bundle_dist makes a bundle, which should work on all supported systems (custom config needed)

# Variables:
# CXXHOST       CXX for host system. Only used when crosscompiling.
# BINARY_PPC    path to the binary for PPC
# BINARY_i386   path to the binary for i386 (default $(PROGDIR)/$(PROG))
# SDL_PPC       path to SDL.framwork for PPC
# SDL_i386      path to SDL.framwork for i386 (default ../Frameworks/SDL.framework)


# The name of the project is simutrans => thus we want to build the default bundle under simutrans
PROGNAME = $(PROG)
ifeq ($(PROGNAME),sim)
  PROGNAME := "simutrans"
endif


CXXHOST     ?= $(CXX)
BINARY_i386 ?= "$(PROGDIR)/$(PROG).app/Contents/MacOS/$(PROG)"
SDL_i386    ?= "../Frameworks/SDL.framework"

.PHONY: bundle bundle_dist


"OSX/getversion":  OSX/getversion.cc simversion.h
	@echo "===> Building $@"
	$(Q)$(CXXHOST) "OSX/getversion.cc" -o $@

bundle: all "OSX/getversion"
	@echo "===> Building bundle"
	$(Q)rm -fr   "$(PROGDIR)/$(PROG).app"
	$(Q)mkdir -p "$(PROGDIR)/$(PROG).app/Contents/MacOS"
	$(Q)mkdir -p "$(PROGDIR)/$(PROG).app/Contents/Resources"
	$(Q)cp "$(PROGDIR)/$(PROG)"   "$(PROGDIR)/$(PROG).app/Contents/MacOS/$(PROG)"
	$(Q)cp "OSX/simutrans.icns"   "$(PROGDIR)/$(PROG).app/Contents/Resources/$(PROG).icns"
	$(Q)echo "APPL????" >         "$(PROGDIR)/$(PROG).app/Contents/PkgInfo"
	$(Q)OSX/plistgen.sh           "$(PROGDIR)/$(PROG).app" "$(PROG)"

bundle_dist: bundle
	@echo "===> Changing bundle to be distributable"
	$(Q)mkdir -p "$(PROGDIR)/$(PROG).app/Contents/Frameworks"
	$(Q)cp "$(BINARY_i386)"           "$(PROGDIR)/$(PROG).app/Contents/MacOS/$(PROG).i386"
	$(Q)cp "$(BINARY_PPC)"            "$(PROGDIR)/$(PROG).app/Contents/MacOS/$(PROG).ppc"
	$(Q)cp "OSX/binary_picker.sh"     "$(PROGDIR)/$(PROG).app/Contents/MacOS/$(PROG)"
	$(Q)cp -r "$(SDL_PPC)"             "$(PROGDIR)/$(PROG).app/Contents/Frameworks/SDL-ppc.framework"
	$(Q)cp -r "$(SDL_i386)"            "$(PROGDIR)/$(PROG).app/Contents/Frameworks/SDL-i386.framework"
