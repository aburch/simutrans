#include "simtools.h"
#include "simmem.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>


//#include "simintr.h"


static unsigned long rand_seed = 0;
static unsigned long noise_seed = 0;

unsigned long setsimrand(unsigned long seed)
{
    unsigned long old_seed = rand_seed;

    rand_seed = seed;
    noise_seed = seed*15731;

    return old_seed;
}

int simrand()
{
   // eigene, ganz einfache Random-Funktion
   // damit plattformunabhängig

   rand_seed *= 3141592621u;
   rand_seed ++;

   return (rand_seed >> 8);
}

int simrand(int max)
{
    return simrand() % max;
}

/**
 * verwuerfelt liste list zufaellig
 */
void
zufallsliste(void *list, int elem_size, int elem_count)
{
    char *buf = (char *)guarded_malloc(elem_size*elem_count);
    int i, p;

    memcpy(buf, list, elem_size*elem_count);


    i = elem_count;
    p = 0;

    while(i > 0) {
	int rand_elem = simrand(i);

        memcpy(((char *)list)+p*elem_size,
               buf+rand_elem*elem_size,
	       elem_size);

	if(rand_elem < i-1) {
	    memmove(buf+rand_elem*elem_size,
                    buf+(rand_elem+1)*elem_size,
		    (i-rand_elem-1)*elem_size);
        }

	i --;
	p ++;
    }

    guarded_free( buf );
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

static inline double
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

    for(int i=0; i<6; i++) {

	const double frequency = (double)(1 << i);
	const double amplitude = pow(p, (double)i);

	total += interpolated_noise((x * frequency) / 64.0,
                                    (y * frequency) / 64.0) * amplitude;
    }

    return total;
}
