/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../../world/simworld.h"
#include "../../builder/wegbauer.h"
#include "../../descriptor/way_desc.h"

#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/pakset_manager.h"

#include "../../utils/cbuffer.h"

#include "runway.h"

const way_desc_t *runway_t::default_runway=NULL;


runway_t::runway_t() : schiene_t()
{
	set_desc(default_runway);
}


runway_t::runway_t(loadsave_t *file) : schiene_t()
{
	rdwr(file);
}


void runway_t::info(cbuffer_t & buf) const
{
	schiene_t::info(buf);

	cbuffer_t reserved;
	reserved.printf( "%s %i\n", translator::translate("waiting"), reservations.get_count() );
	buf.append( reserved );
}


bool runway_t::can_reserve(koord3d ask_takeoff)
{
	if (takeoff == ask_takeoff  ||  reservations.get_count() == 0) {
		return true;
	}
	return false;
}


void runway_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "runway_t" );

	weg_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		const way_desc_t *desc = way_builder_t::get_desc(bname);
		int old_max_speed=get_max_speed();
		if (desc == NULL) {
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if (desc == NULL) {
				desc = way_builder_t::weg_search(air_wt, old_max_speed > 0 ? old_max_speed : 20, 0, (systemtype_t)(old_max_speed > 250));
				if (desc == NULL) {
					desc = default_runway;
					pakset_manager_t::add_missing_paks(bname, MISSING_WAY);
					if (desc == NULL) {
						dbg->fatal("runway_t::rdwr()", "No runway available");
					}
				}
			}
		}
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		set_desc(desc);
	}
}

#ifdef DEBUG_RUNWAYS
#ifdef MULTI_THREAD
void runway_t::display_after(int xpos, int ypos, const sint8 clip_num) const
{
	weg_t::display_after(xpos, ypos+64, clip_num);
#else
void obj_t::display_after(int xpos, int ypos, bool) const
{
	::display_after(int xpos, int ypos, const sint8 clip_num);
#endif
	if (show_reservations && !reservations.empty()) {
		char str[20];
		sprintf(str, "%d", reservations.get_count());
		gfx->draw_text(xpos + gfx->get_current_tile_raster_width() / 2, ypos + gfx->get_current_tile_raster_width() - LINESPACE, str, ALIGN_CENTER_H, gfx->palette_lookup(COL_GREEN), true);
	}
}


FLAGGED_PIXVAL runway_t::get_outline_colour() const
{
	if (show_reservations && !reservations.empty()) {
		FLAGGED_PIXVAL sc = schiene_t::get_outline_colour();
		if (sc) {
			return sc;
		}
		welt->set_dirty();
		return TRANSPARENT75_FLAG | OUTLINE_FLAG | gfx->palette_lookup(COL_BROWN);
	}
	return 0;
}

#endif
