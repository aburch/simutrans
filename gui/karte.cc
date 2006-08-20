#include <stdlib.h>
#ifdef _MSC_VER
#include <string.h>
#endif

#include "../simevent.h"
#include "../simplay.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simvehikel.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../boden/grund.h"
#include "../simfab.h"
#include "../simcity.h"
#include "karte.h"

#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"

#include "../boden/wege/schiene.h"
#include "../dings/leitung2.h"
#include "../dataobj/powernet.h"
#include "../tpl/slist_tpl.h"
#include "../simgraph.h"


sint32 reliefkarte_t::max_capacity=0;
sint32 reliefkarte_t::max_departed=0;
sint32 reliefkarte_t::max_arrived=0;
sint32 reliefkarte_t::max_cargo=0;
sint32 reliefkarte_t::max_convoi_arrived=0;
sint32 reliefkarte_t::max_passed=0;
sint32 reliefkarte_t::max_tourist_ziele=0;

reliefkarte_t * reliefkarte_t::single_instance = NULL;
karte_t * reliefkarte_t::welt = NULL;
reliefkarte_t::MAP_MODES reliefkarte_t::mode = MAP_TOWN;
bool reliefkarte_t::is_visible = false;

const uint8 reliefkarte_t::map_type_color[MAX_MAP_TYPE] =
{
//  7, 11, 15, 132, 23, 31, 35, 7
  248, 7, 11, 15, 132, 23, 27, 31, 35, 241, 7, 124, 31, 71, 55, 11
};

const uint8 reliefkarte_t::severity_color[MAX_SEVERITY_COLORS] =
{
  //11, 10, 9, 23, 22, 21, 15, 14, 13, 18, 19, 35
//   9, 10, 11, 21, 22, 23, 13, 14, 15, 18, 19, 35,
// 169, 168, 167, 166, 165, 164, 163, 156, 157, 158, 159, 160, 140, 139, 130, 129, 128, 123, 122, 18, 19
 169, 168, 167, 166, 165, 164, 163, 156, 157, 158, 160, 140, 15, 120, 139, 130, 129, 128, 122, 18, 19
};



uint8
reliefkarte_t::calc_severity_color(sint32 amount, sint32 max_value)
{
	if(max_value!=0) {
		// color array goes from light blue to red
		sint32 severity = (amount*MAX_SEVERITY_COLORS) / max_value;;
		if(severity>MAX_SEVERITY_COLORS) {
			severity = 20;
		}
		else if(severity < 0) {
			severity = 0;
		}
		return reliefkarte_t::severity_color[severity];
	}
	return reliefkarte_t::severity_color[0];
}



void
reliefkarte_t::setze_relief_farbe(koord k, const int color)
{
	// if map is in normal mode, set new color for map
	// otherwise do nothing
	// result: convois will not "paint over" special maps
	if (relief==NULL  ||  !welt->ist_in_kartengrenzen(k)) {
		return;
	}

	k.x *= zoom;
	k.y *= zoom;

	for (int x = 0; x < zoom; x++) {
		for (int y = 0; y < zoom; y++) {
			relief->at(k.x + x, k.y + y) = color;
		}
	}
}



void
reliefkarte_t::setze_relief_farbe_area(koord k, int areasize, uint8 color)
{
	k -= koord(areasize/2, areasize/2);
	koord p;

	for (p.x = 0; p.x<areasize; p.x++) {
		for (p.y = 0; p.y<areasize; p.y++) {
			setze_relief_farbe(k + p, color);
		}
	}
}



/**
 * Berechnet Farbe für Höhenstufe hdiff (hoehe - grundwasser).
 * @author Hj. Malthaner
 */
