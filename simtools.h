#ifndef SIMTOOLS_H
#define SIMTOOLS_H

#include "simtypes.h"


uint32 get_random_seed();

uint32 setsimrand(uint32 seed, uint32 noise_seed);

/* generates a random number on [0,max-1]-interval
 * without affecting the game state
 * Use this for UI etc.
 */
uint32 sim_async_rand(const uint32 max);

/* generates a random number on [0,max-1]-interval */
uint32 simrand(const uint32 max);

/* generates a random number on [0,0xFFFFFFFFu]-interval */
uint32 simrand_plain(void);

double perlin_noise_2D(const double x, const double y, const double persistence);

// for netowrk debugging, i.e. finding hidden simrands in worng places
enum { INTERACTIVE_RANDOM=1, STEP_RANDOM=2, SYNC_STEP_RANDOM=4, LOAD_RANDOM=8 };
void set_random_mode( uint16 );
void clear_random_mode( uint16 );

// just more speed with those (generate a precalculated map, which needs only smoothing)
void init_perlin_map( sint32 w, sint32 h );
void exit_perlin_map();

#endif
