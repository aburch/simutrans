/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "height_map_loader.h"

#include "environment.h"

#include "../simio.h"
#include "../sys/simsys.h"
#include "../simmem.h"
#include "../macros.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>


#define BMPINFOHEADER_OFFSET (14)


height_map_loader_t::height_map_loader_t(sint8 min_height, sint8 max_height, env_t::height_conversion_mode mode) :
	min_allowed_height(min_height),
	max_allowed_height(max_height),
	conv_mode(mode)
{
}


// read height data from bmp or ppm files
bool height_map_loader_t::get_height_data_from_file( const char *filename, sint8 groundwater, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values )
{
	hfield = NULL;

	FILE *const file = dr_fopen(filename, "rb");
	if(  !file  ) {
		dbg->error("height_map_loader_t::get_height_data_from_file()", "Cannot open heightmap file");
		return false;
	}

	char id[3] = {0, 0, 0};
	// parsing the header of this mixed file format is non-trivial ...
	if(  fgets(id, 3, file) == NULL  ) {
		fclose(file);
		dbg->error("height_map_loader_t::get_height_data_from_file()", "Cannot read magic number");
		return false;
	}

	const bool is_bmp = strcmp(id, "BM") == 0;
	const bool is_ppm = strcmp(id, "P6") == 0;

	if(  !is_bmp  &&  !is_ppm  ) {
		fclose(file);
		dbg->error("height_map_loader_t::get_height_data_from_file()", "Unsupported height map format (must be bmp or ppm)");
		return false;
	}

	const char *err = NULL;
	if(  is_bmp  ) {
		err = read_bmp(file, groundwater, hfield, ww, hh, update_only_values);
	}
	else {
		err = read_ppm(file, groundwater, hfield, ww, hh, update_only_values);
	}

	// success ... (or not)
	fclose(file);

	if(  err  ) {
		dbg->error("height_map_loader_t::get_height_data_from_file()",
		           "Error while reading %s file: %s", is_bmp ? "bmp" : "ppm", err);

		if(  hfield  ) {
			free(hfield);
			hfield = NULL;
		}
	}

	return err == NULL;
}