uint8
reliefkarte_t::calc_hoehe_farbe(const sint16 hoehe, const sint16 grundwasser)
{
	uint8 color;

	if(hoehe <= grundwasser) {
		color = COL_BLUE;
	}
	else {
		switch(hoehe-grundwasser) {
			case 0:
				color = COL_BLUE;
				break;
			case 1:
				color = 183;
				break;
			case 2:
				color = 162;
				break;
			case 3:
				color = 163;
				break;
			case 4:
				color = 164;
				break;
			case 5:
				color = 165;
				break;
			case 6:
				color = 166;
				break;
			case 7:
				color = 167;
				break;
			case 8:
				color = 161;
				break;
			case 9:
				color = 182;
				break;
			case 10:
				color = 147;
				break;
			default:
				color = 199;
				break;
		}
	}
	return color;
}



/**
 * Updated Kartenfarbe an Position k
 * @author Hj. Malthaner
 */
uint8
reliefkarte_t::calc_relief_farbe(const grund_t *gr)
{
	uint8 color = COL_BLACK;

#ifdef DEBUG_ROUTES
	/* for debug purposes only ...*/
	if(welt->ist_markiert(gr)) {
		color = COL_PURPLE;
	}else
#endif
	if(gr->gib_halt().is_bound()) {
		color = HALT_KENN;
	}
	else {
		switch(gr->gib_typ()) {
			case grund_t::brueckenboden:
				color = MN_GREY3;
				break;
			case grund_t::tunnelboden:
				color = MN_GREY0;
				break;
			case grund_t::monorailboden:
				color = MONORAIL_KENN;
				break;
			case grund_t::fundament:
				{
					// object at zero is either factory or house (or attraction ... )
					ding_t * dt = gr->obj_bei(0);
					if(dt == NULL  ||  dt->get_fabrik() == NULL) {
						color = COL_GREY3;
					}
					else {
						color = dt->get_fabrik()->gib_kennfarbe();
					}
				}
				break;
			case grund_t::wasser:
				{
					// object at zero is either factory or boat
					ding_t * dt = gr->obj_bei(0);
					if(dt==NULL  ||  dt->get_fabrik()==NULL) {
						color = COL_BLUE;	// water with boat?
					}
					else {
						color = dt->get_fabrik()->gib_kennfarbe();
					}
				}
				break;
			// normal ground ...
			default:
				if(gr->hat_wege()) {
					switch(gr->gib_weg_nr(0)->gib_typ()) {
						case weg_t::strasse: color = STRASSE_KENN; break;
						case weg_t::schiene_strab:
						case weg_t::schiene: color = SCHIENE_KENN; break;
						case weg_t::monorail: color = MONORAIL_KENN; break;
						case weg_t::wasser: color = CHANNEL_KENN; break;
						case weg_t::luft: color = RUNWAY_KENN; break;
						default:	// silence compiler!
							break;
					}
				}
				else {
					leitung_t *lt = static_cast<leitung_t *>(gr->suche_obj(ding_t::leitung));
					if(lt!=NULL) {
						color = POWERLINE_KENN;
					}
					else {
#ifndef DOUBLE_GROUNDS
						sint16 height = (gr->gib_grund_hang()&1);
#else
						sint16 height = (gr->gib_grund_hang()%3);
#endif
						color = calc_hoehe_farbe((gr->gib_hoehe()>>4)+height, welt->gib_grundwasser()>>4);
					}
				}
				break;
		}
	}
	return color;
}



