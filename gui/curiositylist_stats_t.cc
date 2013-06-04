/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * Where curiosity (attractions) stats are calculated for list dialog
 * @author Hj. Malthaner
 */

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

#include "gui_frame.h"


curiositylist_stats_t::curiositylist_stats_t(karte_t* w, curiositylist::sort_mode_t sortby, bool sortreverse) :
	welt(w)
{
	get_unique_attractions(sortby,sortreverse);
	recalc_size();
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


void curiositylist_stats_t::get_unique_attractions(curiositylist::sort_mode_t sb, bool sr)
{
	const weighted_vector_tpl<gebaeude_t*>& ausflugsziele = welt->get_ausflugsziele();

	sortby = sb;
	sortreverse = sr;

	attractions.clear();
	last_world_curiosities = ausflugsziele.get_count();
	attractions.resize(last_world_curiosities);

	FOR(weighted_vector_tpl<gebaeude_t*>, const geb, ausflugsziele) {
		if (geb != NULL &&
				geb->get_first_tile() == geb &&
				geb->get_passagier_level() != 0) {
			attractions.insert_ordered( geb, compare_curiosities(sortby, sortreverse) );
		}
	}
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool curiositylist_stats_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE+1);

	line_selected = 0xFFFFFFFFu;
	if (line>=attractions.get_count()) {
		return false;
	}

	gebaeude_t* geb = attractions[line];
	if (geb==NULL) {
		return false;
	}

	// un-press goto button
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
	return false;
} // end of function curiositylist_stats_t::infowin_event(const event_t * ev)


void curiositylist_stats_t::recalc_size()
{
	// show_scroll_x==false ->> groesse.x not important ->> no need to calc text pixel length
	set_groesse( koord(210, attractions.get_count() * (LINESPACE+1) ) );
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::zeichnen(koord offset)
{
	clip_dimension const cd = display_get_clip_wh();
	const int start = cd.y-LINESPACE+1;
	const int end = cd.yy;

	static cbuffer_t buf;
	int yoff = offset.y;

	if(  last_world_curiosities != welt->get_ausflugsziele().get_count()  ) {
		// some deleted/ added => resort
		get_unique_attractions( sortby, sortreverse );
		recalc_size();
	}

	uint32 sel = line_selected;
	FORX(vector_tpl<gebaeude_t*>, const geb, attractions, yoff += LINESPACE + 1) {
		if (yoff >= end) break;

		int xoff = offset.x+10;

		// skip invisible lines
		if (yoff < start) continue;

		// goto button
		image_id const img = sel-- != 0 ? button_t::arrow_right_normal : button_t::arrow_right_pushed;
		display_color_img(img, xoff - 8, yoff, 0, false, true);

		buf.clear();

		// is connected? => decide on indicatorfarbe (indicator color)
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

		display_fillbox_wh_clip(xoff+7, yoff+2, D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT, indicatorfarbe, true);

		// the other infos
		const unsigned char *name = (const unsigned char *)ltrim( translator::translate(geb->get_tile()->get_besch()->get_name()) );
		char short_name[256];
		char* dst = short_name;
		int    cr = 0;
		for( int j=0;  j<255  &&  cr<10;  j++  ) {
			if(name[j]>0  &&  name[j]<=' ') {
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
		buf.printf("%s (%d)", short_name, geb->get_passagier_level());

		display_proportional_clip(xoff+D_INDICATOR_WIDTH+10+9,yoff,buf,ALIGN_LEFT,COL_BLACK,true);

		if (geb->get_tile()->get_besch()->get_extra() != 0) {
		    display_color_img(skinverwaltung_t::intown->get_bild_nr(0), xoff+D_INDICATOR_WIDTH+9, yoff, 0, false, false);
		}
	}
}
