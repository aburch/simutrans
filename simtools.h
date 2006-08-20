#ifndef _simtools_h
#define _simtools_h


unsigned long setsimrand(unsigned long seed);
int simrand();
int simrand(int max);

void zufallsliste(void *list, int elem_size, int elem_count);

double perlin_noise_2D(double x, double y, double persistence);

#endif
