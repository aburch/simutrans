/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "curiositylist_stats_t.h"

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simtypes.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simskin.h"

#include "../obj/gebaeude.h"

#include "../descriptor/building_desc.h"
#include "../descriptor/skin_desc.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "gui_frame.h"
#include "simwin.h"

#include "../simcity.h"

#define L_DIALOG_WIDTH (210)

curiositylist_stats_t::curiositylist_stats_t(curiositylist::sort_mode_t sortby, bool sortreverse, bool own_network)
{
	get_unique_attractions(sortby,sortreverse, own_network);
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
					const char* a_name = translator::translate(a->get_tile()->get_desc()->get_name());
					const char* b_name = translator::translate(b->get_tile()->get_desc()->get_name());
					cmp = STRICMP(a_name, b_name);
					break;
				}

				case curiositylist::by_paxlevel:
					cmp = a->get_adjusted_visitor_demand() - b->get_adjusted_visitor_demand();
					break;

				case curiositylist::by_pax_arrived:
					int a_arrive = a->get_passengers_succeeded_commuting() == 65535 ? a->get_passengers_succeeded_visiting() : a->get_passengers_succeeded_visiting() + a->get_passengers_succeeded_commuting();
					int b_arrive = b->get_passengers_succeeded_commuting() == 65535 ? b->get_passengers_succeeded_visiting() : b->get_passengers_succeeded_visiting() + b->get_passengers_succeeded_commuting();
					cmp = a_arrive - b_arrive;
					break;
			}
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		curiositylist::sort_mode_t sortby;
		bool reverse;
};


void curiositylist_stats_t::get_unique_attractions(curiositylist::sort_mode_t sb, bool sr, bool own_network)
{
	const weighted_vector_tpl<gebaeude_t*>& world_attractions = welt->get_ausflugsziele();

	sortby = sb;
	sortreverse = sr;
	filter_own_network = own_network;

	attractions.clear();

	FOR(weighted_vector_tpl<gebaeude_t*>, const geb, world_attractions) {
		// own network filter
		if (filter_own_network && !geb->is_within_players_network(welt->get_active_player(), goods_manager_t::INDEX_PAS)) {
			continue;
		}

		if (geb != NULL &&
				geb->get_first_tile() == geb &&
				geb->get_adjusted_visitor_demand() != 0) {
			attractions.insert_ordered( geb, compare_curiosities(sortby, sortreverse) );
		}
	}
	attractions.resize(attractions.get_count());
	set_size(scr_size(L_DIALOG_WIDTH, attractions.get_count()*LINESPACE + D_V_SPACE));
}


