/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_FONT_H
#define DISPLAY_FONT_H


#include "../simtypes.h"
#include "../utils/unicode.h"

#include <stdio.h>
#include <vector>


#define GLYPH_BITMAP_HEIGHT (23u)
#define GLYPH_BITMAP_WIDTH  (16u)
#define GLYPH_BITMAP_BPP    (1u) // cannot be changed currently

// Size in bytes of a single row
#define GLYPH_ROW_PITCH (((GLYPH_BITMAP_WIDTH*GLYPH_BITMAP_BPP)+CHAR_BIT-1)/CHAR_BIT)


/**
 * Terminology:
 *  - glyph:        display data of a single character
 *  - font:         a collection of glyphs (usually called a font face)
 *  - width,height: size of the glyph
 *  - advance:      number of pixels between the start of a glyph and the next glyph
 *                  (not necessarily equal to width)
 */
class font_t
{
public:
	/// glyph data is stored dense in an array
	/// 23 rows of bytes, second column for widths between 8 and 16
	struct glyph_t
	{
		glyph_t();

		uint8 bitmap[GLYPH_ROW_PITCH * GLYPH_BITMAP_HEIGHT];
		uint8 yoff; // = ascent - bearingY
		uint8 width;
		uint8 advance;
	};

public:
	font_t();

public:
	/// @returns true on success
	bool load_from_file(const char *fname);
	bool is_loaded() const { return !glyphs.empty(); }

	const char *get_fname() const { return fname; }
	sint16 get_linespace() const { return linespace; }
	sint16 get_descent() const { return descent; } ///< Note: Because this value is in grid coordinates, it is NEGATIVE.
	sint16 get_ascent() const { return linespace + descent; }

	/// @returns true if this is a valid (defined) glyph
	bool is_valid_glyph(utf32 c) const
	{ return is_loaded() && c < get_num_glyphs() && glyphs[c].advance != 0xFF; }

	/// @returns size in pixels between the start of this glyph and the next glyph
	uint8 get_glyph_advance(utf32 c) const;

	/// @returns width in pixels of the glyph of a character
	uint8 get_glyph_width(utf32 c) const;

	/// @returns yoffset in pixels of the glyph of a character
	uint8 get_glyph_yoffset(uint32 c) const;

	/// @returns glyph data of a character
	/// @sa font_t::glyph_t::bitmap
	const uint8 *get_glyph_bitmap(utf32 c) const;

private:
	/// Load a BDF font
	bool load_from_bdf(FILE *fin);

	/// Load a .fnt file
	bool load_from_fnt(FILE *fin);

#ifdef USE_FREETYPE
	/// Load a freetype font
	bool load_from_freetype(const char *fname, int pixel_height);
#endif

	void print_debug() const;

	uint32 get_num_glyphs() const { return glyphs.size(); }

private:
	char fname[PATH_MAX];
	sint16 linespace;
	sint16 descent;
public:	// for simgraph has_character()
	std::vector<glyph_t> glyphs;
};

#endif