void
reliefkarte_t::calc_map_pixel(const koord k)
{
	// we ignore requests, when nothing visible ...
	if(!is_visible) {
		return;
	}

	// always use to uppermost ground
	const planquadrat_t *plan=welt->lookup(k);
	const grund_t *gr=plan->gib_boden_bei(plan->gib_boden_count()-1);

	// first use ground color
	setze_relief_farbe( k, calc_relief_farbe(gr) );

	switch(mode) {
		// show passenger coverage
		// display coverage
		case MAP_PASSENGER:
			{
				halthandle_t halt = gr->gib_halt();
				if (halt.is_bound()    &&  halt->get_pax_enabled()) {
					setze_relief_farbe_area(k, (welt->gib_einstellungen()->gib_station_coverage()*2)+1, halt->gib_besitzer()->get_player_color() );
				}
			}
			break;

		// show mail coverage
		// display coverage
		case MAP_MAIL:
			{
				halthandle_t halt = gr->gib_halt();
				if (halt.is_bound()  &&  halt->get_post_enabled()) {
					setze_relief_farbe_area(k, (welt->gib_einstellungen()->gib_station_coverage()*2)+1,halt->gib_besitzer()->get_player_color() );
				}
			}
			break;

		// show usage
		case MAP_FREIGHT:
			// need to init the maximum?
			if(max_cargo==0) {
				max_cargo = 1;
				calc_map();
			}
			else {
				// now calc again ...
				sint32 cargo=0;
				if (gr->gib_weg(weg_t::strasse)) {
					cargo += gr->gib_weg(weg_t::strasse)->get_statistics(WAY_STAT_GOODS);
				}
				if (gr->gib_weg(weg_t::schiene)) {
					cargo += gr->gib_weg(weg_t::schiene)->get_statistics(WAY_STAT_GOODS);
				}
				if (gr->gib_weg(weg_t::wasser)) {
					cargo += gr->gib_weg(weg_t::wasser)->get_statistics(WAY_STAT_GOODS);
				}
				if (gr->gib_weg(weg_t::luft)) {
					cargo += gr->gib_weg(weg_t::luft)->get_statistics(WAY_STAT_GOODS);
				}
				if(cargo>0) {
					if(cargo>max_cargo) {
						max_cargo = cargo;
					}
					setze_relief_farbe(k, calc_severity_color(cargo, max_cargo));
				}
			}
			break;

		// show station status
		case MAP_STATUS:
			{
				halthandle_t halt = gr->gib_halt();
				if (halt.is_bound()  && (halt->gib_besitzer()==welt->get_active_player()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) ) {
					setze_relief_farbe_area(k, 3, halt->gib_status_farbe());
				}
			}
			break;

		// show frequency of convois visiting a station
		case MAP_SERVICE:
			// need to init the maximum?
			if(max_convoi_arrived==0) {
				max_convoi_arrived = 1;
				calc_map();
			}
			else {
				halthandle_t halt = gr->gib_halt();
				if (halt.is_bound()   && (halt->gib_besitzer()==welt->get_active_player()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) ) {
					// get number of last month's arrived convois
					sint32 arrived = halt->get_finance_history(1, HALT_CONVOIS_ARRIVED);
					if(arrived>max_convoi_arrived) {
						max_convoi_arrived = arrived;
					}
					setze_relief_farbe_area(k, 3, calc_severity_color(arrived, max_convoi_arrived));
				}
			}
			break;

		// show traffic (=convois/month)
		case MAP_TRAFFIC:
			// need to init the maximum?
			if(max_passed==0) {
				max_passed = 1;
				calc_map();
			}
			else {
				sint32 passed=0;
				if (gr->gib_weg(weg_t::strasse)) {
					passed += gr->gib_weg(weg_t::strasse)->get_statistics(WAY_STAT_CONVOIS);
				}
				if (gr->gib_weg(weg_t::schiene)) {
					passed += gr->gib_weg(weg_t::schiene)->get_statistics(WAY_STAT_CONVOIS);
				}
				if (gr->gib_weg(weg_t::wasser)) {
					passed += gr->gib_weg(weg_t::wasser)->get_statistics(WAY_STAT_CONVOIS);
				}
				if (gr->gib_weg(weg_t::luft)) {
					passed += gr->gib_weg(weg_t::luft)->get_statistics(WAY_STAT_CONVOIS);
				}
				if (passed>0) {
					if(passed>max_passed) {
						max_passed = passed;
					}
					setze_relief_farbe_area(k, 1, calc_severity_color(passed, max_passed));
				}
			}
			break;

		// show sources of passengers
		case MAP_ORIGIN:
			// need to init the maximum?
			if(max_arrived==0) {
				max_arrived = 1;
				calc_map();
			}
			else if (gr->gib_halt().is_bound()) {
				halthandle_t halt = gr->gib_halt();
				// only show player's haltestellen
				if (halt->gib_besitzer()==welt->get_active_player()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) {
					 sint32 arrived=halt->get_finance_history(1, HALT_DEPARTED)-halt->get_finance_history(1, HALT_ARRIVED);
					if(arrived>max_arrived) {
						max_arrived = arrived;
					}
					const uint8 color = calc_severity_color( arrived, max_arrived );
					setze_relief_farbe_area(k, 3, color);
				}
			}
			break;

		// show destinations for passengers
		case MAP_DESTINATION:
			// need to init the maximum?
			if(max_departed==0) {
				max_departed = 1;
				calc_map();
			}
			else {
				halthandle_t halt = gr->gib_halt();
				if (halt.is_bound()   && (halt->gib_besitzer()==welt->get_active_player()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) ) {
					sint32 departed=halt->get_finance_history(1, HALT_ARRIVED)-halt->get_finance_history(1, HALT_DEPARTED);
					if(departed>max_departed) {
						max_departed = departed;
					}
					const uint8 color = calc_severity_color( departed, max_departed );
					setze_relief_farbe_area(k, 3, color );
				}
			}
			break;

		// show waiting goods
		case MAP_WAITING:
			{
				halthandle_t halt = gr->gib_halt();
				if (halt.is_bound()   && (halt->gib_besitzer()==welt->get_active_player()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) ) {
					const uint8 color = calc_severity_color(halt->get_finance_history(0, HALT_WAITING), halt->get_capacity() );
					setze_relief_farbe_area(k, 3, color );
				}
			}
			break;

		// show tracks: white: no electricity, red: electricity, yellow: signal
		case MAP_TRACKS:
			// show track
			if (gr->gib_weg(weg_t::schiene)) {
				const schiene_t * sch = dynamic_cast<const schiene_t *> (gr->gib_weg(weg_t::schiene));
				if(sch->is_electrified()) {
					setze_relief_farbe(k, COL_RED);
				}
				else {
					setze_relief_farbe(k, COL_WHITE);
				}
				// show signals
				if(sch->has_sign()) {
					setze_relief_farbe(k, COL_YELLOW);
				}
			}
			break;

		// show max speed (if there)
		case MAX_SPEEDLIMIT:
			{
				sint32 speed=gr->get_max_speed();
				if(speed) {
					setze_relief_farbe(k, calc_severity_color(gr->get_max_speed(), 450));
				}
			}
			break;

		// find power lines
		case MAP_POWERLINES:
			// need to init the maximum?
			if(max_capacity==0) {
				max_capacity = 1;
				calc_map();
			}
			else {
				leitung_t *lt = static_cast<leitung_t *>(gr->suche_obj(ding_t::leitung));
				if(lt!=NULL) {
					sint32 capacity=lt->get_net()->get_capacity();
					if(capacity>max_capacity) {
						max_capacity = capacity;
					}
					setze_relief_farbe(k, calc_severity_color(capacity,max_capacity) );
				}
			}
			break;

		case MAP_DEPOT:
			if(gr->gib_depot()) {
				// offset of one to avoid
				setze_relief_farbe_area(k-koord(1,1), 3, gr->gib_depot()->gib_typ() );
			}
			break;

		default:
			break;
	}
}



