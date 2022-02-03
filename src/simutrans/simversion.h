/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMVERSION_H
#define SIMVERSION_H


#ifndef REVISION
// include external generated revision file
#include "revision.h"
#endif

#define SIM_BUILD_NIGHTLY           0
#define SIM_BUILD_RELEASE_CANDIDATE 1
#define SIM_BUILD_RELEASE           2

#define SIM_VERSION_MAJOR 123
#define SIM_VERSION_MINOR   0
#define SIM_VERSION_PATCH   2
#define SIM_VERSION_BUILD SIM_BUILD_NIGHTLY

// Beware: SAVEGAME minor is often ahead of version minor when there were patches.
// ==> These have no direct connection at all!
#define SIM_SAVE_MINOR      1
#define SIM_SERVER_MINOR    1
// NOTE: increment before next release to enable save/load of new features

#define MAKEOBJ_VERSION "60.6"

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
#	define SIM_VERSION_BUILD_STRING " Nightly"
#elif SIM_VERSION_BUILD == SIM_BUILD_RELEASE_CANDIDATE
#	define SIM_VERSION_BUILD_STRING " Release Candidate"
#elif SIM_VERSION_BUILD == SIM_BUILD_RELEASE
#	define SIM_VERSION_BUILD_STRING
#else
#	error invalid SIM_VERSION_BUILD
#endif

#define VERSION_NUMBER QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_VERSION_MINOR) SIM_VERSION_PATCH_STRING SIM_VERSION_BUILD_STRING

#define VERSION_DATE __DATE__

#define SAVEGAME_PREFIX  "Simutrans "
#define XML_SAVEGAME_PREFIX  "<?xml version=\"1.0\"?>"

#define SAVEGAME_VER_NR        "0." QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_SAVE_MINOR)
#define SERVER_SAVEGAME_VER_NR "0." QUOTEME(SIM_VERSION_MAJOR) "." QUOTEME(SIM_SERVER_MINOR)

#define RES_VERSION_NUMBER  0, SIM_VERSION_MAJOR, SIM_VERSION_MINOR, SIM_VERSION_PATCH

#ifdef REVISION
#	define SIM_TITLE_REVISION_STRING " - r" QUOTEME(REVISION)
#else
#	define SIM_TITLE_REVISION_STRING
#endif

#	define SIM_TITLE SAVEGAME_PREFIX VERSION_NUMBER SIM_TITLE_REVISION_STRING


/*********************** Settings related to network games ********************/

/* Server to announce status to */
#define ANNOUNCE_SERVER "servers.simutrans.org:80"

/* Relative URL of the announce function on server */
#define ANNOUNCE_URL "/announce"

/* Relative URL of the list function on server */
#define ANNOUNCE_LIST_URL "/list?format=csv"

/* url for obtaining the external IP for easz servers */
#define QUERY_ADDR_IP "simutrans-forum.de:80"
#define QUERY_ADDR_IPv4_ONLY "ipv4.simutrans-forum.de:80"

/* Relative URL of the IP function on server */
#define QUERY_ADDR_URL "/get_IP.php"

#endif
