/* code for loading heightmaps */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "environment.h"
#include "height_map_loader.h"
#include "../simio.h"
#include "../simsys.h"


height_map_loader_t::height_map_loader_t(bool new_format):
		height_map_conversion_version_new(new_format)
	{
}


// read height data from bmp or ppm files
bool height_map_loader_t::get_height_data_from_file( const char *filename, sint8 groundwater, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values )
{
	if (FILE* const file = dr_fopen(filename, "rb")) {
		char id[3];
		// parsing the header of this mixed file format is nottrivial ...
		id[0] = fgetc(file);
		id[1] = fgetc(file);
		id[2] = 0;
		if(strcmp(id, "P6")) {
			if(strcmp(id, "BM")) {
				fclose(file);
				dbg->error("height_map_loader_t::load_heightfield()","Heightfield has wrong image type %s instead P6/BM", id);
				return false;
			}
			// bitmap format
			fseek( file, 10, SEEK_SET );
			uint32 data_offset;
			sint32 w, h, format, table;
			sint16 bit_depth;
#ifdef SIM_BIG_ENDIAN
			uint32 l;
			uint16 s;
			fread( &l, 4, 1, file );
			data_offset = endian(l);
			fseek( file, 18, SEEK_SET );
			fread( &l, 4, 1, file );
			w = endian(l);
			fread( &l, 4, 1, file );
			h = endian(l);
			fseek( file, 28, SEEK_SET );
			fread( &s, 2, 1, file );
			bit_depth = endian(s);
			fread( &l, 4, 1, file );
			format = endian(l);
			fseek( file, 46, SEEK_SET );
			fread( &l, 4, 1, file );
			table = endian(l);
#else
			fread( &data_offset, 4, 1, file );
			fseek( file, 18, SEEK_SET );
			fread( &w, 4, 1, file );
			fread( &h, 4, 1, file );
			fseek( file, 28, SEEK_SET );
			fread( &bit_depth, 2, 1, file );
			fread( &format, 4, 1, file );
			fseek( file, 46, SEEK_SET );
			fread( &table, 4, 1, file );
#endif
			if((bit_depth!=8  &&  bit_depth!=24)  ||  format>1) {
				if(!update_only_values) {
					dbg->fatal("height_map_loader_t::get_height_data_from_file()","Can only use 8Bit (RLE or normal) or 24 bit bitmaps!");
				}
				fclose( file );
				return false;
			}

			// skip parsing body
			if(update_only_values) {
				ww = w;
				hh = abs(h);
				return true;
			}

			// now read the data and convert them on the fly
			hfield = new sint8[w*h];
			memset( hfield, groundwater, w*h );
			if(bit_depth==8) {
				// convert color tables to height levels
				if(table==0) {
					table = 256;
				}
				sint8 h_table[256];
				fseek( file, 54, SEEK_SET );
				for( int i=0;  i<table;  i++  ) {
					int B = fgetc(file);
					int G = fgetc(file);
					int R = fgetc(file);
					fgetc(file);	// dummy
					h_table[i] = height_map_loader_t::rgb_to_height(R, G, B);
				}
				// now read the data
				fseek( file, data_offset, SEEK_SET );
				if(format==0) {
					// uncompressed (usually mirrored, if h>0)
					bool mirror = (h<0);
					h = abs(h);
					for(  sint32 y=0;  y<h;  y++  ) {
						sint32 offset = mirror ? y*w : (h-y-1)*w;
						for(  sint32 x=0;  x<w;  x++  ) {
							hfield[x+offset] = h_table[fgetc(file)];
						}
						// skip line offset
						if(w&1) {
							fgetc(file);
						}
					}
				}
				else {
					// compressed RLE (reverse y, since mirrored)
					sint32 x=0, y=h-1;
					while (!feof(file)) {
						uint8 Count= fgetc(file);
						uint8 ColorIndex = fgetc(file);

						if (Count > 0) {
							for( sint32 k = 0;  k < Count;  k++, x++  ) {
								hfield[x+(y*w)] = h_table[ColorIndex];
							}
						} else if (Count == 0) {
							sint32 Flag = ColorIndex;
							if (Flag == 0) {
								// goto next line
								x = 0;
								y--;
							}
							else if (Flag == 1) {
								// end of bitmap
								break;
							}
							else if (Flag == 2) {
								// skip with cursor
								x += (uint8)fgetc(file);
								y -= (uint8)fgetc(file);
							}
							else {
								// uncompressed run
								Count = Flag;
								for( sint32 k = 0;  k < Count;  k++, x++  ) {
									hfield[x+y*w] = h_table[(uint8)fgetc(file)];
								}
								if (ftell(file) & 1) {	// alway even offset in file
									fseek(file, 1, SEEK_CUR);
								}
							}
						}
					}
				}
			}
			else {
				// uncompressed 24 bits
				bool mirror = (h<0);
				h = abs(h);
				for(  sint32 y=0;  y<h;  y++  ) {
					sint32 offset = mirror ? y*w : (h-y-1)*w;
					for(  sint32 x=0;  x<w;  x++  ) {
						int B = fgetc(file);
						int G = fgetc(file);
						int R = fgetc(file);
						hfield[x+offset] = height_map_loader_t::rgb_to_height(R, G, B);
					}
					fseek( file, (4-((w*3)&3))&3, SEEK_CUR );	// skip superfluos bytes at the end of each scanline
				}
			}
			// success ...
			fclose(file);
			ww = w;
			hh = h;
			return true;
		}
		else {
			// ppm format
			char buf[255];
			char *c = id+2;
			sint32 param[3]={0,0,0};
			for(int index=0;  index<3;  ) {
				// the format is "P6[whitespace]width[whitespace]height[[whitespace bitdepth]]newline]
				// however, Photoshop is the first program, that uses space for the first whitespace ...
				// so we cater for Photoshop too
				while(*c  &&  *c<=32) {
					c++;
				}
				// usually, after P6 there comes a comment with the maker
				// but comments can be anywhere
				if(*c==0) {
					read_line(buf, sizeof(buf), file);
					c = buf;
					continue;
				}
				param[index++] = atoi(c);
				while(*c>='0'  &&  *c<='9') {
					c++;
				}
			}
			// now the data
			sint32 w = param[0];
			sint32 h = param[1];
			if(param[2]!=255) {
				fclose(file);
				if(!update_only_values) {
					dbg->fatal("height_map_loader_t::load_heightfield()","Heightfield has wrong color depth %d", param[2] );
				}
				return false;
			}

			// report only values
			if(update_only_values) {
				fclose(file);
				ww = w;
				hh = h;
				return true;
			}

			// ok, now read them in
			hfield = new sint8[w*h];
			memset( hfield, groundwater, w*h );

			for(sint16 y=0; y<h; y++) {
				for(sint16 x=0; x<w; x++) {
					int R = fgetc(file);
					int G = fgetc(file);
					int B = fgetc(file);
					hfield[x+(y*w)] = height_map_loader_t::rgb_to_height(R, G, B);
				}
			}

			// success ...
			fclose(file);
			ww = w;
			hh = h;
			return true;
		}
	}
	return false;
}