void
reliefkarte_t::calc_map()
{
	// redraw the map
	koord k;
	for(k.y=0; k.y<welt->gib_groesse_y(); k.y++) {
		for(k.x=0; k.x<welt->gib_groesse_x(); k.x++) {
			calc_map_pixel(k);
		}
	}

	// mark all vehicle positions
	for(unsigned i=0;  i<welt->get_convoi_count();  i++ ) {
		convoihandle_t cnv = welt->get_convoi_array().get(i);
		vehikel_t *v;
		for( uint16 i=0;  (v=cnv->gib_vehikel(i))!=0;  i++ ) {
			setze_relief_farbe( v->gib_pos().gib_2d(), VEHIKEL_KENN );
		}
	}

	// since we do iterate the tourist info list, this must be done here
	// find tourist spots
	if(mode==MAP_TOURIST) {
		// need to init the maximum?
		if(max_tourist_ziele==0) {
			max_tourist_ziele = 1;
			calc_map();
		}
		const weighted_vector_tpl<gebaeude_t *> &ausflugsziele = welt->gib_ausflugsziele();
		for( unsigned i=0;  i<ausflugsziele.get_count();  i++ ) {
			if(max_tourist_ziele<ausflugsziele.at(i)->gib_passagier_level()) {
				max_tourist_ziele = ausflugsziele.at(i)->gib_passagier_level();
			}
		}
		{
			for( unsigned i=0;  i<ausflugsziele.get_count();  i++ ) {
				setze_relief_farbe_area(ausflugsziele.get(i)->gib_pos().gib_2d(), 7, calc_severity_color(ausflugsziele.get(i)->gib_passagier_level(),max_tourist_ziele) );
			}
		}
		return;
	}

	// since we do iterate the factory info list, this must be done here
	// find tourist spots
	if(mode==MAP_FACTORIES) {
		slist_iterator_tpl <fabrik_t *> iter (welt->gib_fab_list());
		while(iter.next()) {
			setze_relief_farbe_area(iter.get_current()->gib_pos().gib_2d(), 9, COL_BLACK );
			setze_relief_farbe_area(iter.get_current()->gib_pos().gib_2d(), 7, iter.get_current()->gib_kennfarbe() );
		}
		return;
	}
}



