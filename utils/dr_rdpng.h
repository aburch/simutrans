#ifndef DR_RDPNG_H
#define DR_RDPNG_H

#ifdef __cplusplus
extern "C" {
#endif

int load_block(unsigned char** block, unsigned* width, unsigned* height, const char* fname, int base_img_size);

#ifdef __cplusplus
}
#endif

#endif
