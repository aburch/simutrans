/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_VIEWPORT_H
#define DISPLAY_VIEWPORT_H


#include "../simtypes.h"
#include "scr_coord.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../dataobj/rect.h"
#include "../convoihandle.h"

class karte_t;
class grund_t;

/**
 * @brief Map viewing camera.
 * Describes a camera, looking at our world. Contains various routines to handle the coordinates conversion.
 *
 * We handle four level of coordinates, described here:
 * - map_coord: It's a three dimensional coordinate, located in the simulated world. Implemented with koord3d, most game objects are positioned like this.
 * - map2d_coord: Two-dimensional coordinate, located in the simulated world, they are assumed z=0. Implemented with koord.
 * - viewport_coord: Two-dimensional coordinate, with our camera transformations applied, where 0,0 is the center of screen.
 * - screen_coord: Final transformation, exact coordinate on the output frame buffer.
 *
 * Diagram of a viewport screen, each viewport_coord corresponds to one of the diamonds:
 * --------------------
 * |\/\/\/\/\/\/\/\/\/|
 * |/\/\/\/\/\/\/\/\/\|
 * |\/\/\/\/\/\/\/\/\/|
 * |/\/\/\/\/\/\/\/\/\|
 * |\/\/\/\/\/\/\/\/\/|
 * |/\/\/\/\/\/\/\/\/\|
 * |\/\/\/\/\/\/\/\/\/|
 * --------------------
 */
class viewport_t
{
public:
	/**
	 * @brief The prepared area of the view port.
	 *
	 * The area that has already been prepared for this view port. When the view
	 * port is moved then only the new area not already prepared will be
	 * prepared. If the view port is stationary no area will be prepared.
	 */
	rect_t prepared_rect;

private:
	/// The simulated world this view is associated to.
	karte_t *world;
	/**
	 * @name Camera position
	 *       This variables are related to the view camera position.
	 * @{
	 */

	/**
	 * Current view position, expressed in 2D map coordinates.
	 * @see x_off
	 * @see y_off
	 */
	koord ij_off;

	sint16 x_off; //!< Fine scrolling x offset. Units in pixels.
	sint16 y_off; //!< Fine scrolling y offset. Units in pixels.

	/**
	 * This is the current offset for getting from tile to screen.
	 * @note Extra offset, if added to ij_off it will give us the 2D coordinate of the tile on the *BOTTOM-RIGHT* of screen.
	 */
	koord view_ij_off;

	/**
	 * For performance reasons, we cache ( (0,0)-ij_off-view_ij_off ) here
	 */
	koord cached_aggregated_off;

	/**
	 * @}
	 */

	sint16 cached_disp_width;  ///< Cached window width
	sint16 cached_disp_height; ///< Cached window height
	sint16 cached_img_size;    ///< Cached base raster image size

	/**
	 * Sets current ij offsets of this viewport, depends of its proportions and the zoom level.
	 */
	void set_viewport_ij_offset( const koord &k );

	/**
	 * The current convoi to follow.
	 */
	convoihandle_t follow_convoi;

	/**
	 * Converts map_coord to map2d_coord actually used for main view.
	 */
	koord get_map2d_coord( const koord3d& ) const;

	/**
	 * Returns the viewport_coord coordinates of the requested map2d_coord coordinate.
	 */
	koord get_viewport_coord( const koord& coord ) const;

	/**
	 * Updates the transformation vector we have cached.
	 */
	void update_cached_values() {
		cached_aggregated_off = -ij_off-view_ij_off;
	}

public:

	/**
	 * @name Coordinate transformations.
	 *       This methods handle the projection from the map coordinates, to 2D, screen coordinates.
	 * @{
	 */

	/**
	 * Takes a in-game 3D coordinates and returns which coordinate in screen corresponds to it.
	 * @param pos 3D in-map coordinate.
	 * @param off offset to add to the final coordinates, in pixels (will be scaled to zoom level).
	 */
	scr_coord get_screen_coord( const koord3d& pos, const koord& off = koord(0,0) ) const;

	/**
	 * Scales the 2D dimensions, expressed on base raster size pixels, to the current viewport.
	 */
	scr_coord scale_offset(const koord &value);

