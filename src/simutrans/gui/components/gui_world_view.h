/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_WORLD_VIEW_H
#define GUI_COMPONENTS_GUI_WORLD_VIEW_H


#include "gui_component.h"

#include "../../dataobj/koord3d.h"
#include "../../tpl/vector_tpl.h"
#include "../../dataobj/rect.h"

class obj_t;


/**
 * Displays a little piece of the world
 */
class world_view_t : public gui_world_component_t
{
private:
	/**
	 * @brief Contains a reference to every world_view_t object.
	 *
	 * Used to allow mass invalidating of all prepared area for cases such as
	 * changing underground mode and snow levels.
	 */
	static vector_tpl<world_view_t *> view_list;

	/**
	 * @brief The prepared area of the view port.
	 *
	 * The area that has already been prepared for this view port. When the view
	 * port is moved then only the new area not already prepared will be
	 * prepared. If the view port is stationary no area will be prepared.
	 */
	rect_t prepared_rect;

	/**
	 * @brief The display area centered around the map origin.
	 *
	 * The area used by this view to display the world. It is centered around
	 * the origin but can easilly be shifted to where the actual view is
	 * focused.
	 */
	rect_t display_rect;

	vector_tpl<koord> offsets; /**< Offsets are stored. */

	sint16            raster;  /**< For this rastersize. */
	scr_size min_size;  ///< set by constructor

protected:
	virtual koord3d get_location() = 0;

	void internal_draw(scr_coord offset, obj_t const *);

	void calc_offsets(scr_size size, sint16 dy_off);

public:
	/**
	 * @brief Clears all prepared area.
	 *
	 * Set prepared rect to have no area. Used when all views must be reprepared
	 * such as changing underground mode or slice. This method is required
	 * because object views use completly separate draw logic from the main map
	 * view and must also have their prepared area invalidated for correct
	 * graphic reproduction.
	 */
	static void invalidate_all();

	world_view_t(scr_size size);

	world_view_t();

	~world_view_t();

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * resize window in response to a resize event
	 * need to recalculate the list of offsets
	 */
	void set_size(scr_size size) OVERRIDE;

	scr_size get_min_size() const OVERRIDE { return min_size; }

	scr_size get_max_size() const OVERRIDE { return min_size; }
};

#endif
