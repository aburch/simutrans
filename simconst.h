/* simconst.h
 *
 * all defines that can change the compiling
 */

#ifndef simconst_h
#define simconst_h

// number of player
#define MAX_PLAYER_COUNT (8)

/* Flag for Intel byte order
 * SET THIS IN YOUR COMPILER COMMAND LINE!
 * big endian also not completely supported yet
 */
//#define LITTLE_ENDIAN

/* crossconnect industry and half heights like openTTD */
//#define OTTD_LIKE

/* two inclinations per pixel */
//#define DOUBLE_GROUNDS

/* single height is only 8 pixel (default 16) */
//#define HALF_HEIGHT

/* normal ground can be covered by other ground (building does not work correctly yet ...) */
//#define COVER_TILES

/* construct automatic bridges also a active player */
//#define AUTOMATIC_BRIDGES

/* citycars have a destination; if they could get near, they dissolve */
//#define DESTINATION_CITYCARS

/* need to emulate the mouse pointer with a graphic */
//#define USE_SOFTPOINTER

/* Use C implementation of image drawing routines
 * needed i.e. for MSVC and PowerPC */
//#define USE_C

// maximum distance to look ahead for tiles (if undefined, uses all the route, recommended)
//#define MAX_CHOOSE_BLOCK_TILES (64)

/******************************* obsolete stuf ******************************/

/* prissi: if uncommented, also support for a small size font will be enabled (by not used at the moment) */
//#define USE_SMALL_FONT

/* storage shed (old implementation) LIKELY NOT WORKING */
//#define LAGER_NOT_IN_USE

/* moves old files to "new" locations pre simutrans 80.0.0 => not really needed any more */
//#define CHECK_LOCATIONS

/*********************** Useful things for debugging ... ********************/

/* will highlite marked areas and convoi will leave traces */
//#define DEBUG_ROUTES

/* shows which tiles are grown as dings (used in boden/grund.cc) */
//#define SHOW_FORE_GRUND 1

/* shows with block needed update and which not */
//#define DEBUG_FLUSH_BUFFER

/**************************** automatic stuff ********************************/

// inclination types
#ifndef DOUBLE_GROUNDS
// single height definitions
#define SOUTH_SLOPE (12)
#define NORTH_SLOPE (3)
#define WEST_SLOPE (6)
#define EAST_SLOPE (9)
#define ALL_UP_SLOPE (16)
#define ALL_DOWN_SLOPE (17)
#define RESTORE_SLOPE (18)

#define corner1(i) (i%2)
#define corner2(i) ((i/2)%2)
#define corner3(i) ((i/4)%2)
#define corner4(i) (i/8)

#else
// double height (two slopes per tile) definitions
#define SOUTH_SLOPE (36)
#define NORTH_SLOPE (4)
#define WEST_SLOPE (12)
#define EAST_SLOPE (28)
#define ALL_UP_SLOPE (82)
#define ALL_DOWN_SLOPE (83)
#define RESTORE_SLOPE (84)

#define corner1(i) (i%3)
#define corner2(i) ((i/3)%3)
#define corner3(i) ((i/9)%9)
#define corner4(i) (i/27)
#endif


// height calculation stuff
#if defined(HALF_HEIGHT)  ||  defined(OTTD_LIKE)
// 8 px per height
#define TILE_HEIGHT_STEP (8)
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 7)
#define height_scaling(i) ((i)>>1)
#define height_unscaling(i) ((i)<<1)

#else
// 16 pix per height
#define TILE_HEIGHT_STEP (16)
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 6)
#define height_scaling(i) (i)
#define height_unscaling(i) (i)

#endif

#ifdef OTTD_LIKE
#define DEFAULT_OBJPATH "pak.ttd/"
#else
#define DEFAULT_OBJPATH "pak/"
#endif

// bug alert for GCC 4.03
#ifdef USE_C
#define USE_C_FOR_IMAGE
#endif

#endif