/**
 * Events werden hiermit an die GUI-components
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
			welt->get_viewport()->change_world_position(geb->get_pos());
		}
		else {
			geb->show_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->get_viewport()->change_world_position(geb->get_pos());
	}
	return false;
} // end of function curiositylist_stats_t::infowin_event(const event_t * ev)


void curiositylist_stats_t::recalc_size()
{
	// show_scroll_x==false ->> size.w not important ->> no need to calc text pixel length
	set_size( scr_size(L_DIALOG_WIDTH, attractions.get_count() * (LINESPACE+1) ) );
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void curiositylist_stats_t::draw(scr_coord offset)
{
	clip_dimension const cd = display_get_clip_wh();
	const int start = cd.y-LINESPACE+1;
	const int end = cd.yy;

	static cbuffer_t buf;
	int yoff = offset.y;

	if(  last_world_curiosities != welt->get_ausflugsziele().get_count()  ) {
		// some deleted/ added => resort
		get_unique_attractions( sortby, sortreverse, filter_own_network );
		recalc_size();
	}

	uint32 sel = line_selected;
	FORX(vector_tpl<gebaeude_t*>, const geb, attractions, yoff += LINESPACE + 1) {
		if (yoff >= end) {
			break;
		}

		int xoff = offset.x+2;

		// skip invisible lines
		if (yoff < start) {
			continue;
		}

		// goto button
		bool selected = sel==0  ||  welt->get_viewport()->is_on_center( geb->get_pos() );
		display_img_aligned( gui_theme_t::pos_button_img[ selected ], scr_rect( xoff, yoff, 14, LINESPACE ), ALIGN_CENTER_V | ALIGN_CENTER_H, true );
		sel --;

		buf.clear();

		// is connected?
		enum{ no_networks = 0, someones_network=1, own_network=2 };
		uint8 mail = 0;
		uint8 pax  = 0;
		uint8 pax_crowded = 0;
		const planquadrat_t *plan = welt->access(geb->get_pos().get_2d());
		const nearby_halt_t *halt_list = plan->get_haltlist();
		for(  unsigned h=0;  h < plan->get_haltlist_count();  h++ ) {
			halthandle_t halt = halt_list[h].halt;
			if (halt->get_pax_enabled()) {
				if (halt->has_available_network(welt->get_active_player(), goods_manager_t::INDEX_PAS)) {
					pax |= own_network;
					if (halt->get_pax_unhappy() > 40) {
						pax_crowded |= own_network;
					}
				}
				else{
					pax |= someones_network;
					if (halt->get_pax_unhappy() > 40) {
						pax_crowded |= someones_network;
					}
				}
			}
			if (halt->get_mail_enabled()) {
				if (halt->has_available_network(welt->get_active_player(), goods_manager_t::INDEX_MAIL)) {
					mail |= own_network;
				}
				else {
					mail |= someones_network;
				}
			}
		}

		xoff += D_POS_BUTTON_WIDTH+2;
		if (pax & own_network) {
			display_color_img(skinverwaltung_t::passengers->get_image_id(0), xoff, yoff + 2, 0, false, false);
		}
		else if (pax & someones_network) {
			display_img_blend(skinverwaltung_t::passengers->get_image_id(0), xoff, yoff + 2, TRANSPARENT25_FLAG, false, false);
		}
		xoff += 9; // symbol width
		if (mail & own_network) {
			display_color_img(skinverwaltung_t::mail->get_image_id(0), xoff, yoff + 2, 0, false, false);
		}
		else if (mail & someones_network) {
			display_img_blend(skinverwaltung_t::mail->get_image_id(0), xoff, yoff + 2, TRANSPARENT25_FLAG, false, false);
		}
		xoff += 9; // symbol width

		if (geb->get_tile()->get_desc()->get_extra() != 0) {
		    display_color_img_with_tooltip(skinverwaltung_t::intown->get_image_id(0), xoff, yoff, 0, false, false, welt->get_city(geb->get_pos().get_2d())->get_name());
		}
		xoff += 9 + 2; // symbol width + margin

		// the other infos
		const unsigned char *name = (const unsigned char *)ltrim( translator::translate(geb->get_tile()->get_desc()->get_name()) );
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
		buf.printf("%s (", short_name);
		xoff += display_proportional_clip_rgb(xoff, yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
		buf.clear();
		buf.printf("%d", geb->get_passengers_succeeded_commuting() == 65535 ? geb->get_passengers_succeeded_visiting() : geb->get_passengers_succeeded_visiting() + geb->get_passengers_succeeded_commuting());
		xoff += display_proportional_clip_rgb(xoff, yoff, buf, ALIGN_LEFT, pax_crowded ? color_idx_to_rgb(COL_OVERCROWD-1) : SYSCOL_TEXT, true);
		buf.clear();
		buf.printf("/%d)", geb->get_adjusted_visitor_demand());
		xoff += display_proportional_clip_rgb(xoff,yoff,buf,ALIGN_LEFT,SYSCOL_TEXT,true) + D_H_SPACE;

		// Pakset must have pax evaluation symbols to show overcrowding symbol
		if (pax_crowded & own_network && skinverwaltung_t::pax_evaluation_icons) {
			display_color_img(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), xoff, yoff, 0, false, false);
		}
		else if (pax_crowded & someones_network && skinverwaltung_t::pax_evaluation_icons) {
			display_img_blend(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), xoff, yoff, TRANSPARENT25_FLAG, false, false);
		}

		if(  win_get_magic( (ptrdiff_t)geb )  ) {
			display_blend_wh_rgb( offset.x+D_POS_BUTTON_WIDTH+D_H_SPACE, yoff, size.w, LINESPACE, SYSCOL_TEXT, 25 );
		}
	}
}
