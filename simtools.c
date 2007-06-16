#include <math.h>
#include "simtools.h"

/* This is the mersenne random generator: More random and faster! */

/* Period parameters */
#define MERSENNE_TWISTER_N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mersenne_twister[MERSENNE_TWISTER_N]; // the array for the state vector
static int mersenne_twister_index = MERSENNE_TWISTER_N + 1; // mersenne_twister_index==N+1 means mersenne_twister[N] is not initialized

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
static void MTgenerate(void)
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


/* generates a random number on [0,0xffffffff]-interval */
static uint32 simrand_plain(void)
{
	uint32 y;

	if (mersenne_twister_index >= MERSENNE_TWISTER_N) { /* generate N words at one time */
		MTgenerate();
	}
	y = mersenne_twister[mersenne_twister_index++];

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}


/* generates a random number on [0,max-1]-interval */
uint32 simrand(const uint32 max)
{
	if(max<=1) {	// may rather assert this?
		return 0;
	}

	return simrand_plain() % max;
}


static uint32 noise_seed = 0;

uint32 setsimrand(uint32 seed,uint32 ns)
{
	uint32 old_noise_seed = noise_seed;

	if(seed!=0xFFFFFFFF) {
		init_genrand( seed );
	}
	if(noise_seed!=0xFFFFFFFF) {
		noise_seed = ns*15731;
	}
	return old_noise_seed;
}


static double
int_noise(const long x, const long y)
{
    long n = x + y*101 + noise_seed;

    n = (n<<13) ^ n;
    return ( 1.0 - (double)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

static double
smoothed_noise(const int x, const int y)
{
/* this gives a very smooth world */
    const double corners = ( int_noise(x-1, y-1)+int_noise(x+1, y-1)+
                             int_noise(x-1, y+1)+int_noise(x+1, y+1) );

    const double sides   = ( int_noise(x-1, y) + int_noise(x+1, y) +
                             int_noise(x, y-1) + int_noise(x, y+1) );

    const double center  =  int_noise(x, y);

    return (corners + sides+sides + center*4.0) / 16.0;


/* a hilly world
    const double sides   = ( int_noise(x-1, y) + int_noise(x+1, y) +
                             int_noise(x, y-1) + int_noise(x, y+1) );

    const double center  =  int_noise(x, y);

    return (sides+sides + center*4) / 8.0;
*/

// this gives very hilly world
//   return int_noise(x,y);
}

static double
linear_interpolate(const double a, const double b, const double x)
{
//    return  a*(1.0-x) + b*x;
//    return  a - a*x + b*x;
    return  a + x*(b-a);
}


static double
interpolated_noise(const double x, const double y)
{
    const int    integer_X    = (int)x;
    const int    integer_Y    = (int)y;

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
 * x,y Koordinaten des Punktes
 * p   Persistenz
 */

double perlin_noise_2D(const double x, const double y, const double p)
{
    double total = 0.0;
    int i;

    for(i=0; i<6; i++) {

	const double frequency = (double)(1 << i);
	const double amplitude = pow(p, (double)i);

	total += interpolated_noise((x * frequency) / 64.0,
                                    (y * frequency) / 64.0) * amplitude;
    }

    return total;
}
