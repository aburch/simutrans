/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "font.h"

#include "../macros.h"
#include "../simdebug.h"
#include "../sys/simsys.h"
#include "../simtypes.h"
#include "../utils/simstring.h"

#ifdef USE_FREETYPE
#include "../dataobj/environment.h"
#endif

#include <math.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>


font_t::glyph_t::glyph_t() :
	height(0),
	width(0),
	advance(0xFF),
	top(0)
{
	bitmap = 0;
}


font_t::font_t() :
	linespace (0),
	descent(0)
{
	fname[0] = 0;
}

#if 0
// FIXME

/// Decodes a single line of a glyph
static void dsp_decode_bdf_data_row(font_t::glyph_t *target, int y, int xoff, int g_width, const char *str)
{
	char buf[3];
	buf[0] = str[0];
	buf[1] = str[1];
	buf[2] = '\0';

	uint16 data = (uint16)strtol(buf, NULL, 16)<<8;

	// read second byte but use only first nibble
	if(  g_width > 8  ) {
		buf[0] = str[2];
		buf[1] = str[3];
		buf[2] = '\0';
		data |= strtol(buf, NULL, 16);
	}

	data >>= xoff;

	// now store them, and the second nibble store interleaved
	target->bitmap[y] = data>>8;

	if(  g_width+xoff > 8  ) {
		target->bitmap[y+GLYPH_BITMAP_HEIGHT] = data;
	}
}


/// Reads a single glyph.
/// @returns index of glyph successfully read, or -1 on error
static sint32 dsp_read_bdf_glyph(FILE *bdf_file, std::vector<font_t::glyph_t> &data, int glyph_limit, int f_height, int f_desc)
{
	sint32 glyph_nr = 0;
	int g_width = 0, h = 0, g_desc = 0;
	int glyph_advance = -1;
	int xoff = 0;

	while(  !feof(bdf_file)  ) {
		char str[256];

		if(  fgets(str, sizeof(str), bdf_file)==NULL  &&  !feof(bdf_file)  ) {
			return -1;
		}

		// encoding (sint8 number) in decimal
		if(  strstart(str, "ENCODING")  ) {
			glyph_nr = atoi(str + 8);
			if(  glyph_nr <= 0  ||  glyph_nr >= glyph_limit  ) {
				dbg->error("dsp_read_bdf_glyph", "Unexpected glyph (%i) for %i glyph font!\n", glyph_nr, glyph_limit);
				glyph_nr = 0;
			}
			continue;
		}

		// information over size and coding
		if(  strstart(str, "BBX")  ) {
			sscanf(str + 3, "%d %d %d %d", &g_width, &h, &xoff, &g_desc);
			continue;
		}

		// information over size and coding
		if(  strstart(str, "DWIDTH")  ) {
			glyph_advance = atoi(str + 6);
			continue;
		}

		// start if bitmap data
		if (strstart(str, "BITMAP")) {
			const int top = f_height + f_desc - h - g_desc;

			// maximum size GLYPH_HEIGHT pixels
			h = min(h + top, (int)GLYPH_BITMAP_HEIGHT);

			// read for height times
			for (int y = top; y < h; y++) {
				if(  fgets(str, sizeof(str), bdf_file)==NULL  &&  !feof(bdf_file)  ) {
					return -1;
				}

				if(  y>=0  ) {
					dsp_decode_bdf_data_row(&data[glyph_nr], y, xoff, g_width, str);
				}
			}
			continue;
		}

		// finally add width information (width = 0: not there!)
		if(  strstart(str, "ENDCHAR")  ) {
			uint8 start_h = 0;

			// find the start offset
			for(  uint8 i=0;  i<GLYPH_BITMAP_HEIGHT;  i++  ) {
				if(data[glyph_nr].bitmap[i]==0  &&  data[glyph_nr].bitmap[GLYPH_BITMAP_HEIGHT+i]==0) {
					start_h++;
				}
				else {
					break;
				}
			}

			if(  start_h==GLYPH_BITMAP_HEIGHT  ) {
				g_width = 0;
			}

			data[glyph_nr].yoff  = start_h;
			data[glyph_nr].width = g_width;
			if(  glyph_advance == -1  ) {
				// no screen width: should not happen, but we can recover
				dbg->warning( "dsp_read_bdf_glyph", "BDF warning: Glyph %i has no advance (screen width) assigned!\n", glyph_nr);
				glyph_advance = g_width + 1;
			}

			data[glyph_nr].advance = glyph_advance;

			// finished
			return glyph_nr;
		}
	}

	return -1;
}


