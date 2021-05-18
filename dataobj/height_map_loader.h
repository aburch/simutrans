/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_HEIGHT_MAP_LOADER_H
#define DATAOBJ_HEIGHT_MAP_LOADER_H


#include "../simtypes.h"
#include "environment.h"


/**
 * Loads height data from BMP or PPM files.
 */
class height_map_loader_t
{
public:
	height_map_loader_t(sint8 min_allowed_height, sint8 max_allowed_height, env_t::height_conversion_mode conv_mode);

	/**
	 * Reads height data from 8 or 24 bit bmp or ppm files.
	 *
	 * @param filename the file to load the height data from.
	 * @param[out] hfield 2d array of height values.
	 * @param[out] ww width of @p hfield. On failure, this value is undefined.
	 * @param[out] hh height of @p hfield. On failure, this value is undefined.
	 * @param update_only_values When true, do not allocate the height field; instead, only update @p ww and @p hh
	 *
	 * @return true on success, false on failure. On success, @p hfield contains
	 * the height field data (must be free()'d by the caller unless @p update_only_values is set).
	 * On failure, @p hfield is set to NULL (Does not need to be free()'d)
	 */
	bool get_height_data_from_file( const char *filename, sint8 groundwater, sint8 *&hfield, sint16 &ww, sint16 &hh, bool update_only_values );

private:
	sint8 rgb_to_height( const int r, const int g, const int b );

private:
	const sint8 min_allowed_height;
	const sint8 max_allowed_height;
	const env_t::height_conversion_mode conv_mode;
};

#endif
