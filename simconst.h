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

// The wind (i.e. approach direction) is random all over the map (not recommended, since it confuses players)
//#define USE_DIFFERENT_WIND

// define this to disallow the harbour tunnel feature
//#define ONLY_TUNNELS_BELOW_GROUND

// define this for automaticcally joining stations next to a public stop with it
//#define AUTOJOIN_PUBLIC

/******************************* obsolete stuf ******************************/

/* storage shed (old implementation) LIKELY NOT WORKING */
//#define LAGER_NOT_IN_USE

/*********************** Useful things for debugging ... ********************/

/* will highlite marked areas and convoi will leave traces */
//#define DEBUG_ROUTES

/* shows which tiles are grown as dings (used in boden/grund.cc) */
//#define SHOW_FORE_GRUND 1

/* shows with block needed update and which not */
//#define DEBUG_FLUSH_BUFFER

/* shows reserved tiles */
//#define DEBUG_PDS

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




#ifdef OTTD_LIKE
#define DEFAULT_OBJPATH "pak.ttd/"
#else
#define DEFAULT_OBJPATH "pak/"
#endif

#endif
