#ifndef _simtools_h
#define _simtools_h

#include "simtypes.h"

#ifdef __cplusplus
extern "C" {
#endif


/* generate N words at one time */
void	MTgenerate(void);
uint32 setsimrand(uint32 seed,uint32 noise_seed);
uint32 simrand_plain();

/* generates a random number on [0,max-1]-interval */
uint32 simrand(const uint32 max);

void zufallsliste(void *list, int elem_size, int elem_count);

double perlin_noise_2D(const double x, const double y, const double persistence);

#ifdef __cplusplus
}
#endif

#endif
