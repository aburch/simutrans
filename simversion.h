#ifndef simversion_h
#define simversion_h

#if defined(REVISION_FROM_FILE)  &&  !defined(REVISION)
// include external generated revision file
#include "revision.h"
#endif

#define SIM_BUILD_NIGHTLY           0
#define SIM_BUILD_RELEASE_CANDIDATE 1
#define SIM_BUILD_RELEASE           2

#define SIM_VERSION_MAJOR 120
#define SIM_VERSION_MINOR   0
#define SIM_VERSION_PATCH   1
#define SIM_VERSION_BUILD SIM_BUILD_NIGHTLY

// Beware: SAVEGAME minor is often ahead of version minor when there were patches.
// ==> These have no direct connection at all!
#define SIM_SAVE_MINOR      7
#define SIM_SERVER_MINOR    8

#define EX_VERSION_MAJOR	12
#define EX_VERSION_MINOR	9000

#define MAKEOBJ_VERSION "55.4"

#ifndef QUOTEME
#	define QUOTEME_(x) #x
#	define QUOTEME(x)  QUOTEME_(x)
#endif

#if SIM_VERSION_PATCH != 0
#	define SIM_VERSION_PATCH_STRING "." QUOTEME(SIM_VERSION_PATCH)
#else
#	define SIM_VERSION_PATCH_STRING
#endif

#if   SIM_VERSION_BUILD == SIM_BUILD_NIGHTLY
#	define SIM_VERSION_BUILD_STRING " Development build"
#elif SIM_VERSION_BUILD == SIM_BUILD_RELEASE_CANDIDATE
#	define SIM_VERSION_BUILD_STRING " Release Candidate"
#elif SIM_VERSION_BUILD == SIM_BUILD_RELEASE
#	define SIM_VERSION_BUILD_STRING
#else
#	error invalid SIM_VERSION_BUILD
#endif

#define VERSION_NUMBER QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_VERSION_MINOR) SIM_VERSION_PATCH_STRING " Experimental" SIM_VERSION_BUILD_STRING " "
#define EXPERIMENTAL_VERSION QUOTEME(EX_VERSION_MAJOR) "." QUOTEME(EX_VERSION_MINOR)

#define REVISION 129000

#define VERSION_DATE __DATE__

#define SAVEGAME_PREFIX  "Simutrans "
#define XML_SAVEGAME_PREFIX  "<?xml version=\"1.0\"?>"

#define SAVEGAME_VER_NR        "0." QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_SAVE_MINOR)
#define SERVER_SAVEGAME_VER_NR "0." QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_SERVER_MINOR)
#define EXPERIMENTAL_VER_NR		"." QUOTEME(EX_VERSION_MAJOR)

#define EXPERIMENTAL_SAVEGAME_VERSION (SAVEGAME_PREFIX SAVEGAME_VER_NR EXPERIMENTAL_VER_NR)

#define RES_VERSION_NUMBER  SIM_VERSION_MAJOR, SIM_VERSION_MINOR, EX_VERSION_MAJOR, EX_VERSION_MINOR

#ifdef REVISION
#	define SIM_TITLE_REVISION_STRING " - r" QUOTEME(REVISION)
#else
#	define SIM_TITLE_REVISION_STRING
#endif

#define SIM_TITLE SAVEGAME_PREFIX VERSION_NUMBER EXPERIMENTAL_VERSION /*SIM_TITLE_REVISION_STRING*/

/*********************** Settings related to network games ********************/

/* Server to announce status to */
#define ANNOUNCE_SERVER "servers.experimental.simutrans.org:80"

/* Relative URL of the announce function on server */
#define ANNOUNCE_URL "/announce"

/* Relative URL of the list function on server */
#define ANNOUNCE_LIST_URL "/list?format=csv"

#endif
