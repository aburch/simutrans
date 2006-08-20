#ifndef _simtools_h
#define _simtools_h


#ifdef __cplusplus
extern "C" {
#endif

unsigned long setsimrand(unsigned long seed);
int simrand_plain();
int simrand(int max);

void zufallsliste(void *list, int elem_size, int elem_count);

double perlin_noise_2D(const double x, const double y, const double persistence);

#ifdef __cplusplus
}
#endif

#endif
