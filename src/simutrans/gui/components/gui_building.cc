/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_building.h"
#include "gui_image.h"
#include "../../descriptor/building_desc.h"
#include "../../tpl/vector_tpl.h"


gui_building_t::gui_building_t(const building_desc_t* d, int r)
{
	init(d, r);
}

void gui_building_t::init(const building_desc_t* d, int r)
{
	tl = scr_coord(0,0);
	br = scr_coord(0,0);
	desc = d;
	layout = r;
	clear_ptr_vector(images);

	if (desc == 0) {
		return;
	}

	scr_coord_val rw4 = get_base_tile_raster_width()/4;
	for(int i=0; i< desc->get_x(layout); i++) {
		for(int j=0; j < desc->get_y(layout); j++) {
			image_id id = desc->get_tile(layout,i,j)->get_background(0,0,0);
			if (id == IMG_EMPTY) {
				id = desc->get_tile(layout,i,j)->get_foreground(0,0);
			}
			if (id != IMG_EMPTY) {
				gui_image_t *img = new gui_image_t(id);
				img->set_pos(scr_coord( 2*(i-j)*rw4, (i+j)*rw4 ));
				images.append(img);
				// compute bounding box
				scr_coord_val x,y,w,h;
				display_get_base_image_offset( id, &x, &y, &w, &h );
				tl.clip_rightbottom(img->get_pos() + scr_coord(x,y));
				br.clip_lefttop(img->get_pos() + scr_coord(x+w,y+h));
			}
		}
	}
}


gui_building_t::~gui_building_t()
{
	clear_ptr_vector(images);
}


scr_size gui_building_t::get_min_size() const
{
	scr_coord diff = br - tl;
	return scr_size(diff.x, diff.y);
}


void gui_building_t::draw(scr_coord offset)
{
	scr_coord pos = get_pos() + offset - tl;
	for(gui_image_t* img : images) {
		img->draw(pos);
	}
}


bool gui_building_t::infowin_event(const event_t *ev) {
	if (IS_LEFTRELEASE(ev)) {
		call_listeners( (value_t)layout);
	}
	return false;
}