bool font_t::load_from_bdf(FILE *bdf_file)
{
	dbg->message("font_t::load_from_bdf", "Loading BDF font '%s'", fname);
	glyphs.clear();

	uint32 f_height = 0;
	int f_desc = 0;
	int f_numglyphs = 0;
	sint32 max_glyph = 0;

	while(  !feof(bdf_file)  ) {
		char str[256];

		if(  fgets(str, sizeof(str), bdf_file)==NULL  &&  !feof(bdf_file)  ) {
			return false;
		}

		if(  strstart(str, "FONTBOUNDINGBOX")  ) {
			sscanf(str + 15, "%*d %d %*d %d", &f_height, &f_desc);
			continue;
		}

		if(  strstart(str, "CHARS")  &&  str[5] <= ' '  ) {
			// the characters 0xFFFF and 0xFFFE are guaranteed to be non-unicode characters
			f_numglyphs = atoi(str + 5) <= 0x100 ? 0x100 : 0xFFFE;

			glyphs.resize(max(f_numglyphs, 0));
			glyphs[(uint32)' '].advance = clamp(f_height / 2, 3u, GLYPH_BITMAP_HEIGHT);
			continue;
		}

		if(  strstart(str, "STARTCHAR")  &&  f_numglyphs > 0  ) {
			const sint32 glyph = dsp_read_bdf_glyph(bdf_file, glyphs, f_numglyphs, f_height, f_desc);
			max_glyph = max(max_glyph, glyph);
			continue;
		}
	}

	// ok, was successful?
	if(  f_numglyphs <= 0  ) {
		return false;
	}

	// init default glyph for missing glyphs (just a box)

	const int real_font_height = min(f_height, (int)GLYPH_BITMAP_HEIGHT);
	int h = 2;

	glyphs[0].bitmap[1]     = 0x7E; // 0111 1110
	for(  ; h < real_font_height-2;  h++  ) {
		glyphs[0].bitmap[h] = 0x42; // 0100 0010
	}
	glyphs[0].bitmap[h++]   = 0x7E; // 0111 1110


	glyphs[0].yoff  = 1; // y-offset
	glyphs[0].width = 7; // real width
	glyphs[0].advance = 8;

	linespace = f_height;
	descent   = f_desc;

	// Use only needed amount
	glyphs.resize( (uint32)max_glyph+1 );

	return true;
}
#endif

#ifdef USE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H
#include FT_BITMAP_H


bool font_t::load_from_freetype(const char *fname, int pixel_height)
{
	dbg->message( "font_t::load_from_freetype", "trying to load '%s' in size %d", fname, pixel_height);

	FT_Library ft_library = NULL;
	if(  FT_Init_FreeType(&ft_library) != FT_Err_Ok  ) {
		dbg->error( "font_t::load_from_freetype", "Freetype initialization failed" );
		return false;
	}

	// Ok, we guessed something about the filename, now actually load it
	FT_Face face;

	if(  FT_New_Face( ft_library, fname, 0, &face ) != FT_Err_Ok  ) {
		dbg->warning( "font_t::load_from_freetype", "Cannot load %s", fname );
		FT_Done_FreeType( ft_library );
		return false;
	}

	if(  FT_Set_Pixel_Sizes( face, 0, pixel_height ) != FT_Err_Ok  ) {
		dbg->warning( "font_t::load_from_freetype", "Cannot set pixel size %d for %s", pixel_height, fname);

		// try to find closest available pixel_height
		int best = -1;
		for (int i=0; i<face->num_fixed_sizes; i++) {
			int h = (face->available_sizes[i].y_ppem + 32) >> 6;
			if (best == -1  ||  abs(best - pixel_height) > abs(h - pixel_height)) {
				best = h;
			}
		}

		if (best == -1) {
			// failed
			FT_Done_Face(face);
			FT_Done_FreeType(ft_library);
			return false;
		}
		if(  FT_Set_Pixel_Sizes( face, 0, best) != FT_Err_Ok  ) {
			dbg->warning( "font_t::load_from_freetype", "Cannot set pixel size %d for %s", best, fname);
		}
		// continue anyway
	}

	glyphs.resize(0x10000);

	ascent    = face->size->metrics.ascender/64;
	linespace = face->size->metrics.height/64;
	descent   = face->size->metrics.descender/64;

	tstrncpy( this->fname, fname, lengthof(this->fname) );

	uint32 num_glyphs = 0;

	// 8 bit bitmap
	FT_Bitmap bitmap_8;
	FT_Bitmap_Init(&bitmap_8);

	for(  uint32 glyph_nr=0;  glyph_nr<0xFFFF;  glyph_nr++  ) {

		/* load glyph image into the slot (erase previous one) */
		if(  FT_Load_Char( face, glyph_nr, FT_LOAD_RENDER) != FT_Err_Ok  ) {
			// glyph not there ...
			glyphs[glyph_nr].advance = 0xFF;
			continue;
		}

		const FT_Error error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if(error != FT_Err_Ok) {
			// glyph not there ...
			glyphs[glyph_nr].advance = 0xFF;
			continue;
		}

		// use only needed amount
		num_glyphs = glyph_nr+1;

		/* now render into cache
		 * the bitmap is at slot->bitmap
		 * the glyph base is at slot->bitmap_left, CELL_HEIGHT - slot->bitmap_top
		 */

		glyph_t & glyph = glyphs[glyph_nr];

		FT_Bitmap *bitmap = &face->glyph->bitmap;
		bool one_bit_pp = false;

		// check bit depth, for some reason bdf fonts get bitmaps with 1 bit per pixel
		if (face->glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
			// need to convert
			if (FT_Bitmap_Convert(ft_library, &face->glyph->bitmap, &bitmap_8, 4) != FT_Err_Ok) {
				// should not happen
				continue;
			}
			bitmap = &bitmap_8;
			one_bit_pp = true;
		}

		// set glyph size
		glyph.height  = bitmap->rows;
		glyph.width   = bitmap->width;
		glyph.advance = (face->glyph->advance.x + 31) / 64;

		// the bitmaps are all top aligned. Bitmap top is the ascent
		// above the base line
		// to find the real top position, we must take the font ascent
		// and reduce it by the glyph ascent
		glyph.top = ascent - face->glyph->bitmap_top - 1;

		glyph.left = face->glyph->bitmap_left;

		// transform glyph to Simutrans bitmap
		glyph.bitmap = (uint8*)calloc(glyph.height * glyph.width, 1);

		if (!one_bit_pp) {
			// 8 bit alpha per pixel
			for(int y = 0; y < glyph.height; y++) {
				for(int x = 0; x < glyph.width; x++) {
					uint16 alpha = bitmap->buffer[y * bitmap->pitch + x];
					// simgraph blend routines want alpha in 0..32
					glyph.bitmap[y * glyph.width + x] = (alpha * 32) / 255;
				}
			}
		}
		else {
			// 1 bit per pixel
			for(int y = 0; y < glyph.height; y++) {
				for(int x = 0; x < glyph.width; x++) {
					uint16 alpha = bitmap->buffer[y * bitmap->pitch + x];
					// simgraph blend routines want alpha in 0..32
					glyph.bitmap[y * glyph.width + x] = (alpha * 32);
				}
			}
		}
	}

	FT_Bitmap_Done(ft_library, &bitmap_8);

	if(num_glyphs < 0x80) {
		FT_Done_Face( face );
		FT_Done_FreeType( ft_library );
		return false;
	}

	// hack for not rendered full width space
	if(  glyphs[0x3000].advance == 0xFF  &&  glyphs[0x3001].advance != 0xFF  ) {
		glyphs[0x3000].advance = glyphs[0x3001].advance;
		glyphs[0x3000].width = 0;
		glyphs[0x3000].top = 0;
	}

	if (glyphs[' '].advance == 0xFF) {
		glyphs[' '].advance = glyphs['n'].advance;
	}

	// Use only needed amount
	glyphs.resize(num_glyphs);

	FT_Done_Face( face );
	FT_Done_FreeType( ft_library );
	return true;
}

