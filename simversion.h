/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMVERSION_H
#define SIMVERSION_H


#ifdef MAKEOBJ
#ifdef _MSC_VER
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }
#endif
#endif

#if defined(REVISION_FROM_FILE)  &&  !defined(REVISION)
// include external generated revision file
#include "revision.h"
#endif

#define SIM_BUILD_NIGHTLY           0
#define SIM_BUILD_RELEASE_CANDIDATE 1
#define SIM_BUILD_RELEASE           2

#define SIM_VERSION_MAJOR 120
#define SIM_VERSION_MINOR   2
#define SIM_VERSION_PATCH   1
#define SIM_VERSION_BUILD SIM_BUILD_NIGHTLY

// Beware: SAVEGAME minor is often ahead of version minor when there were patches.
//  These have no direct connection at all!
#define SIM_SAVE_MINOR      4
#define SIM_SERVER_MINOR    4

#define EX_VERSION_MAJOR	14
#define EX_VERSION_MINOR	12
#define EX_SAVE_MINOR		30

// Do not forget to increment the save game versions in settings_stats.cc when changing this

#define MAKEOBJ_VERSION "60.06"
// Transparency and new factories(60.0), railcar_tab(60.01), basic constraint + mixed_load_prohibition(60.06). NOTE: standard now 60.2

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
#	define SIM_VERSION_BUILD_STRING " Nightly development build"
#elif SIM_VERSION_BUILD == SIM_BUILD_RELEASE_CANDIDATE
#	define SIM_VERSION_BUILD_STRING " Release candidate"
#elif SIM_VERSION_BUILD == SIM_BUILD_RELEASE
#	define SIM_VERSION_BUILD_STRING
#else
#	error invalid SIM_VERSION_BUILD
#endif

#define VERSION_NUMBER QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_VERSION_MINOR) SIM_VERSION_PATCH_STRING " Extended" SIM_VERSION_BUILD_STRING " "
#define EXTENDED_VERSION QUOTEME(EX_VERSION_MAJOR) "." QUOTEME(EX_VERSION_MINOR)

#ifndef REVISION
#define REVISION EX_VERSION_MAJOR.EX_VERSION_MINOR
#endif

#define VERSION_DATE __DATE__

#define SAVEGAME_PREFIX  "Simutrans "
#define XML_SAVEGAME_PREFIX  "<?xml version=\"1.0\"?>"

#define SAVEGAME_VER_NR        "0." QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_SAVE_MINOR)
#define SERVER_SAVEGAME_VER_NR "0." QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_SERVER_MINOR)
#define EXTENDED_VER_NR		"." QUOTEME(EX_VERSION_MAJOR)
#define EXTENDED_REVISION_NR "." QUOTEME(EX_SAVE_MINOR)

#define EXTENDED_SAVEGAME_VERSION (SAVEGAME_PREFIX SAVEGAME_VER_NR EXTENDED_VER_NR)

#define RES_VERSION_NUMBER  SIM_VERSION_MAJOR, SIM_VERSION_MINOR, EX_VERSION_MAJOR, EX_VERSION_MINOR

#ifdef REVISION
#	define SIM_TITLE_REVISION_STRING " #" QUOTEME(REVISION)
#else
#	define SIM_TITLE_REVISION_STRING
#endif

#define SIM_TITLE SAVEGAME_PREFIX VERSION_NUMBER EXTENDED_VERSION SIM_TITLE_REVISION_STRING

/*********************** Settings related to network games ********************/

/* Server to announce status to */
#define ANNOUNCE_SERVER "list.extended.simutrans.org:8080"

/* Relative URL of the announce function on server */
#define ANNOUNCE_URL "/announce"

/* Relative URL of the list function on server */
#define ANNOUNCE_LIST_URL "/list?format=csv"

#endif
