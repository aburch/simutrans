/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "curiositylist_stats.h"

#include "components/gui_image.h"
#include "../simtypes.h"
#include "../simcolor.h"
#include "../world/simworld.h"
#include "../simhalt.h"
#include "../simskin.h"

#include "../obj/gebaeude.h"

#include "../descriptor/building_desc.h"
#include "../descriptor/skin_desc.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer.h"


curiositylist::sort_mode_t curiositylist_stats_t::sortby = curiositylist::by_name;
bool curiositylist_stats_t::sortreverse = false;
static karte_ptr_t welt;

bool curiositylist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb )
{
	const curiositylist_stats_t* ca = dynamic_cast<const curiositylist_stats_t*>(aa);
	const curiositylist_stats_t* cb = dynamic_cast<const curiositylist_stats_t*>(bb);
	const gebaeude_t* a = ca->attraction;
	const gebaeude_t* b = cb->attraction;

	int cmp = 0;
	switch (sortby) {
		default: NOT_REACHED

		case curiositylist::by_paxlevel:
			if (a->get_passagier_level() != b->get_passagier_level()) {
				cmp = a->get_passagier_level() - b->get_passagier_level();
				break;
			}
		// fall-through
		case curiositylist::by_name:
		{
			const char* a_name = translator::translate(a->get_tile()->get_desc()->get_name());
			const char* b_name = translator::translate(b->get_tile()->get_desc()->get_name());
			cmp = STRICMP(a_name, b_name);
			break;
		}
	}
	return sortreverse ? cmp > 0 : cmp < 0;
}



curiositylist_stats_t::curiositylist_stats_t(gebaeude_t *att)
{
	attraction = att;
	// pos button
	set_table_layout(4,1);
	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
	b->set_targetpos3d(attraction->get_pos());
	// indicator bar
	add_component(&indicator);
	indicator.set_max_size(scr_size(D_INDICATOR_WIDTH,D_INDICATOR_HEIGHT));
	// city attraction images
	if (attraction->get_tile()->get_desc()->get_extra() != 0) {
		new_component<gui_image_t>(skinverwaltung_t::intown->get_image_id(0), 0, ALIGN_NONE, true);
	}
	else {
		new_component<gui_empty_t>();
	}
	// name
	gui_label_buf_t *l = new_component<gui_label_buf_t>();
	l->buf().printf("%s (%d)", get_text(), attraction->get_passagier_level());
	l->update();
}


const char* curiositylist_stats_t::get_text() const
{
	const unsigned char *name = (const unsigned char *)ltrim( translator::translate(attraction->get_tile()->get_desc()->get_name()) );
	static char short_name[256];
	char* dst = short_name;
	int    cr = 0;
	for( int j=0;  name[j]>0  &&  j<255  &&  cr<10;  j++  ) {
		if(name[j]<=' ') {
			cr++;
			if(  name[j]<32  ) {
				break;
			}
			if (dst != short_name && dst[-1] != ' ') {
				*dst++ = ' ';
			}
		}
		else {
			*dst++ = name[j];
		}
	}
	*dst = '\0';
	// now we have a short name ...
	return short_name;
}


bool curiositylist_stats_t::is_valid() const
{
	return world()->get_attractions().is_contained(attraction);
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 */
bool curiositylist_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);
	if(  !swallowed  &&  IS_LEFTRELEASE(ev)  ) {
		attraction->show_info();
		swallowed = true;
	}
	return swallowed;
}


/**
 * Draw the component
 */
void curiositylist_stats_t::draw(scr_coord offset)
{
		// is connected? => decide on indicatorfarbe (indicator color)
		PIXVAL indicatorfarbe;
		bool mail=false;
		bool pax=false;
		bool all_crowded=true;
		bool some_crowded=false;
		const planquadrat_t *plan = welt->access(attraction->get_pos().get_2d());
		const halthandle_t *halt_list = plan->get_haltlist();
		for(  unsigned h=0;  (mail&pax)==0  &&  h<plan->get_haltlist_count();  h++ ) {
			halthandle_t halt = halt_list[h];
			if (halt->get_pax_enabled()) {
				pax = true;
				if (halt->get_pax_unhappy() > 40) {
					some_crowded |= true;
				}
				else {
					all_crowded = false;
				}
			}
			if (halt->get_mail_enabled()) {
				mail = true;
				if (halt->get_pax_unhappy() > 40) {
					some_crowded |= true;
				}
				else {
					all_crowded = false;
				}
			}
		}
		// now decide on color
		if(some_crowded) {
			indicatorfarbe = color_idx_to_rgb(all_crowded ? COL_RED : COL_ORANGE);
		}
		else if(pax) {
			indicatorfarbe = color_idx_to_rgb(mail ? COL_TURQUOISE : COL_DARK_GREEN);
		}
		else {
			indicatorfarbe = color_idx_to_rgb(mail ? COL_BLUE : COL_YELLOW);
		}

	indicator.set_color(indicatorfarbe);
	gui_aligned_container_t::draw(offset);
}
