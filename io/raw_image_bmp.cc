/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>

#include "raw_image.h"

#include "../sys/simsys.h"
#include "../simdebug.h"
#include "../simmem.h"
#include "../tpl/array_tpl.h"

#include "rdwr/raw_file_rdwr_stream.h"


#define BMPINFOHEADER_OFFSET (14)


#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

struct bitmap_file_header_t
{
	uint8 magic[2];
	uint32 file_size;
	uint16 reserved1;
	uint16 reserved2;
	uint32 image_data_offset;
} GCC_PACKED;


struct bitmap_info_header_t
{
	uint32 header_size;
	sint32 width;
	sint32 height;
	uint16 num_color_planes;
	uint16 bpp;
	uint32 compression;
	uint32 image_size;
	uint32 horizontal_resolution;
	uint32 vertical_resolution;
	uint32 num_palette_colors;
	uint32 num_important_colors;
} GCC_PACKED;

#ifdef _MSC_VER
#pragma pack(pop)
#endif


enum compression_method_t
{
	BI_RGB  = 0,
	BI_RLE8 = 1,
	BI_RLE4 = 2
};


static bool is_format_supported(uint16 bpp, uint32 compression)
{
	if (compression == BI_RGB) {
		return bpp == 8 || bpp == 24;
	}
	else if (compression == BI_RLE8) {
		return bpp == 8;
	}
	else {
		return false;
	}
}


