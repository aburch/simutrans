/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "depotlist_frame.h"
#include "gui_theme.h"

#include "../player/simplay.h"

#include "../obj/depot.h"
#include "../simskin.h"
#include "../world/simworld.h"

#include "../dataobj/translator.h"

#include "../descriptor/skin_desc.h"


depotlist_stats_t::depotlist_stats_t(depot_t *d)
{
	this->depot = d;
	// pos button
	set_table_layout(3,1);
	gotopos.set_typ(button_t::posbutton_automatic);
	gotopos.set_targetpos3d(depot->get_pos());
	add_component(&gotopos);
	// now add all specific tabs
	switch(  d->get_waytype()  ) {
	case maglev_wt:
		waytype_symbol.set_image( skinverwaltung_t::maglevhaltsymbol->get_image_id(0), true );
		break;
	case monorail_wt:
		waytype_symbol.set_image( skinverwaltung_t::monorailhaltsymbol->get_image_id(0), true );
		break;
	case track_wt:
		waytype_symbol.set_image( skinverwaltung_t::zughaltsymbol->get_image_id(0), true );
		break;
	case tram_wt:
		waytype_symbol.set_image( skinverwaltung_t::tramhaltsymbol->get_image_id(0), true );
		break;
	case narrowgauge_wt:
		waytype_symbol.set_image( skinverwaltung_t::narrowgaugehaltsymbol->get_image_id(0), true );
		break;
	case road_wt:
		waytype_symbol.set_image( skinverwaltung_t::autohaltsymbol->get_image_id(0), true );
		break;
	case water_wt:
		waytype_symbol.set_image( skinverwaltung_t::schiffshaltsymbol->get_image_id(0), true );
		break;
	case air_wt:
		waytype_symbol.set_image( skinverwaltung_t::airhaltsymbol->get_image_id(0), true );
		break;
	default: ;
	}
	add_component(&waytype_symbol);
	add_component(&label);
	update_label();
}


void depotlist_stats_t::update_label()
{
	cbuffer_t &buf = label.buf();
	buf.append( translator::translate(depot->get_name()) );
	buf.printf( " %s ", depot->get_pos().get_2d().get_fullstr() );
	int cnvs = depot->convoi_count();
	if( cnvs == 0 ) {
//		buf.append( translator::translate( "no convois" ) );
	}
	else if( cnvs == 1 ) {
		buf.append( translator::translate( "1 convoi" ) );
		buf.append(" ");
	}
	else {
		buf.printf( translator::translate( "%d convois" ), cnvs );
		buf.append(" ");
	}
	int vhls = depot->get_vehicle_list().get_count();
	if( vhls == 0 ) {
		buf.append( translator::translate( "Keine Einzelfahrzeuge im Depot" ) );
	}
	else if( vhls == 1 ) {
		buf.append( translator::translate( "1 Einzelfahrzeug im Depot" ) );
	}
	else {
		buf.printf( translator::translate( "%d Einzelfahrzeuge im Depot" ), vhls );
	}
	label.update();
}


void depotlist_stats_t::set_size(scr_size size)
{
	gui_aligned_container_t::set_size(size);
	label.set_size(scr_size(get_size().w - label.get_pos().x, label.get_size().h));
}


bool depotlist_stats_t::is_valid() const
{
	return depot_t::get_depot_list().is_contained(depot);
}


bool depotlist_stats_t::infowin_event(const event_t * ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if(  !swallowed  &&  IS_LEFTRELEASE(ev)  ) {
		depot->show_info();
		swallowed = true;
	}
	return swallowed;
}


void depotlist_stats_t::draw(scr_coord pos)
{
	update_label();

	gui_aligned_container_t::draw(pos);
}


bool depotlist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	const depotlist_stats_t* fa = dynamic_cast<const depotlist_stats_t*>(aa);
	const depotlist_stats_t* fb = dynamic_cast<const depotlist_stats_t*>(bb);
	// good luck with mixed lists
	assert(fa != NULL  &&  fb != NULL);
	depot_t *a=fa->depot, *b=fb->depot;

	int cmp = a->convoi_count() - b->convoi_count();
	if( cmp == 0 ) {
		cmp = a->get_vehicle_list().get_count() - b->get_vehicle_list().get_count();
	}
	if (cmp == 0) {
		cmp = koord_distance( a->get_pos(), koord( 0, 0 ) ) - koord_distance( b->get_pos(), koord( 0, 0 ) );
		if( cmp == 0 ) {
			cmp = a->get_pos().x - b->get_pos().x;
		}
	}
	return cmp > 0;
}


depotlist_frame_t::depotlist_frame_t(player_t *player) :
	gui_frame_t( translator::translate("dp_title"), player ),
	scrolly(gui_scrolled_list_t::windowskin, depotlist_stats_t::compare)
{
	this->player = player;
	last_depot_count = 0;

	set_table_layout(1,0);

	scrolly.set_maximize(true);

	tabs.init_tabs(&scrolly);
	tabs.add_listener(this);
	add_component(&tabs);

	fill_list();

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


depotlist_frame_t::depotlist_frame_t() :
	gui_frame_t(translator::translate("dp_title"), NULL),
	scrolly(gui_scrolled_list_t::windowskin, depotlist_stats_t::compare)
{
	player = NULL;
	last_depot_count = 0;

	set_table_layout(1, 0);

	scrolly.set_maximize(true);

	tabs.init_tabs(&scrolly);
	tabs.add_listener(this);
	add_component(&tabs);

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool depotlist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &tabs) {
		fill_list();
	}
	return true;
}


void depotlist_frame_t::fill_list()
{
	scrolly.clear_elements();
	for(depot_t* const depot : depot_t::get_depot_list()) {
		if(  depot->get_owner() == player  ) {
			if(  tabs.get_active_tab_index() == 0  ||  depot->get_waytype() == tabs.get_active_tab_waytype()  ) {
				scrolly.new_component<depotlist_stats_t>(depot);
			}
		}
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());

	last_depot_count = depot_t::get_depot_list().get_count();
}


void depotlist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  depot_t::get_depot_list().get_count() != last_depot_count  ) {
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}

void depotlist_frame_t::rdwr(loadsave_t* file)
{
	scr_size size = get_windowsize();
	uint8 player_nr = player ? player->get_player_nr() : 0;

	file->rdwr_byte(player_nr);
	size.rdwr(file);
	tabs.rdwr(file);
	scrolly.rdwr(file);

	if (file->is_loading()) {
		player = welt->get_player(player_nr);
		win_set_magic(this, magic_depotlist + player_nr);
		fill_list();
		set_windowsize(size);
	}
}
