/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>

#include "raw_image.h"

#include "../simdebug.h"
#include "../sys/simsys.h"
#include "../simio.h"
#include "../simmem.h"


bool raw_image_t::read_ppm(const char *filename)
{
#ifdef MAKEOBJ
	FILE *file = fopen(filename, "rb");
#else
	FILE *file = dr_fopen(filename, "rb");
#endif

	// ppm format
	char buf[255];
	const char *c = "";
	sint32 param[3] = {0, 0, 0};

	for(  int index=0;  index<3;  ) {
		// the format is "P6[whitespace]width[whitespace]height[[whitespace bitdepth]]newline]
		// however, Photoshop is the first program that uses space for the first whitespace
		// so we cater for Photoshop too
		while(  *c!=0  &&  *c<=32  ) {
			c++;
		}

		// usually, after P6 there comes a comment with the maker
		// but comments can be anywhere
		if(  *c==0  ) {
			if(  read_line(buf, sizeof(buf), file) == NULL  ) {
				dbg->warning("raw_image::read_ppm", "Malformed ppm file");
				return false;
			}

			c = buf;
			continue;
		}

		param[index++] = atoi(c);
		while(  *c>='0'  &&  *c<='9'  ) {
			c++;
		}
	}

	// now the data
	const sint32 w = param[0];
	const sint32 h = param[1];

	if(  param[2]!=255  ) {
		dbg->warning("raw_image_t::read_ppm", "Heightfield has wrong color depth (must be 255)");
	}

	width  = w;
	height = h;
	fmt    = FMT_RGBA8888;
	bpp    = 32;

	data = REALLOC(data, uint8, width * height * (bpp/CHAR_BIT));

	for(  sint16 y=0;  y<h;  y++  ) {
		uint8 *data = access_pixel(0, y);
		for(  sint16 x=0;  x<w;  x++  ) {
			const int R = fgetc(file);
			const int G = fgetc(file);
			const int B = fgetc(file);

			if(  R==EOF  ||  G==EOF  || B==EOF  ) {
				dbg->error("raw_image_t::read_ppm", "Malformed ppm file");
				return false;
			}

			*data++ = R;
			*data++ = G;
			*data++ = B;
			*data++ = 0xFF;
		}
	}

	return true;
}


bool raw_image_t::write_ppm(const char *) const
{
	dbg->error("raw_image_t::write_ppm", "Not implemented");
	return false;
}