// from now on public routines



reliefkarte_t::reliefkarte_t()
{
	relief = NULL;
	zoom = 1;
	mode = MAP_TOWN;
	fab = 0;
}



reliefkarte_t::~reliefkarte_t()
{
	if(relief != NULL) {
		delete relief;
	}
}



reliefkarte_t *
reliefkarte_t::gib_karte()
{
	if(single_instance == NULL) {
		single_instance = new reliefkarte_t();
	}
	return single_instance;
}



void
reliefkarte_t::setze_welt(karte_t *welt)
{
    this->welt = welt;			// Welt fuer display_win() merken

    if(relief) {
		delete relief;
		relief = NULL;
    }

    if(welt) {
    		koord size = koord(welt->gib_groesse_x()*zoom, welt->gib_groesse_y()*zoom);
DBG_MESSAGE("reliefkarte_t::setze_welt()","welt size %i,%i",size.x,size.y);
		relief = new array2d_tpl<unsigned char> (size.x,size.y);
		setze_groesse(size);

		max_capacity = max_departed = max_arrived = max_cargo = max_convoi_arrived = max_passed = max_tourist_ziele = 0;
	}
}



void
reliefkarte_t::set_mode (MAP_MODES new_mode)
{
	mode = new_mode;
	calc_map();
}



void
reliefkarte_t::neuer_monat()
{
	if(mode>MAP_TOWN) {
		calc_map();
	}
}




// these two are the only gui_container specific routines


// handle event
void
reliefkarte_t::infowin_event(const event_t *ev)
{
	// get factory under mouse cursor
	fab = fabrik_t::gib_fab(welt, koord(ev->mx / zoom, ev->my / zoom));

	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) {
		welt->set_follow_convoi( convoihandle_t() );
		welt->setze_ij_off(koord(ev->mx/zoom-8, ev->my/zoom-8)); // Offset empirisch ermittelt !
	}

	if(IS_RIGHTRELEASE(ev) || IS_WHEELUP(ev) || IS_WHEELDOWN(ev)) {

		if (ev->ev_key_mod&1 || IS_WHEELDOWN(ev)) {
			zoom == 1 ? zoom = MAX_MAP_ZOOM : zoom /= 2;
		}
		else {
			zoom >= MAX_MAP_ZOOM ? zoom = 1 : zoom *= 2;
		}

		setze_welt(welt);
		calc_map();
	}
}



