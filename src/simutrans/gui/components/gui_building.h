/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_BUILDING_H
#define GUI_COMPONENTS_GUI_BUILDING_H


#include "gui_component.h"
#include "gui_action_creator.h"
#include "../../tpl/vector_tpl.h"

class building_desc_t;
class gui_image_t;

/**
 * Component to draw a multi-tile building
 */
class gui_building_t : public gui_component_t, public gui_action_creator_t
{
	const building_desc_t* desc;
	int layout;
	vector_tpl<gui_image_t*> images;
	// bounding box of images
	scr_coord tl, br;
public:
	gui_building_t(const building_desc_t* d=NULL, int r=0);

	~gui_building_t();

	void init(const building_desc_t* d, int r);

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }

	void draw(scr_coord offset) OVERRIDE;

	bool infowin_event(const event_t *ev) OVERRIDE;
};

#endif
