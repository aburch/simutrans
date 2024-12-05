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
#include "../io/raw_image.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>



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

	raw_image_t heightmap_img;
	if (!heightmap_img.read_from_file(filename)) {
		dbg->error("height_map_loader_t::get_height_data_from_file", "Cannot read heightmap file '%s'", filename);
		return false;
	}

	// report only values
	if(  update_only_values  ) {
		ww = heightmap_img.get_width();
		hh = heightmap_img.get_height();
		return true;
	}

	// ok, now read them in
	hfield = MALLOCN(sint8, heightmap_img.get_width()*heightmap_img.get_height());

	if (!hfield) {
		dbg->error("height_map_loader_t::get_height_data_from_file", "Not enough memory");
		return false;
	}

	memset( hfield, groundwater, heightmap_img.get_width()*heightmap_img.get_height() );

	for (uint32 y=0; y<heightmap_img.get_height(); y++) {
		for (uint32 x = 0; x < heightmap_img.get_width(); x++) {
			const uint8 pixel_value = *heightmap_img.access_pixel(x, y);
			hfield[x + y*heightmap_img.get_width()] = rgb_to_height(pixel_value, pixel_value, pixel_value);
		}
	}

	ww = heightmap_img.get_width();
	hh = heightmap_img.get_height();

	return true;
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
}
