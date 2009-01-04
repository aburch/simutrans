/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
#include "curiositylist_stats_t.h"

#include "../simgraph.h"
#include "../simtypes.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simskin.h"

#include "../dings/gebaeude.h"

#include "../besch/haus_besch.h"
#include "../besch/skin_besch.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/list_button.h"


curiositylist_stats_t::curiositylist_stats_t(karte_t* w, curiositylist::sort_mode_t sortby, bool sortreverse) :
    attractions(10)
{
	welt = w;
	get_unique_attractions(sortby,sortreverse);
	set_groesse(koord(210, attractions.get_count()*(LINESPACE+1)-10));
	line_selected = 0xFFFFFFFFu;
}


class compare_curiosities
{
	public:
		compare_curiosities(curiositylist::sort_mode_t sortby_, bool reverse_) :
			sortby(sortby_),
			reverse(reverse_)
		{}

		bool operator ()(const gebaeude_t* a, const gebaeude_t* b)
		{
			int cmp;
			switch (sortby) {
				default: NOT_REACHED
				case curiositylist::by_name:
				{
					const char* a_name = translator::translate(a->get_tile()->get_besch()->get_name());
					const char* b_name = translator::translate(b->get_tile()->get_besch()->get_name());
					cmp = STRICMP(a_name, b_name);
					break;
				}

				case curiositylist::by_paxlevel:
					cmp = a->get_passagier_level() - b->get_passagier_level();
					break;
			}
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		curiositylist::sort_mode_t sortby;
		bool reverse;
};


void curiositylist_stats_t::get_unique_attractions(curiositylist::sort_mode_t sortby, bool sortreverse)
{
	const weighted_vector_tpl<gebaeude_t*>& ausflugsziele = welt->get_ausflugsziele();
	attractions.clear();
	attractions.resize(welt->get_ausflugsziele().get_count());
	for (weighted_vector_tpl<gebaeude_t*>::const_iterator i = ausflugsziele.begin(), end = ausflugsziele.end(); i != end; ++i) {
		gebaeude_t* geb = *i;
		if (geb != NULL &&
				geb->get_tile()->get_offset() == koord(0, 0) &&
				geb->get_passagier_level() != 0) {
			attractions.push_back(geb);
		}
	}
	std::sort(attractions.begin(), attractions.end(), compare_curiosities(sortby, sortreverse));
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE+1);

	line_selected = 0xFFFFFFFFu;
	if (line>=attractions.get_count()) {
		return;
	}

	gebaeude_t* geb = attractions[line];
	if (geb==NULL) {
		return;
	}

	// deperess goto button
	if(  ev->button_state>0  &&  ev->cx>0  &&  ev->cx<15  ) {
		line_selected = line;
	}

	if (IS_LEFTRELEASE(ev)) {
		if(  ev->cx>0  &&  ev->cx<15  ) {
			welt->change_world_position(geb->get_pos());
		}
		else {
			geb->zeige_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->change_world_position(geb->get_pos());
	}
} // end of function curiositylist_stats_t::infowin_event(const event_t * ev)



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::zeichnen(koord offset)
{
	image_id const arrow_right_normal = skinverwaltung_t::window_skin->get_bild(10)->get_nummer();
	const struct clip_dimension cd = display_get_clip_wh();
	const int start = cd.y-LINESPACE+1;
	const int end = cd.yy;

	static cbuffer_t buf(256);
	int yoff = offset.y;

	for (uint32 i=0; i<attractions.get_count()  &&  yoff<end; i++) {
		const gebaeude_t* geb = attractions[i];

		int xoff = offset.x+10;

		// skip invisible lines
		if(yoff<start) {
			yoff += LINESPACE+1;
			continue;
		}

		if(i!=line_selected) {
			// goto information
			display_color_img(arrow_right_normal, xoff-8, yoff, 0, false, true);
		}
		else {
			// select goto button
			display_color_img(skinverwaltung_t::window_skin->get_bild(11)->get_nummer(),
				xoff-8, yoff, 0, false, true);
		}

		buf.clear();

		// is connected? => decide on indicatorcolor
		int indicatorfarbe;
		bool post=false;
		bool pax=false;
		bool all_crowded=true;
		bool some_crowded=false;
		const planquadrat_t *plan = welt->lookup(geb->get_pos().get_2d());
		const halthandle_t *halt_list = plan->get_haltlist();
		for(  unsigned h=0;  (post&pax)==0  &&  h<plan->get_haltlist_count();  h++ ) {
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
			if (halt->get_post_enabled()) {
				post = true;
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
			indicatorfarbe = all_crowded ? COL_RED : COL_ORANGE;
		}
		else if(pax) {
			indicatorfarbe = post ? COL_TURQUOISE : COL_DARK_GREEN;
		}
		else {
			indicatorfarbe = post ? COL_BLUE : COL_YELLOW;
		}

		display_fillbox_wh_clip(xoff+7, yoff+2, INDICATOR_WIDTH, INDICATOR_HEIGHT, indicatorfarbe, true);

		// the other infos
		const unsigned char *name = (const unsigned char *)ltrim( translator::translate(geb->get_tile()->get_besch()->get_name()) );
		char short_name[256];
		int i=0, cr=0;
		for( int j=0;  j<255  &&  name[j]>='\n'  &&  cr<2;  j++  ) {
			if(name[j]>0  &&  name[j]<=' ') {
				cr++;
				if(i>0  &&  short_name[i-1]!=' ') {
					short_name[i++] = ' ';
				}
			}
			else {
				short_name[i++] = name[j];
			}
		}
		short_name[i] = 0;
		// now we have a short name ...
		buf.append(short_name);
		buf.append(" (");
		buf.append(geb->get_passagier_level());
		buf.append(") ");

		display_proportional_clip(xoff+INDICATOR_WIDTH+10+9,yoff,buf,ALIGN_LEFT,COL_BLACK,true);

		if (geb->get_tile()->get_besch()->get_extra() != 0) {
		    display_color_img(skinverwaltung_t::intown->get_bild_nr(0), xoff+INDICATOR_WIDTH+9, yoff, 0, false, false);
		}

		yoff +=LINESPACE+1;
	}
}
