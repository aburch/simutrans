// ****************** List of all depots ************************


#include "depotlist_frame.h"
#include "gui_theme.h"
#include "../simdepot.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "../descriptor/skin_desc.h"

enum sort_mode_t { by_coord, by_waytype, by_vehicle, SORT_MODES };

int depotlist_stats_t::sort_mode = by_waytype;
bool depotlist_stats_t::reverse = false;


depotlist_stats_t::depotlist_stats_t(depot_t *d)
{
	this->depot = d;
	// pos button
	set_table_layout(3,1);
	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
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
	}
	add_component(&waytype_symbol);
	b->set_targetpos(depot->get_pos().get_2d());
	add_component(&label);
	update_label();
}


void depotlist_stats_t::update_label()
{
	cbuffer_t &buf = label.buf();
	buf.append(depot->get_name());
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

	int cmp;
	switch( sort_mode ) {
	default:
	case by_coord:
		cmp = 0;
		break;

	case by_waytype:
		cmp = a->get_waytype() - b->get_waytype();
		break;

	case by_vehicle:
		cmp = a->convoi_count() - b->convoi_count();
		if( cmp == 0 ) {
			cmp = a->get_vehicle_list().get_count() - b->get_vehicle_list().get_count();
		}
		break;

	}
	if (cmp == 0) {
		cmp = koord_distance( a->get_pos(), koord( 0, 0 ) ) - koord_distance( b->get_pos(), koord( 0, 0 ) );
		if( cmp == 0 ) {
			cmp = a->get_pos().x - b->get_pos().x;
		}
	}
	return reverse ? cmp > 0 : cmp < 0;
}




static const char *sort_text[SORT_MODES] = {
	"koord",
	"waytype",
	"vehicles stored"
};

depotlist_frame_t::depotlist_frame_t(player_t *player) :
	gui_frame_t( translator::translate("dp_title"), player ),
	scrolly(gui_scrolled_list_t::windowskin, depotlist_stats_t::compare)
{
	this->player = player;

	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(2,0);
	sortedby.init(button_t::roundbox, sort_text[depotlist_stats_t::sort_mode]);
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, depotlist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	sorteddir.add_listener(this);
	add_component(&sorteddir);
	end_table();

	add_component(&scrolly);
	fill_list();

	set_resizemode(diagonal_resize);
	scrolly.set_maximize(true);
	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool depotlist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		depotlist_stats_t::sort_mode = (depotlist_stats_t::sort_mode + 1) % SORT_MODES;
		sortedby.set_text(sort_text[depotlist_stats_t::sort_mode]);
		scrolly.sort(0);
	}
	else if(comp == &sorteddir) {
		depotlist_stats_t::reverse = !depotlist_stats_t::reverse;
		sorteddir.set_text( depotlist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		scrolly.sort(0);
	}
	return true;
}


void depotlist_frame_t::fill_list()
{
	scrolly.clear_elements();
	// all vehikels stored in depot not part of a convoi
	FOR(slist_tpl<depot_t*>, const depot, depot_t::get_depot_list()) {
		if( depot->get_owner() == player ) {
			scrolly.new_component<depotlist_stats_t>( depot );
		}
	}
	scrolly.sort(0);
//	scrolly.set_size(scrolly.get_size());
}


void depotlist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  depot_t::get_depot_list().get_count() != (uint32)scrolly.get_count()  ) {
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}
