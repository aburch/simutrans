#ifndef SIMTOOLS_H
#define SIMTOOLS_H

#include "simtypes.h"

uint32 setsimrand(uint32 seed, uint32 noise_seed);

/* generates a random number on [0,max-1]-interval */
uint32 simrand(const uint32 max);

/* generates a random number on [0,0xFFFFFFFFu]-interval */
uint32 simrand_plain(void);

double perlin_noise_2D(const double x, const double y, const double persistence);

#endif