#endif


void font_t::print_debug() const
{
	dbg->debug("font_t::print_debug", "Loaded font %s with %i glyphs\n", get_fname(), get_num_glyphs());
	dbg->debug("font_t::print_debug", "height: %i, descent: %i", linespace, descent );

	/*
	for(uint8 glyph_nr = ' ';  glyph_nr<128; glyph_nr ++) {
		char msg[128 + GLYPH_BITMAP_HEIGHT * (GLYPH_BITMAP_WIDTH+1)]; // +1 for trailing newline

		char *c = msg + sprintf(msg, "glyph %c: width %i, top %i\n", glyph_nr, get_glyph_width(glyph_nr), get_glyph_yoffset(glyph_nr) );

		for(  uint32 y = 0;  y < GLYPH_BITMAP_HEIGHT;  y++  ) {
			for(  uint32 x = 0;  x < (uint32)min(GLYPH_BITMAP_WIDTH, get_glyph_width(glyph_nr));  x++  ) {
				const uint8 data = get_glyph_bitmap(glyph_nr)[y+(x/CHAR_BIT)*GLYPH_BITMAP_HEIGHT];
				const bool bit_set = (data & (0x80>>(x%CHAR_BIT))) != 0;

				*c++ = bit_set ? '*' : ' ';
			}
			*c++ = '\n';
		}
		*c++ = 0;
		dbg->debug("font_t::print_debug", "glyph data: %s", msg );
	}
	*/
}


bool font_t::load_from_file(const char *srcfilename)
{
	tstrncpy( fname, srcfilename, lengthof(fname) );

#ifdef USE_FREETYPE
	bool ok = load_from_freetype( fname, env_t::fontsize );

#if MSG_LEVEL>=4
	if(  ok  ) {
		print_debug();
	}
#endif
	return ok;
#else
	return true;
#endif
}


sint16 font_t::get_glyph_advance(utf32 c) const
{
	if(  !is_loaded()  ) {
		return 0;
	}
	else if(  c >= get_num_glyphs()  ||  glyphs[c].advance == 0xFF  ) {
		return glyphs[0].advance;
	}

	return glyphs[c].advance;
}


const font_t::glyph_t& font_t::get_glyph(utf32 c) const
{
	static glyph_t dummy;
	if (is_loaded()) {
		if (c < get_num_glyphs()  &&  glyphs[c].advance < 0xFF) {
			return glyphs[c];
		}
		else {
			return glyphs[0];
		}
	}
	return dummy;
}
