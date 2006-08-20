#ifndef _simtools_h
#define _simtools_h


#ifdef __cplusplus
extern "C" {
#endif


/* generate N words at one time */
 void	MTgenerate(void);
 unsigned long setsimrand(unsigned long seed,unsigned long noise_seed);
unsigned long simrand_plain();


/* generates a random number on [0,max-1]-interval */
inline unsigned long simrand(const unsigned long max)
{
#ifndef MERSENNE_TWISTER_N
	// defined in simtools.h!
	#define MERSENNE_TWISTER_N 624
	extern unsigned long mersenne_twister[MERSENNE_TWISTER_N];
	extern unsigned long mersenne_twister_index;
#endif
	unsigned long y;

	if(max<=1) {	// may rather assert this?
		return 0;
	}

	if (mersenne_twister_index >= MERSENNE_TWISTER_N) { /* generate N words at one time */
		MTgenerate();
	}
	y = mersenne_twister[mersenne_twister_index++];

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y%max;
};



void zufallsliste(void *list, int elem_size, int elem_count);

double perlin_noise_2D(const double x, const double y, const double persistence);

#ifdef __cplusplus
}
#endif

#endif