bool raw_image_t::read_bmp(const char *filename)
{
#ifdef MAKEOBJ
	FILE *file = fopen(filename, "rb");
#else
	FILE *file = dr_fopen(filename, "rb");
#endif

	bitmap_file_header_t bmp_header;

	if (fread(&bmp_header, sizeof(bitmap_file_header_t), 1, file) != 1) {
		dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
		fclose(file);
		return false;
	}
	else if (bmp_header.magic[0] != 'B' || bmp_header.magic[1] != 'M') {
		dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
		fclose(file);
		return false;
	}

	const uint32 image_data_offset = endian(bmp_header.image_data_offset);

	bitmap_info_header_t bmpinfo_header;

	if (fseek(file, BMPINFOHEADER_OFFSET, SEEK_SET) != 0 || fread(&bmpinfo_header, sizeof(bitmap_info_header_t), 1, file) != 1) {
		dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
		fclose(file);
		return false;
	}

	const uint32 bmpinfoheader_size = endian(bmpinfo_header.header_size);
	if (bmpinfo_header.header_size < sizeof(bitmap_info_header_t)) {
		dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
		fclose(file);
		return false;
	}

	sint32 width             = endian(bmpinfo_header.width);
	sint32 height            = endian(bmpinfo_header.height);
	const uint16 bit_depth   = endian(bmpinfo_header.bpp);
	const uint32 compression = endian(bmpinfo_header.compression);
	uint32 table             = endian(bmpinfo_header.num_palette_colors);

	// We only allow the following image formats:
	//   8 bit: BI_RGB (uncompressed)
	//          BI_RLE8 (8 bit RLE)
	//  24 bit: BI_RGB
	if(  !is_format_supported(bit_depth, compression)  ) {
		dbg->warning("raw_image_t::read_bmp", "Can only use 8 bit (RLE or normal) or 24 bit bitmaps!");
		fclose(file);
		return false;
	}

	this->width  = abs(width);
	this->height = abs(height);
	this->fmt    = FMT_RGBA8888;
	this->bpp    = 32;

	// now read the data and convert them on the fly
	data = REALLOC(data, uint8, abs(width) * abs(height) * (bpp / CHAR_BIT));

	if (!data) {
		dbg->warning("raw_image_t::read_bmp", "Not enough memory");
		fclose(file);
		return false;
	}

	if(  bit_depth==8  ) {
		// convert color tables to height levels
		if(  table==0  ) {
			table = 256;
		}

		const uint32 colortable_offset = BMPINFOHEADER_OFFSET + bmpinfoheader_size;

		if(  fseek( file, colortable_offset, SEEK_SET ) != 0  ) {
			dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
			fclose(file);
			return false;
		}
		else if (table > 256) {
			dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
			fclose(file);
			return false;
		}

		uint8 h_table[256 * 4];
		for( uint32 i=0;  i<table;  i++  ) {
			const int B = fgetc(file);
			const int G = fgetc(file);
			const int R = fgetc(file);
			const int A = fgetc(file); // dummy

			if(  B==EOF  || G==EOF  || R==EOF  || A==EOF  ) {
				dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
				fclose(file);
				return false;
			}

			h_table[4*i + 0] = R;
			h_table[4*i + 1] = G;
			h_table[4*i + 2] = B;
			h_table[4*i + 3] = 0xFF;
		}

		// now read the data
		if(  fseek( file, image_data_offset, SEEK_SET ) != 0  ) {
			dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
			fclose(file);
			return false;
		}

		const bool mirror = (height<0);
		height = abs(height);
		width = abs(width);

		if(  compression==0  ) {
			// uncompressed (usually mirrored, if h<0)
			const int padding = (4 - (width & 3)) & 3; // padding at end of line

			for(  sint32 y=0;  y<height;  y++  ) {
				const sint32 offset = (mirror ? y*width : (height-y-1)*width) * (bpp/CHAR_BIT);
				for(  sint32 x=0;  x<width;  x++  ) {
					const int res = fgetc(file);
					if(  res==EOF  ) {
						dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
						fclose(file);
						return false;
					}
					data[offset + x * (bpp/CHAR_BIT) + 0] = h_table[4*res + 0];
					data[offset + x * (bpp/CHAR_BIT) + 1] = h_table[4*res + 1];
					data[offset + x * (bpp/CHAR_BIT) + 2] = h_table[4*res + 2];
					data[offset + x * (bpp/CHAR_BIT) + 3] = h_table[4*res + 3];
				}

				if(  padding != 0  ) {
					// ignore missing padding at end of file
					if (fseek(file, padding, SEEK_CUR)!=0 && y < height-1) {
						dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
						fclose(file);
						return false;
					}
				}
			}
		}
		else {
			// compressed RLE (reverse y, since mirrored)
			sint32 x=0, y = mirror ? 0 : height-1;

			while(  !feof(file)  && (mirror ? (y < height) : (y >= 0))) {
				int res = fgetc(file);
				if(  res == EOF  ) {
					dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
					fclose(file);
					return false;
				}
				uint8 Count = res;

				res = fgetc(file);
				if(  res == EOF  ) {
					dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
					fclose(file);
					return false;
				}

				const uint8 ColorIndex = res;

				if(  Count>0  ) {
					for(  sint32 k=0;  k<Count && x<width;  k++, x++  ) {
						access_pixel(x, y)[0] = h_table[4*ColorIndex + 0];
						access_pixel(x, y)[1] = h_table[4*ColorIndex + 1];
						access_pixel(x, y)[2] = h_table[4*ColorIndex + 2];
						access_pixel(x, y)[3] = h_table[4*ColorIndex + 3];
					}
				}
				else if(  Count==0  ) {
					sint32 Flag = ColorIndex;
					if(  Flag==0  ) {
						// goto next line
						x = 0;
						y = mirror ? y+1 : y-1;
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
							dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
							fclose(file);
							return false;
						}

						x += (uint8)xx;
						y = mirror ? y+(uint8)yy : y-(uint8)yy;
					}
					else {
						// uncompressed run
						Count = Flag;
						for(  sint32 k=0;  k<Count;  k++, x++  ) {
							const int idx = fgetc(file);
							if(  idx==EOF  ) {
								dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
								fclose(file);
								return false;
							}

							access_pixel(x, y)[0] = h_table[4 * (uint8)idx + 0];
							access_pixel(x, y)[1] = h_table[4 * (uint8)idx + 1];
							access_pixel(x, y)[2] = h_table[4 * (uint8)idx + 2];
							access_pixel(x, y)[3] = h_table[4 * (uint8)idx + 3];
						}

						// always even offset in file
						const long int pos = ftell(file);
						if(  pos==-1L  ||  (pos&1  &&  fseek(file, 1, SEEK_CUR)!=0)  ) {
							dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
							fclose(file);
							return false;
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
			dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
			fclose(file);
			return false;
		}

		for(  sint32 y=0;  y<height;  y++  ) {
			const sint32 real_y = mirror ? y : (height-y-1);
			for(  sint32 x=0;  x<width;  x++  ) {
				const int B = fgetc(file);
				const int G = fgetc(file);
				const int R = fgetc(file);

				if(  B==EOF  ||  G==EOF  ||  R==EOF  ) {
					dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
					fclose(file);
					return false;
				}

				access_pixel(x, real_y)[0] = R;
				access_pixel(x, real_y)[1] = G;
				access_pixel(x, real_y)[2] = B;
				access_pixel(x, real_y)[3] = 0xFF;
			}

			// skip padding to 4 bytes at the end of each scanline
			const int padding = (4 - ((width*3) & 3)) & 3;

			if(  padding != 0 && fseek( file, padding, SEEK_CUR ) != 0  ) {
				// Allow missing padding at end of file
				if (y != height-1) {
					dbg->warning("raw_image_t::read_bmp", "Malformed bmp file");
					fclose(file);
					return false;
				}
			}
		}
	}

	fclose(file);
	return true;
}


bool raw_image_t::write_bmp(const char *filename) const
{
	if (fmt == FMT_INVALID) {
		dbg->error("raw_image_t::write_bmp", "Invalid format");
		return false;
	}
	else if (fmt == FMT_GRAY8) {
		dbg->error("raw_image_t::write_bmp", "Saving greyscale images is not implemented");
		return false;
	}

	bitmap_file_header_t fheader;
	fheader.magic[0] = 'B';
	fheader.magic[1] = 'M';
	fheader.file_size = 0; // This is overwritten by the correct value below
	fheader.reserved1 = 0;
	fheader.reserved2 = 0;
	fheader.image_data_offset = 0; // This is overwritten by the correct value below

	bitmap_info_header_t iheader;
	iheader.header_size = sizeof(bitmap_info_header_t);
	iheader.width                 = endian(sint32(get_width()));
	iheader.height                = endian(sint32(get_height()));
	iheader.num_color_planes      = endian(uint16(1));
	iheader.bpp                   = endian(uint16(bpp));
	iheader.compression           = endian(uint32(BI_RGB));
	iheader.horizontal_resolution = 0;
	iheader.vertical_resolution   = 0;
	iheader.num_palette_colors    = 0;
	iheader.num_important_colors  = 0;

	// size in bytes
	const uint32 pitch           = ((get_width() * (bpp / CHAR_BIT)) + 3) & ~3; // align to 4 bytes
	const uint32 image_data_size = pitch * get_height();
	const uint32 headers_size    = uint32(sizeof(bitmap_file_header_t) + sizeof(bitmap_info_header_t));
	const uint32 gap1_size       = 2; // padding between headers_size and next multiple of 4 bytes

	fheader.image_data_offset    = endian(uint32(headers_size + gap1_size));
	fheader.file_size            = endian(uint32(headers_size + gap1_size + image_data_size));

	// now actually write the data
	FILE *f = fopen(filename, "wb");

	if (!f) {
		return false;
	}

	if (fwrite(&fheader, sizeof(bitmap_file_header_t), 1, f) != 1) {
		fclose(f);
		return false;
	}

	if (fwrite(&iheader, sizeof(bitmap_info_header_t), 1, f) != 1) {
		fclose(f);
		return false;
	}

	const uint8 zeroes[4] = { 0, 0, 0, 0 };
	if (fwrite(zeroes, 1, gap1_size, f) != gap1_size) {
		fclose(f);
		return false;
	}

	// write row by row, add padding
	const uint32 row_size = get_width() * (bpp/CHAR_BIT);
	const uint32 padding = pitch - row_size;
	assert(padding < 4);

	array_tpl<uint8> row_buffer(pitch);

	for (sint32 y = get_height()-1; y >= 0; --y) {
		const uint8 *row_start = access_pixel(0, y);
		const uint8 *row_end   = row_start + row_size;

		array_tpl<uint8>::iterator dst = row_buffer.begin();

		switch (fmt) {
		case FMT_RGB888: {
				const uint8 *src = row_start;

				while (src < row_end) {
					const uint8 R = *src++;
					const uint8 G = *src++;
					const uint8 B = *src++;

					*dst++ = B;
					*dst++ = G;
					*dst++ = R;
				}

				for (uint32 i = 0; i < padding; ++i) {
					*dst++ = 0;
				}

				if (fwrite(row_buffer.begin(), 1, pitch, f) != pitch) {
					fclose(f);
					return false;
				}
			}
			continue;

		case FMT_RGBA8888: {
				const uint8 *src = row_start;

				while (src < row_end) {
					const uint8 R = *src++;
					const uint8 G = *src++;
					const uint8 B = *src++;
					const uint8 A = *src++;

					*dst++ = B;
					*dst++ = G;
					*dst++ = R;
					*dst++ = A;
				}

				assert(padding == 0);
				if (fwrite(row_buffer.begin(), 1, pitch, f) != pitch) {
					fclose(f);
					return false;
				}
			}
			continue;

		default:
			// we only write supported formats here
			assert(false);
			break;
		}
	}

	fclose(f);
	return true;
}
