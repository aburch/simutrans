#ifndef SIMTOOLS_H
#define SIMTOOLS_H

#include "simtypes.h"

uint32 get_random_seed();

uint32 setsimrand(uint32 seed, uint32 noise_seed);

/* generates a random number on [0,max-1]-interval */
uint32 simrand(const uint32 max);

/* generates a random number on [0,0xFFFFFFFFu]-interval */
uint32 simrand_plain(void);

double perlin_noise_2D(const double x, const double y, const double persistence);

// for debugging, i.e. finding hidden simrands in worng places
bool set_random_allowed( bool );

// just more speed with those (generate a precalculated map, which needs only smoothing)
void init_perlin_map( sint32 w, sint32 h );
void exit_perlin_map();

#endif
