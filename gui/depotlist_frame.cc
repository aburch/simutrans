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
	gotopos.set_typ(button_t::posbutton_automatic);
	gotopos.set_targetpos(depot->get_pos().get_2d());
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
	last_depot_count = 0;

	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(4,0);
	{
		sortedby.clear_elements();
		for (int i = 0; i < SORT_MODES; i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(depotlist_stats_t::sort_mode);
		sortedby.add_listener(this);
		add_component(&sortedby);

		sort_asc.init(button_t::arrowup_state, "");
		sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
		sort_asc.add_listener(this);
		sort_asc.pressed = depotlist_stats_t::reverse;
		add_component(&sort_asc);

		sort_desc.init(button_t::arrowdown_state, "");
		sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
		sort_desc.add_listener(this);
		sort_desc.pressed = !depotlist_stats_t::reverse;
		add_component(&sort_desc);
		new_component<gui_margin_t>(LINESPACE);
	}
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
bool depotlist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		depotlist_stats_t::sort_mode = max(0, v.i);
		scrolly.sort(0);
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		depotlist_stats_t::reverse = !depotlist_stats_t::reverse;
		scrolly.sort(0);
		sort_asc.pressed = depotlist_stats_t::reverse;
		sort_desc.pressed = !depotlist_stats_t::reverse;
	}
	return true;
}


void depotlist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(slist_tpl<depot_t*>, const depot, depot_t::get_depot_list()) {
		if( depot->get_owner() == player ) {
			scrolly.new_component<depotlist_stats_t>( depot );
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
