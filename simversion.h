#ifndef simversion_h
#define simversion_h

#define MAKEOBJ_VERSION "52"

#define VERSION_NUMBER "110.6 Experimental "
#define WIDE_VERSION_NUMBER L"110.6 Experimental "

#define VERSION_DATE __DATE__

#define SAVEGAME_PREFIX  "Simutrans "
#define XML_SAVEGAME_PREFIX  "<?xml version=\"1.0\"?>"

#define SAVEGAME_VER_NR  "0.110.6"
#define SERVER_SAVEGAME_VER_NR  "0.110.7"

#define EXPERIMENTAL_VERSION L"10.3"
#define NARROW_EXPERIMENTAL_VERSION "10.3"
#define EXPERIMENTAL_VER_NR ".10"
#define EXPERIMENTAL_SAVEGAME_VERSION (SAVEGAME_PREFIX SAVEGAME_VER_NR EXPERIMENTAL_VER_NR)

#define RES_VERSION_NUMBER  110, 6, 10, 3


/*********************** Settings related to network games ********************/

/* Server to announce status to */
#define ANNOUNCE_SERVER "servers.experimental.simutrans.org:8080"

/* Relative URL of the announce function on server */
#define ANNOUNCE_URL "/announce"

/* Relative URL of the list function on server */
#define ANNOUNCE_LIST_URL "/list?format=csv"

/* Name of file to save server listing to temporarily while downloading list */
#define SERVER_LIST_FILE "serverlist.csv"

#endif