const char *height_map_loader_t::read_bmp( FILE *file, sint8 groundwater, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values )
{
	sint32 sl;
	uint32 ul;
	uint16 us;

	if(  fseek( file, 10, SEEK_SET ) != 0  ||  fread(&ul, sizeof(uint32), 1, file) != 1  ) {
		return "Malformed bmp file";
	}
	const uint32 image_data_offset = endian(ul);

	// Read BITMAPINFOHEADER
	if(  fseek( file, BMPINFOHEADER_OFFSET + 0, SEEK_SET ) != 0  ||  fread(&ul, sizeof(uint32), 1, file) != 1  ) {
		return "Malformed BMP file";
	}
	const uint32 bmpinfoheader_size = endian(ul);

	if (bmpinfoheader_size < 36) {
		return "Malformed BMP file";
	}

	if(  fseek( file, BMPINFOHEADER_OFFSET + 4, SEEK_SET ) != 0  ||  fread(&sl, sizeof(sint32), 1, file) != 1  ) {
		return "Malformed BMP file";
	}
	sint32 width = endian(sl);

	if(  fseek( file, BMPINFOHEADER_OFFSET + 8, SEEK_SET ) != 0  ||  fread(&sl, sizeof(sint32), 1, file) != 1  ) {
		return "Malformed BMP file";
	}
	sint32 height = endian(sl);

	if(  fseek( file, BMPINFOHEADER_OFFSET + 14, SEEK_SET ) != 0  ||  fread(&us, sizeof(uint16), 1, file) != 1  ) {
		return "Malformed BMP file";
	}
	const uint16 bit_depth = endian(us);

	if(  fseek( file, BMPINFOHEADER_OFFSET + 16, SEEK_SET ) != 0  ||  fread(&ul, sizeof(uint32), 1, file) != 1  ) {
		return "Malformed BMP file";
	}
	const uint32 format = endian(ul);

	if(  fseek( file, BMPINFOHEADER_OFFSET + 32, SEEK_SET ) != 0  ||  fread(&ul, sizeof(uint32), 1, file) != 1  ) {
		return "Malformed BMP file";
	}
	uint32 table = endian(ul);

	// We only allow the following image formats:
	//   8 bit: BI_RGB (uncompressed)
	//          BI_RLE8 (8 bit RLE)
	//  24 bit: BI_RGB
	if(  (bit_depth!=8  ||  format>1)  &&  (bit_depth!=24  ||  format!=0)  ) {
		return "Can only use 8 bit (RLE or normal) or 24 bit bitmaps!";
	}

	// skip parsing body
	if(  update_only_values  ) {
		ww = abs(width);
		hh = abs(height);
		return NULL;
	}

	// now read the data and convert them on the fly
	hfield = MALLOCN(sint8, abs(width) * abs(height));

	if (!hfield) {
		return "Not enough memory";
	}

	memset( hfield, groundwater, abs(width) * abs(height) );

	if(  bit_depth==8  ) {
		// convert color tables to height levels
		if(  table==0  ) {
			table = 256;
		}

		const uint32 colortable_offset = BMPINFOHEADER_OFFSET + bmpinfoheader_size;

		if(  fseek( file, colortable_offset, SEEK_SET ) != 0  ) {
			return "Malformed bmp file";
		}

		sint8 h_table[256];
		for( uint32 i=0;  i<table;  i++  ) {
			const int B = fgetc(file);
			const int G = fgetc(file);
			const int R = fgetc(file);
			const int A = fgetc(file); // dummy

			if(  B==EOF  || G==EOF  || R==EOF  || A==EOF  ) {
				return "Malformed bmp file";
			}

			h_table[i] = height_map_loader_t::rgb_to_height(R, G, B);
		}

		// now read the data
		if(  fseek( file, image_data_offset, SEEK_SET ) != 0  ) {
			return "Malformed bmp file";
		}

		if(  format==0  ) {
			// uncompressed (usually mirrored, if h<0)
			const bool mirror = (height<0);
			height = abs(height);
			width = abs(width);
			const int padding = (4 - (width & 3)) & 3; // padding at end of line

			for(  sint32 y=0;  y<height;  y++  ) {
				const sint32 offset = mirror ? y*width : (height-y-1)*width;
				for(  sint32 x=0;  x<width;  x++  ) {
					const int res = fgetc(file);
					if(  res==EOF  ) {
						return "Malformed bmp file";
					}
					hfield[x+offset] = h_table[res];
				}

				if(  padding != 0  ) {
					// ignore missing padding at end of file
					if (fseek(file, padding, SEEK_CUR)!=0 && y < height-1) {
						return "Malformed bmp file";
					}
				}
			}
		}
		else {
			// compressed RLE (reverse y, since mirrored)
			sint32 x=0, y=height-1;
			while(  !feof(file)  ) {
				int res = fgetc(file);
				if(  res == EOF  ) {
					return "Malformed bmp file";
				}
				uint8 Count = res;

				res = fgetc(file);
				if(  res == EOF  ) {
					return "Malformed bmp file";
				}

				const uint8 ColorIndex = res;

				if(  Count>0  ) {
					for(  sint32 k=0;  k<Count;  k++, x++  ) {
						hfield[x+(y*width)] = h_table[ColorIndex];
					}
				}
				else if(  Count==0  ) {
					sint32 Flag = ColorIndex;
					if(  Flag==0  ) {
						// goto next line
						x = 0;
						y--;
					}
					else if(  Flag==1  ) {
						// end of bitmap
						break;
					}
					else if(  Flag==2  ) {
						// skip with cursor
						const int xx = fgetc(file);
						const int yy = fgetc(file);

						if(  xx==EOF  || yy==EOF  ) {
							return "Malformed bmp file";
						}

						x += (uint8)xx;
						y -= (uint8)yy;
					}
					else {
						// uncompressed run
						Count = Flag;
						for(  sint32 k=0;  k<Count;  k++, x++  ) {
							const int idx = fgetc(file);
							if(  idx==EOF  ) {
								return "Malformed bmp file";
							}

							hfield[x+y*width] = h_table[(uint8)idx];
						}

						// always even offset in file
						const long int pos = ftell(file);
						if(  pos==-1L  ||  (pos&1  &&  fseek(file, 1, SEEK_CUR)!=0)  ) {
							return "Malformed bmp file";
						}
					}
				}
			}
		}
	}
	else {
		// uncompressed 24 bits
		const bool mirror = (height<0);
		height = abs(height);

		// Now read the data
		if(  fseek( file, image_data_offset, SEEK_SET ) != 0  ) {
			return "Malformed bmp file";
		}

		for(  sint32 y=0;  y<height;  y++  ) {
			const sint32 offset = mirror ? y*width : (height-y-1)*width;
			for(  sint32 x=0;  x<width;  x++  ) {
				const int B = fgetc(file);
				const int G = fgetc(file);
				const int R = fgetc(file);

				if(  B==EOF  ||  G==EOF  ||  R==EOF  ) {
					return "Malformed bmp file";
				}

				hfield[x+offset] = height_map_loader_t::rgb_to_height(R, G, B);
			}

			// skip padding to 4 bytes at the end of each scanline
			const int padding = (4 - ((width*3) & 3)) & 3;

			if(  padding != 0 && fseek( file, padding, SEEK_CUR ) != 0  ) {
				// Allow missing padding at end of file
				if (y != height-1) {
					return "Malformed bmp file";
				}
			}
		}
	}

	ww = width;
	hh = height;

	return NULL;
}