// helper function for redraw: factory connections
void
reliefkarte_t::draw_fab_connections(const fabrik_t * fab, uint8 colour, koord pos) const
{
	const koord fabpos = koord(pos.x + fab->pos.x * zoom, pos.y + fab->pos.y * zoom);
	const vector_tpl <koord> &lieferziele = event_get_last_control_shift()&1 ? fab->get_suppliers() : fab->gib_lieferziele();
	for(uint32 i=0; i<lieferziele.get_count(); i++) {
		const koord lieferziel = lieferziele.get(i);
		const fabrik_t * fab2 = fabrik_t::gib_fab(welt, lieferziel);
		if (fab2) {
			const koord end = pos + lieferziel * zoom;
			display_direct_line(fabpos.x+zoom, fabpos.y+zoom, end.x+zoom, end.y+zoom, colour);

			const koord boxpos = end + koord(10, 0);
			display_fillbox_wh_clip(end.x, end.y, 3 * zoom, 3 * zoom, ((welt->gib_zeit_ms() >> 10) & 1) == 0 ? COL_RED : COL_WHITE, true);

			const char * name = translator::translate(fab2->gib_name());
			display_ddd_proportional_clip(boxpos.x, boxpos.y, proportional_string_width(name)+8, 0, 5, COL_WHITE, name, true);
		}
	}
}



// draw the map
void
reliefkarte_t::zeichnen(koord pos) const
{
	if(relief==NULL) {
		return;
	}

	display_fillbox_wh_clip(pos.x, pos.y, 4000, 4000, COL_BLACK, true);
	display_array_wh(pos.x, pos.y, relief->get_width(), relief->get_height(), relief->to_array());

	int xpos = (welt->gib_ij_off().x+8)*zoom;
	int ypos = (welt->gib_ij_off().y+8)*zoom;

	// zoom faktor
	const int zf = zoom * get_zoom_factor();

	// zoom/resize "selection box" in map
	display_direct_line(pos.x+xpos+12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos-12*zf, COL_WHITE);
	display_direct_line(pos.x+xpos-12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos-12*zf, COL_WHITE);
	display_direct_line(pos.x+xpos+12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos+12*zf, COL_WHITE);
	display_direct_line(pos.x+xpos-12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos+12*zf, COL_WHITE);

	// if we do not do this here, vehicles would erase the won name
	if(mode==MAP_TOWN) {
		const weighted_vector_tpl <stadt_t *> * staedte = welt->gib_staedte();

		for(unsigned i=0; i<staedte->get_count(); i++) {
			const stadt_t *stadt = staedte->get(i);

			koord p = stadt->gib_pos();
			const char * name = stadt->gib_name();

			int w = proportional_string_width(name);
			p.x = max( pos.x+(p.x*zoom)-(w/2), pos.x );
			display_proportional_clip( p.x, pos.y+p.y*zoom, name, ALIGN_LEFT, COL_WHITE, true);
		}
	}

	if (fab) {
		draw_fab_connections(fab, event_get_last_control_shift()&1 ? COL_RED : COL_WHITE, pos);
		const koord fabpos = koord(pos.x + fab->pos.x * zoom, pos.y + fab->pos.y * zoom);
		const koord boxpos = fabpos + koord(10, 0);
		const char * name = translator::translate(fab->gib_name());
		display_ddd_proportional_clip(boxpos.x, boxpos.y, proportional_string_width(name)+8, 0, 10, COL_WHITE, name, true);
	}
}
