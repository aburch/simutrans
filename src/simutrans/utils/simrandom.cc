/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <assert.h>
#include <math.h>
#include <time.h>
#include "simrandom.h"
#include "../sys/simsys.h"
#include "../dataobj/loadsave.h"


/* This is the mersenne random generator: More random and faster! */

/* Period parameters */
#define MERSENNE_TWISTER_N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static uint32 mersenne_twister[MERSENNE_TWISTER_N]; // the array for the state vector
static uint32 mersenne_twister_index = MERSENNE_TWISTER_N + 1; // mersenne_twister_index==N+1 means mersenne_twister[N] is not initialized

static uint8 random_origin = 0;


/* initializes mersenne_twister[N] with a seed */
static void init_genrand(uint32 s)
{
	mersenne_twister[0]= s & 0xffffffffUL;
	for (mersenne_twister_index=1; mersenne_twister_index<MERSENNE_TWISTER_N; mersenne_twister_index++) {
		mersenne_twister[mersenne_twister_index] = (1812433253UL * (mersenne_twister[mersenne_twister_index-1] ^ (mersenne_twister[mersenne_twister_index-1] >> 30)) + mersenne_twister_index);
		/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
		/* In the previous versions, MSBs of the seed affect   */
		/* only MSBs of the array mersenne_twister[].                        */
		/* 2002/01/09 modified by Makoto Matsumoto             */
		mersenne_twister[mersenne_twister_index] &= 0xffffffffUL;
		/* for >32 bit machines */
	}
}