const char *height_map_loader_t::read_ppm( FILE *file, sint8 groundwater, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values )
{
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
				return "Malformed ppm file";
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
		return "Heightfield has wrong color depth (must be 255)";
	}

	// report only values
	if(  update_only_values  ) {
		ww = w;
		hh = h;
		return NULL;
	}

	// ok, now read them in
	hfield = MALLOCN(sint8, w*h);

	if (!hfield) {
		return "Not enough memory";
	}

	memset( hfield, groundwater, w*h );

	for(  sint16 y=0;  y<h;  y++  ) {
		for(  sint16 x=0;  x<w;  x++  ) {
			const int R = fgetc(file);
			const int G = fgetc(file);
			const int B = fgetc(file);

			if(  R==EOF  ||  G==EOF  || B==EOF  ) {
				return "Malformed ppm file";
			}

			hfield[x+(y*w)] = height_map_loader_t::rgb_to_height(R, G, B);
		}
	}

	ww = w;
	hh = h;

	return NULL;
}


sint8 height_map_loader_t::rgb_to_height(const int r, const int g, const int b)
{
	const sint32 h0 = (2*r + 3*g + b); // 0..0x5FA

	switch (conv_mode) {
	case env_t::HEIGHT_CONV_LEGACY_SMALL: {
		// old style
		if(  env_t::pak_height_conversion_factor == 2  ) {
			return ::clamp<sint8>(h0/32 - 28, min_allowed_height, max_allowed_height); // -28..19
		}
		else {
			return ::clamp<sint8>(h0/64 - 14, min_allowed_height, max_allowed_height); // -14..9
		}
	}
	case env_t::HEIGHT_CONV_LEGACY_LARGE: {
		// new style, heights more spread out
		if(  env_t::pak_height_conversion_factor == 2  ) {
			return ::clamp<sint8>(h0/24 - 34, min_allowed_height, max_allowed_height); // -34..29
		}
		else {
			return ::clamp<sint8>(h0/48 - 18, min_allowed_height, max_allowed_height); // -18..13
		}
	}
	case env_t::HEIGHT_CONV_LINEAR: {
		return min_allowed_height + (h0*(max_allowed_height-min_allowed_height)) / 0x5FA;
	}
	case env_t::HEIGHT_CONV_CLAMP: {
		return ::clamp<sint8>((sint8)(((h0 * 0xFF) / 0x5FA) - 128), min_allowed_height, max_allowed_height);
	}
	default:
		dbg->fatal("height_map_loader_t::rgb_to_height", "Unhandled height conversion mode %d", conv_mode);
	}

	return 0;
}
