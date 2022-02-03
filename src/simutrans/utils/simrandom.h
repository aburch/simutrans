/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_SIMRANDOM_H
#define UTILS_SIMRANDOM_H


#include "../simtypes.h"

#include <cstddef>


class loadsave_t;


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
uint32 simrand_plain();

/// reads/writes the sate of the random number generator
void simrand_rdwr(loadsave_t *file);

double perlin_noise_2D(const double x, const double y, const double persistence);

// for network debugging, i.e. finding hidden simrands in wrong places
enum {
	INTERACTIVE_RANDOM = 1 << 0,
	STEP_RANDOM        = 1 << 1,
	SYNC_STEP_RANDOM   = 1 << 2,
	LOAD_RANDOM        = 1 << 3,
	MAP_CREATE_RANDOM  = 1 << 4
};

void set_random_mode( uint16 );
void clear_random_mode( uint16 );
uint16 get_random_mode();

// just more speed with those (generate a precalculated map, which needs only smoothing)
void init_perlin_map( sint32 w, sint32 h );
void exit_perlin_map();

/* Randomly select an entry from the given array. */
template<typename T, size_t N> T const& pick_any(T const (&array)[N])
{
	return array[simrand(N)];
}

/* Randomly select an entry from the given container. */
template<typename T, template<typename> class U> T const& pick_any(U<T> const& container)
{
	return container[simrand(container.get_count())];
}

/* Randomly select an entry from the given weighted container. */
template<typename T, template<typename> class U> T const& pick_any_weighted(U<T> const& container)
{
	return container.at_weight(simrand(container.get_sum_weight()));
}


// compute integer log10
uint32 log10( uint32 v );

uint32 log2( uint32 i );


// compute integer sqrt
uint32 sqrt_i32(uint32 num);
uint64 sqrt_i64(uint64 num);

#endif