/* generate N words at one time */
static void MTgenerate()
{
	static uint32 mag01[2]={0x0UL, MATRIX_A};
	uint32 y;
	int kk;

	if (mersenne_twister_index == MERSENNE_TWISTER_N+1)   /* if init_genrand() has not been called, */
		init_genrand(5489UL); /* a default initial seed is used */

	for (kk=0;kk<MERSENNE_TWISTER_N-M;kk++) {
		y = (mersenne_twister[kk]&UPPER_MASK)|(mersenne_twister[kk+1]&LOWER_MASK);
		mersenne_twister[kk] = mersenne_twister[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
	}
	for (;kk<MERSENNE_TWISTER_N-1;kk++) {
		y = (mersenne_twister[kk]&UPPER_MASK)|(mersenne_twister[kk+1]&LOWER_MASK);
		mersenne_twister[kk] = mersenne_twister[kk+(M-MERSENNE_TWISTER_N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
	}
	y = (mersenne_twister[MERSENNE_TWISTER_N-1]&UPPER_MASK)|(mersenne_twister[0]&LOWER_MASK);
	mersenne_twister[MERSENNE_TWISTER_N-1] = mersenne_twister[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

	mersenne_twister_index = 0;
}


/* returns current seed value */
uint32 get_random_seed()
{
	if (mersenne_twister_index >= MERSENNE_TWISTER_N) { /* generate N words at one time */
		MTgenerate();
	}
	return mersenne_twister[mersenne_twister_index];
}


/* generates a random number on [0,0xffffffff]-interval */
uint32 simrand_plain()
{
	uint32 y;

	if (mersenne_twister_index >= MERSENNE_TWISTER_N) { /* generate N words at one time */
		MTgenerate();
	}
	y = mersenne_twister[mersenne_twister_index++];

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y <<  7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}


/* generates a random number on [0,max-1]-interval */
uint32 simrand(const uint32 max)
{
	assert( (random_origin&INTERACTIVE_RANDOM) == 0  );

	if(max<=1) { // may rather assert this?
		return 0;
	}
	return simrand_plain() % max;
}


void simrand_rdwr(loadsave_t *file)
{
	xml_tag_t t(file, "simrand");

	// rdwr index
	file->rdwr_long(mersenne_twister_index);

	// rdwr state vector
	for (uint32 i=0; i<MERSENNE_TWISTER_N; ++i) {
		file->rdwr_long(mersenne_twister[i]);
	}
}


void clear_random_mode( uint16 mode )
{
	random_origin &= ~mode;
}


void set_random_mode( uint16 mode )
{
	random_origin |= mode;
}


uint16 get_random_mode()
{
	return random_origin;
}


static uint32 async_rand_seed = 12345678 + (uint32)time( NULL ); // Do not use dr_time(). It returns 0 on program startup for some platforms (SDL).

/* simpler simrand for anything not game critical (like UI) */
uint32 sim_async_rand( uint32 max )
{
	if(  max==0  ) {
		return 0;
	}
	async_rand_seed *= 3141592621u;
	async_rand_seed ++;

	return (async_rand_seed >> 8) % max;
}


static uint32 noise_seed = 0;

uint32 setsimrand(uint32 seed,uint32 ns)
{
	uint32 old_noise_seed = noise_seed;

	if(seed!=0xFFFFFFFF) {
		init_genrand( seed );
		async_rand_seed = seed + dr_time(); // dr_time() ok here. re comment ^^^. setsimrand not called immediately on program startup.
		random_origin = 0;
	}
	if(noise_seed!=0xFFFFFFFF) {
		noise_seed = ns*15731;
	}
	return old_noise_seed;
}


static double int_noise(const sint32 x, const sint32 y)
{
	uint32 n = (uint32)x + (uint32)y*101U + noise_seed;

	n = (n<<13) ^ n;
	return 1.0 - (double)((n * (n * n * 15731U + 789221U) + 1376312589U) & 0x7fffffff) / 1073741824.0;
}


static float *map = 0;
static sint32 map_w=0;

void init_perlin_map( sint32 w, sint32 h )
{
	map_w = w+2;
	map = new float[map_w*(h+2)];
	for(  sint32 y=0;  y<h+2;  y++ ) {
		for(  sint32 x=0;  x<map_w;  x++ ) {
			map[x+(y*map_w)] = (float)int_noise( x-1, y-1 );
		}
	}
}


void exit_perlin_map()
{
	map_w = 0;
	delete [] map;
	map = 0;
}


#define map_noise(x,y) (0+map[(x)+1+((y)+1)*map_w])

static double smoothed_noise(const int x, const int y)
{
	/* this gives a very smooth world */
	if(map) {
		const double corners =
			map_noise(x-1, y-1)+map_noise(x+1, y-1)+map_noise(x-1, y+1)+map_noise(x+1, y+1);

		const double sides =
			map_noise(x-1, y) + map_noise(x+1, y) + map_noise(x, y-1) + map_noise(x, y+1);

		const double center = map_noise(x, y);

		return (corners + sides+sides + center*4.0) / 16.0;
	}
	else {
		const double corners =
			int_noise(x-1, y-1)+int_noise(x+1, y-1)+int_noise(x-1, y+1)+int_noise(x+1, y+1);

		const double sides =
			int_noise(x-1, y) + int_noise(x+1, y) + int_noise(x, y-1) + int_noise(x, y+1);

		const double center = int_noise(x,y);

		return (corners + sides+sides + center*4.0) / 16.0;
	}


/* a hilly world
	const double sides   = ( int_noise(x-1, y) + int_noise(x+1, y) +
							 int_noise(x, y-1) + int_noise(x, y+1) );

	const double center  =  int_noise(x, y);

	return (sides+sides + center*4) / 8.0;
*/

// this gives very hilly world
//   return int_noise(x,y);
}


static double linear_interpolate(const double a, const double b, const double x)
{
//    return  a*(1.0-x) + b*x;
//    return  a - a*x + b*x;
	return  a + x*(b-a);
}


static double interpolated_noise(const double x, const double y)
{
	// The function floor is needed because (int) rounds always towards zero,
	// but we need integer_x be the biggest integer not bigger than x.
	// So  (int)      -1.5  = -1.0
	// But (int)floor(-1.5) = -2.0

	const int    integer_X    = (int)floor(x);
	const int    integer_Y    = (int)floor(y);

	const double fractional_X = x - (double)integer_X;
	const double fractional_Y = y - (double)integer_Y;

	const double v1 = smoothed_noise(integer_X,     integer_Y);
	const double v2 = smoothed_noise(integer_X + 1, integer_Y);
	const double v3 = smoothed_noise(integer_X,     integer_Y + 1);
	const double v4 = smoothed_noise(integer_X + 1, integer_Y + 1);

	const double i1 = linear_interpolate(v1 , v2 , fractional_X);
	const double i2 = linear_interpolate(v3 , v4 , fractional_X);

	return linear_interpolate(i1 , i2 , fractional_Y);
}


/**
 * x,y Point coordinates
 * p   Persistence
 */
double perlin_noise_2D(const double x, const double y, const double p)
{
	double total = 0.0;
	for(  int  i=0;  i<6;  i++  ) {
		const double frequency = (double)(1 << i);
		const double amplitude = pow(p, (double)i);
		total += interpolated_noise( (x * frequency) / 64.0, (y * frequency) / 64.0) * amplitude;
	}

	return total;
}


/* compute integer log10 */
uint32 log10(uint32 v)
{
	return ( (log2(v) + 1) * 1233) >> 12; // 1 / log_2(10) ~~ 1233 / 4096
}


uint32 log2(uint32 n)
{
	uint32 i = (n & 0xffff0000) ? 16 : 0;
	if(  (n >>= i) & 0xff00  ) {
		i |= 8;
		n >>= 8;
	}
	if(  n & 0xf0  ) {
		i |= 4;
		n >>= 4;
	}
	if(  n & 0xC  ) {
		i |= 2;
		n >>= 2;
	}
	return i | (n >> 1);
}


/* compute integer sqrt */
uint32 sqrt_i32(uint32 num)
{
	// taken from https://en.wikipedia.org/wiki/Methods_of_computing_square_roots
	uint32 res = 0;
	uint32 bit = 1 << 30; // The second-to-top bit is set: 1<<14 for short

	// "bit" starts at the highest power of four <= the argument.
	while(  bit > num  ) {
		bit >>= 2;
	}

	while(  bit != 0  ) {
		if(  num >= res + bit  ) {
			num -= res + bit;
			res = (res >> 1) + bit;
		}
		else {
			res >>= 1;
		}
		bit >>= 2;
	}
	return res;
}


/* compute integer sqrt */
uint64 sqrt_i64(uint64 num)
{
	// taken from https://en.wikipedia.org/wiki/Methods_of_computing_square_roots
	uint64 res = 0;
	uint64 bit = (uint64)1 << 62; // The second-to-top bit is set: 1<<14 for short

	// "bit" starts at the highest power of four <= the argument.
	while(  bit > num  ) {
		bit >>= 2;
	}

	while(  bit != 0  ) {
		if(  num >= res + bit  ) {
			num -= res + bit;
			res = (res >> 1) + bit;
		}
		else {
			res >>= 1;
		}
		bit >>= 2;
	}
	return res;
}
