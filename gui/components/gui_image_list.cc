/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../simdebug.h"
#include "gui_image_list.h"
#include "../../display/simgraph.h"
#include "../../simevent.h"
#include "../../simcolor.h"


gui_image_list_t::gui_image_list_t(vector_tpl<image_data_t*> *images) :
	grid(16, 16),
	placement(16, 16)
{
	this->images = images;
	player_nr = 0;
	max_rows = -1;
	max_width = -1;
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 */
bool gui_image_list_t::infowin_event(const event_t *ev)
{
	int sel_index = index_at(scr_coord(0,0)-pos, ev->mx, ev->my);
	if(  sel_index != -1  &&  (IS_LEFTDBLCLK(ev)  ||  IS_LEFTRELEASE(ev))  ) {
		value_t p;
		p.i = sel_index;
		call_listeners( p );
		return true;
	}
	return false;
}



int gui_image_list_t::index_at(scr_coord parent_pos, int xpos, int ypos) const
{
	xpos -= parent_pos.x + pos.x + BORDER;
	ypos -= parent_pos.y + pos.y + BORDER;

	if(xpos>=0  &&  ypos>=0  &&  xpos<size.w-2*BORDER  &&  ypos < size.h-2*BORDER) {
		const int columns = (size.w - 2 * BORDER) / grid.x;

		const int column = xpos / grid.x;
		const int row = ypos / grid.y;

		const unsigned int index = row * columns + column;

		if (index < images->get_count()  &&  (*images)[index]->image != IMG_EMPTY) {
			return index;
		}
	}
	return -1;
}




void gui_image_list_t::draw(scr_coord parent_pos)
{
	const int columns = (size.w - 2 * BORDER) / grid.x;

	// sel_index should come from infowin_event, but it is not sure?
	int sel_index = index_at(parent_pos, get_mouse_x(), get_mouse_y());

	// Show available wagon types
	int xmin = parent_pos.x + pos.x + BORDER;
	int ymin = parent_pos.y + pos.y + BORDER;
	int xmax = xmin + columns * grid.x;
	int xpos = xmin;
	int ypos = ymin;

	FOR(vector_tpl<image_data_t*>, const& iptr, *images) {
		image_data_t const& idata = *iptr;
		if(idata.count>=0) {

			// display mark
			if(idata.lcolor!=EMPTY_IMAGE_BAR) {
				display_fillbox_wh_clip_rgb( xpos + 1, ypos + grid.y - 5, grid.x/2 - 1, 4, idata.lcolor, true);
			}
			if(idata.rcolor!=EMPTY_IMAGE_BAR) {
				display_fillbox_wh_clip_rgb( xpos + grid.x/2, ypos + grid.y - 5, grid.x - grid.x/2 - 1, 4, idata.rcolor, true);
			}
			if (sel_index-- == 0) {
				display_ddd_box_clip_rgb(xpos, ypos, grid.x, grid.y, color_idx_to_rgb(MN_GREY4), color_idx_to_rgb(MN_GREY0));
			}

			// Get image data
			scr_coord_val x,y,w,h;
			display_get_base_image_offset( idata.image, &x, &y, &w, &h );

			// calculate image offsets
			y = -y + (grid.y-h) - 6; // align to bottom mark
			x = -x + 2;              // Add 2 pixel margin
			//display_base_img(idata.image, xpos + placement.x, ypos + placement.y, player_nr, false, true);
			display_base_img(idata.image, xpos + x, ypos + y, player_nr, false, true);

			// If necessary, display a number:
			if(idata.count > 0) {
				char text[20];

				sprintf(text, "%d", idata.count);

				// Let's make a black background to ensure visibility
				for(int iy = -3; iy < 0; iy++) {
					for(int ix = 1; ix < 4; ix++) {
						display_proportional_clip_rgb(xpos + ix, ypos + iy, text, ALIGN_LEFT, color_idx_to_rgb(COL_BLACK), true);
					}
				}
				// Display the number white on black
				display_proportional_clip_rgb(xpos + 2, ypos - 2, text, ALIGN_LEFT, color_idx_to_rgb(COL_WHITE), true);
			}
		}
		// advance x, y to next position
		xpos += grid.x;
		if(xpos == xmax) {
			xpos = xmin;
			ypos += grid.y;
		}
	}
}


scr_size gui_image_list_t::get_min_size() const { return get_max_size(); }

scr_size gui_image_list_t::get_max_size() const
{
	if (max_rows > 0) {
		// this many images per column
		sint32 cols = (images->get_count() + max_rows - 1) / max_rows;
		return scr_size(cols*grid.x + 2*BORDER, max_rows*grid.y + 2*BORDER);
	}
	else if (max_width > 0) {
		sint32 cols = (max_width - 2*BORDER) / grid.x;
		sint32 rows = (images->get_count() + cols - 1) / cols;
		return scr_size(cols*grid.x + 2*BORDER, rows*grid.y + 2*BORDER);

	}
	return scr_size((images->get_count()+1)*grid.x + 2*BORDER, grid.y + 2*BORDER);
}
