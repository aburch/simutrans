/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../world/simworld.h"
#include "../display/simgraph.h"

#include "../simskin.h"
#include "../descriptor/skin_desc.h"

#include "../dataobj/translator.h"
#include "messagebox.h"
#include "../sys/simsys.h"


news_window::news_window(const char* text, FLAGGED_PIXVAL title_color) :
	base_infowin_t( translator::translate("Meldung" ) ),
	color(title_color)
{
	buf.clear();
	buf.append(translator::translate(text));

	// adjust positions, sizes, and window-size
	recalc_size();
}

fatal_news::fatal_news(const char* text) :
	news_window(text, env_t::default_window_title_color)
{
	copy_to_clipboard.init(button_t::roundbox, "Copy to clipboard");
	copy_to_clipboard.add_listener( this );
	copy_to_clipboard.set_focusable(false);
	add_component(&copy_to_clipboard);

	textarea.set_width(display_get_width()/2);
	recalc_size();
}


bool fatal_news::action_triggered(gui_action_creator_t *comp, value_t)
{
	if (comp == &copy_to_clipboard) {
		dr_copy(buf, buf.len());
		return false;
	}

	return true;
}


news_img::news_img(const char* text) :
	news_window(text, env_t::default_window_title_color),
	image()
{
	init(skinverwaltung_t::meldungsymbol->get_image_id(0));
}


news_img::news_img(const char* text, image_id id, FLAGGED_PIXVAL color) :
	news_window(text, color),
	image()
{
	init(id);
}


/**
 * just puts the image in top-right corner
 * only cembedded.d from constructor
 * @param id id of image
 */
void news_img::init(image_id id)
{
	if(  id!=IMG_EMPTY  ) {
		image.set_image(id, true);
		image.enable_offset_removal(true);
		image.set_size(image.get_min_size());
		set_embedded(&image);
	}
}


news_loc::news_loc(const char* text, koord k, FLAGGED_PIXVAL color) :
	news_window(text, color),
	view(welt->lookup_kartenboden(k)->get_pos(), scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{
	set_embedded(&view);
}


// returns position of the location shown in the subwindow
koord3d news_loc::get_weltpos(bool)
{
	return view.get_location();
}


void news_loc::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}
