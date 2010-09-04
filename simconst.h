/*
 * all defines that can change the compiling
 */

#ifndef simconst_h
#define simconst_h

// number of player
#define MAX_PLAYER_COUNT (16)
#define PLAYER_UNOWNED (15)

/* Flag for Intel byte order
 * SET THIS IN YOUR COMPILER COMMAND LINE!
 */
//#define LITTLE_ENDIAN

/* crossconnect industry and half heights like openTTD */
//#define OTTD_LIKE

/* two inclinations per pixel */
//#define DOUBLE_GROUNDS

/* single height is only 8 pixel (default 16) */
//#define HALF_HEIGHT

/* construct automatic bridges also as active player */
//#define AUTOMATIC_BRIDGES

/* construct automatic tunnels also as active player */
//#define AUTOMATIC_TUNNELS

/* citycars have a destination; if they could get near, they dissolve */
//#define DESTINATION_CITYCARS

/* need to emulate the mouse pointer with a graphic */
//#define USE_SOFTPOINTER

/* Use C implementation of image drawing routines
 * needed i.e. for MSVC and PowerPC */
//#define USE_C

// maximum distance to look ahead for tiles (if undefined, uses all the route, recommended)
//#define MAX_CHOOSE_BLOCK_TILES (64)

// The wind (i.e. approach direction) is random all over the map (not recommended, since it confuses players)
//#define USE_DIFFERENT_WIND

// define this to disallow the harbour tunnel feature
//#define ONLY_TUNNELS_BELOW_GROUND

// define this for automaticcally joining stations next to a public stop with it
//#define AUTOJOIN_PUBLIC

/*********************** Useful things for debugging ... ********************/

/* will highlite marked areas and convoi will leave traces */
//#define DEBUG_ROUTES

/* shows which tiles are drawn as dings (used in boden/grund.cc) */
//#define SHOW_FORE_GRUND

/* shows with block needed update and which not */
//#define DEBUG_FLUSH_BUFFER

/**************************** automatic stuff ********************************/


// inclination types
#ifndef DOUBLE_GROUNDS
// constants used in tools wkz_setslope / wkz_restoreslope_t
#define ALL_UP_SLOPE (16)
#define ALL_DOWN_SLOPE (17)
#define RESTORE_SLOPE (18)
#else
// double height (two slopes per tile) definitions
#define ALL_UP_SLOPE (82)
#define ALL_DOWN_SLOPE (83)
#define RESTORE_SLOPE (84)
#endif


// height calculation stuff
#if defined(HALF_HEIGHT)  ||  defined(OTTD_LIKE)
// 8 px per height
#define Z_TILE_STEP (1)
#define TILE_HEIGHT_STEP (8)
#define TILE_STEPS (16)
#define SPEED_STEP_WIDTH (1l<<16)
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 6)
#define height_scaling(i) ((i)>>1)
#define height_unscaling(i) ((i)<<1)
#else
// 16 internal pixels per tile, koord3d.z granularity is 1,
#define Z_TILE_STEP (1)
#define TILE_HEIGHT_STEP (16)
#define TILE_STEPS (16)
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)	// these must be changed for according to TILE_STEPS!
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 6)
#define height_scaling(i) (i)
#define height_unscaling(i) (i)
#endif

/*
 * Global vehicle speed conversion factor between Simutrans speed
 * and km/h
 * @author Hj. Malthaner
 */
#define VEHICLE_SPEED_FACTOR  (80)

/**
 * Converts speed value to km/h
 * @author Hj. Matthaner
 */
#define speed_to_kmh(speed) (((speed)*VEHICLE_SPEED_FACTOR+511) >> 10)

/**
 * Converts km/h value to speed
 * @author Hj. Matthaner
 */
#define kmh_to_speed(speed) (((speed) << 10) / VEHICLE_SPEED_FACTOR)


// offsets for mouse pointer
#define Z_PLAN (4)
#define Z_GRID (0)


// sanity check: USE_C if not GCC and not intel 32bit
#if !defined USE_C && (!defined __GNUC__ || !defined __i386__)
#	define USE_C
#endif

#endif