	/**
	 * Checks a 3d in-map coordinate, to know if it's centered on the current viewport.
	 * @param pos 3D map coordinate to check.
	 * @return true if the requested map coordinates are on the center of the viewport, false otherwise.
	 */
	bool is_on_center(const koord3d &pos) const {
		return ( (get_x_off() | get_y_off()) == 0  &&
			get_world_position() == get_map2d_coord( pos ) );
	}

	/**
	 * @}
	 */

	/**
	 * @name Camera position
	 *       This methods are related to the view camera position.
	 * @{
	 */

	/**
	 * Viewpoint in tile coordinates. i,j coordinate of the tile in center of screen.
	 */
	koord get_world_position() const { return ij_off; }

	/**
	 * Offset from tile on center to tile in top-left corner of screen.
	 */
	koord get_viewport_ij_offset() const { return view_ij_off; }

	/**
	 * Set center viewport position.
	 */
	void change_world_position( koord ij, sint16 x=0, sint16 y=0 );

	/**
	 * Set center viewport position, taking height into account
	 */
	void change_world_position( const koord3d& ij );

	/**
	 * Set center viewport position, placing a in-game koord3d under the desired screen position.
	 * @param pos map position to consider.
	 * @param off extra offset.
	 * @param sc screen position "pos" should be under.
	 */
	void change_world_position(const koord3d& pos, const koord& off, scr_coord sc);

	/**
	 * Fine offset within the viewport tile.
	 */
	int get_x_off() const {return x_off;}

	/**
	 * Fine offset within the viewport tile.
	 */
	void set_x_off(sint16 value) {x_off = value;}

	/**
	 * Fine offset within the viewport tile.
	 */
	int get_y_off() const {return y_off;}

	/**
	 * Fine offset within the viewport tile.
	 */
	void set_y_off(sint16 value) {y_off = value;}

	/**
	 * @}
	 */

	/**
	 * @name Ray tracing
	 *       This methods are related to the act of finding a in-map entity, given a screen_cord. Used for player interaction, mainly.
	 * @{
	 */

	/**
	 * Searches for the ground_t that's under the requested screen position.
	 * @param screen_pos Screen coordinates to check for.
	 * @param intersect_grid Special case for the lower/raise tool, will return a limit border tile if we are on the south/east border of screen.
	 * @param found_i out parameter, i-coordinate of the found tile. It's necessary because it might point to a grid position that doesn't map to a real in-map coordinate, on border tiles (south and east border).
	 * @param found_j out parameter, j-coordinate of the found tile. It's necessary because it might point to a grid position that doesn't map to a real in-map coordinate, on border tiles (south and east border).
	 * @return the grund_t that's under the desired screen coordinate. NULL if we are outside map or we can't find it.
	 */
	grund_t* get_ground_on_screen_coordinate(scr_coord screen_pos, sint32 &found_i, sint32 &found_j, const bool intersect_grid=false) const;

	/**
	 * Gets a new world position, under the requested screen coordinates. Used to move the cursor.
	 * @param screen_pos Screen position to check. Input parameter.
	 * @param grid_coordinates indicates if this function is to check against the map tiles, or the grid of heights. Input parameter.
	 * @return koord3d::invalid if no position exists under the requested coordinate, a 3d koord directly under it otherwise.
	 */
	koord3d get_new_cursor_position(const scr_coord &screen_pos, bool grid_coordinates);

	/**
	 * @}
	 */

	/**
	 * @name Convoi following
	 *       This methods are related to the mechanics of a viewport follow a vehicle.
	 * @{
	 */

	/**
	 * Function for following a convoi on the map give an unbound handle to unset.
	 */
	void set_follow_convoi(convoihandle_t cnv) { follow_convoi = cnv; }

	/**
	 * Returns the convoi this viewport is following (if any).
	 */
	convoihandle_t get_follow_convoi() const { return follow_convoi; }

	/**
	 * @}
	 */

	/**
	 * This class caches the viewport dimensions, and the base image size. On zoom or viewport resize, call this
	 * method to force its recalculation.
	 */
	void metrics_updated();

	/**
	 * (A better description is needed here)
	 * Operations needed on map rotation.
	 */
	void rotate90( sint16 y_size );

	viewport_t( karte_t *world, const koord ij_off = koord::invalid, sint16 x_off = 0, sint16 y_off = 0 );
};

#endif
