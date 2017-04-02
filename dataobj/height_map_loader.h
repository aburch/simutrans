/* code for loading heightmaps */

#ifndef load_height_file_h
#define load_height_file_h

#include "../simtypes.h"
#include "environment.h"

class height_map_loader_t {
public:
	height_map_loader_t(bool height_map_conversion_version);

	/**
	 * Reads height data from 8 or 25 bit bmp or ppm files.
	 * @return Either pointer to heightfield (use delete [] for it) or NULL.
	 */
	bool get_height_data_from_file( const char *filename, sint8 groundwater, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values );

private:
	inline sint8 rgb_to_height( const int r, const int g, const int b ) {
		const sint16 h0 = (r*2+g*3+b);
		if(  !height_map_conversion_version_new  ) {
			// old style
			if(  env_t::pak_height_conversion_factor == 2  ) {
				// was: return (((r*2+g*3+b)/4 - 224) & 0xFFF8)/8;
				return h0/32 - 28;
			}
			else {
				// was: return (( ((r*2+g*3+b)/4 - 224)) & 0xFFF0)/16;
				return h0/64 - 14;
			}
		}
		else {
			// new style, heights more spread out
			if(  env_t::pak_height_conversion_factor == 2  ) {
				return h0/24 - 34;
			}
			else {
				return h0/48 - 18;
			}
		}
	}

	const bool height_map_conversion_version_new;
};

#endif
