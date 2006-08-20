/* some geme-dependent constants ....
 */

#ifndef simconst_h
#define simconst_h

// number of player
#define MAX_PLAYER_COUNT (8)

// maybe we should find a better place than here ...
// warning: never change this directly, many places assume this is 16!!!
#define TILE_HEIGHT_STEP (16)

// inclination types
#ifndef DOUBLE_GROUNDS
#define SOUTH_SLOPE 12
#define NORTH_SLOPE 3
#define WEST_SLOPE 6
#define EAST_SLOPE 9
#define ALL_UP_SLOPE 16
#define ALL_DOWN_SLOPE 17
#define RESTORE_SLOPE 18
#else
#define SOUTH_SLOPE 36
#define NORTH_SLOPE 4
#define WEST_SLOPE 12
#define EAST_SLOPE 28
#define ALL_UP_SLOPE 82
#define ALL_DOWN_SLOPE 83
#define RESTORE_SLOPE 84
#endif


// height calculation stuff
#if defined(HALF_HEIGHT)  ||  defined(OTTD_LIKE)
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 7)
#define height_scaling(i) ((i)>>1)
#define height_unscaling(i) ((i)<<1)
#else
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 6)
#define height_scaling(i) (i)
#define height_unscaling(i) (i)
#endif


#endif
