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



// converts karte koordinates to screen corrdinates
inline void
reliefkarte_t::welt_to_screen( koord &k ) const
{
	if(rotate45) {
		// 45 rotate view
		sint16 x = welt->gib_groesse_y()/2 + (k.x-k.y)/2;
		k.y = (k.x+k.y)/2;
		k.x = x;
	}
	k.x = (k.x*zoom_out)/zoom_in;
	k.y = (k.y*zoom_out)/zoom_in;
}


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

	welt_to_screen(k);

	for (int x = 0; x < zoom_out; x++) {
		for (int y = 0; y < zoom_out; y++) {
			relief->at(k.x + x, k.y + y) = color;
		}
	}
}



void
reliefkarte_t::setze_relief_farbe_area(koord k, int areasize, uint8 color)
{
	k -= koord(areasize/2, areasize/2);
	koord p;

	welt_to_screen(k);
	if(rotate45) {
		for (p.x = 0; p.x<areasize; p.x++) {
			for (p.y = 0; p.y<areasize; p.y++) {
				koord pos = koord( k.x+(p.x-p.y)/2, k.y+(p.x+p.y)/2 );
				setze_relief_farbe( pos, color);
			}
		}
	}
	else {
		for (p.x = 0; p.x<areasize; p.x++) {
			for (p.y = 0; p.y<areasize; p.y++) {
				setze_relief_farbe(k + p, color);
			}
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
					ding_t * dt = gr->first_obj();
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
					ding_t * dt = gr->first_obj();
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
						case road_wt: color = STRASSE_KENN; break;
						case tram_wt:
						case track_wt: color = SCHIENE_KENN; break;
						case monorail_wt: color = MONORAIL_KENN; break;
						case water_wt: color = CHANNEL_KENN; break;
						case air_wt: color = RUNWAY_KENN; break;
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
						color = calc_hoehe_farbe((gr->gib_hoehe()/Z_TILE_STEP)+height, welt->gib_grundwasser()/Z_TILE_STEP);
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
			else if(gr->hat_wege()) {
				// now calc again ...
				sint32 cargo=0;

				// maximum two ways for one ground
				const weg_t *w=gr->gib_weg_nr(0);
				if(w) {
					cargo = w->get_statistics(WAY_STAT_GOODS);
					const weg_t *w=gr->gib_weg_nr(1);
					if(w) {
						cargo += w->get_statistics(WAY_STAT_GOODS);
					}
					if(cargo>0) {
						if(cargo>max_cargo) {
							max_cargo = cargo;
						}
						setze_relief_farbe(k, calc_severity_color(cargo, max_cargo));
					}
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
			else if(gr->hat_wege()) {
				// now calc again ...
				sint32 passed=0;

				// maximum two ways for one ground
				const weg_t *w=gr->gib_weg_nr(0);
				if(w) {
					passed = w->get_statistics(WAY_STAT_CONVOIS);
					const weg_t *w=gr->gib_weg_nr(1);
					if(w) {
						passed += w->get_statistics(WAY_STAT_CONVOIS);
					}
					if (passed>0) {
						if(passed>max_passed) {
							max_passed = passed;
						}
						setze_relief_farbe_area(k, 1, calc_severity_color(passed, max_passed));
					}
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
			if (gr->hat_weg(track_wt)) {
				const schiene_t * sch = dynamic_cast<const schiene_t *> (gr->gib_weg(track_wt));
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
	// size cahnge due to zoom?
	koord size = rotate45 ?
			koord( ((welt->gib_groesse_y()/2+zoom_in-1)*zoom_out)/zoom_in+((welt->gib_groesse_x()/2+zoom_in-1)*zoom_out)/zoom_in, ((welt->gib_groesse_y()/2+zoom_in-1)*zoom_out)/zoom_in+((welt->gib_groesse_x()/2+zoom_in-1)*zoom_out)/zoom_in ) :
			koord( ((welt->gib_groesse_x()+zoom_in-1)*zoom_out)/zoom_in, ((welt->gib_groesse_y()+zoom_in-1)*zoom_out)/zoom_in );
	if(relief->get_width()!=size.x  ||  relief->get_height()!=size.y) {
		delete relief;
		relief = new array2d_tpl<unsigned char> (size.x,size.y);
		setze_groesse(size);
	}
	if(rotate45) {
		memset( (void *)(relief->to_array()), COL_BLACK, size.x*size.y );
	}

	// redraw the map
	koord k;
	for(k.y=0; k.y<welt->gib_groesse_y(); k.y++) {
		for(k.x=0; k.x<welt->gib_groesse_x(); k.x++) {
			calc_map_pixel(k);
		}
	}

	// mark all vehicle positions
	for(unsigned i=0;  i<welt->get_convoi_count();  i++ ) {
		convoihandle_t cnv = welt->get_convoi_array()[i];
		for (uint16 i = 0; i < cnv->gib_vehikel_anzahl(); i++) {
			setze_relief_farbe(cnv->gib_vehikel(i)->gib_pos().gib_2d(), VEHIKEL_KENN);
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
			int pax = ausflugsziele[i]->gib_passagier_level();
			if (max_tourist_ziele < pax) max_tourist_ziele = pax;
		}
		{
			for( unsigned i=0;  i<ausflugsziele.get_count();  i++ ) {
				const gebaeude_t* g = ausflugsziele[i];
				setze_relief_farbe_area(g->gib_pos().gib_2d(), 7, calc_severity_color(g->gib_passagier_level(), max_tourist_ziele));
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
	zoom_out = 1;
	zoom_in = 1;
	rotate45 = false;
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
		koord size = koord( (welt->gib_groesse_x()*zoom_out)/zoom_in+1, (welt->gib_groesse_y()*zoom_out)/zoom_in+1 );
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
	koord k( (ev->mx*zoom_in)/zoom_out, (ev->my*zoom_in)/zoom_out );
	if(rotate45) {
		k.x = (k.x-welt->gib_groesse_y()/2) + k.y;
		k.y = (2*k.y) - k.x;
	}

	// get factory under mouse cursor
	fab = fabrik_t::gib_fab(welt, k );

	// recenter
	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) {
		welt->set_follow_convoi( convoihandle_t() );
		int z = 0;
		if(welt->ist_in_kartengrenzen(k)) {
			z = welt->min_hgt(k);
		}
		welt->setze_ij_off(koord3d(k,z));
	}
}



// helper function for redraw: factory connections
void
reliefkarte_t::draw_fab_connections(const fabrik_t * fab, uint8 colour, koord pos) const
{
	koord fabpos = fab->pos.gib_2d();
	welt_to_screen( fabpos );
	fabpos += pos;
	const vector_tpl <koord> &lieferziele = event_get_last_control_shift()&1 ? fab->get_suppliers() : fab->gib_lieferziele();
	for(uint32 i=0; i<lieferziele.get_count(); i++) {
		koord lieferziel = lieferziele[i];
		const fabrik_t * fab2 = fabrik_t::gib_fab(welt, lieferziel);
		if (fab2) {
			welt_to_screen( lieferziel );
			const koord end = lieferziel+pos;
			display_direct_line(fabpos.x, fabpos.y, end.x, end.y, colour);
			display_fillbox_wh_clip(end.x, end.y, 3, 3, ((welt->gib_zeit_ms() >> 10) & 1) == 0 ? COL_RED : COL_WHITE, true);

			const koord boxpos = end + koord(10, 0);
			const char * name = translator::translate(fab2->gib_name());
			display_ddd_proportional_clip(boxpos.x, boxpos.y, proportional_string_width(name)+8, 0, 5, COL_WHITE, name, true);
		}
	}
}



// draw the map
void
reliefkarte_t::zeichnen(koord pos)
{
	if(relief==NULL) {
		return;
	}

	display_fillbox_wh_clip(pos.x, pos.y, 4000, 4000, COL_BLACK, true);
	display_array_wh(pos.x, pos.y, relief->get_width(), relief->get_height(), relief->to_array());

	// if we do not do this here, vehicles would erase the won name
	if(mode==MAP_TOWN) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->gib_staedte();

		for (uint i = 0; i < staedte.get_count(); i++) {
			const stadt_t* stadt = staedte[i];

			koord p = stadt->gib_pos();
			const char * name = stadt->gib_name();

			int w = proportional_string_width(name);
			p.x = max( pos.x+((p.x*zoom_out)/zoom_in)-(w/2), pos.x );
			display_proportional_clip( p.x, pos.y+(p.y*zoom_out)/zoom_in, name, ALIGN_LEFT, COL_WHITE, true);
		}
	}

	// zoom/resize "selection box" in map
	// this must be rotated by 45 degree (sin45=cos45=0,5*sqrt(2)=0.707...)
	const sint16 raster=get_tile_raster_width();

	// calculate and draw the rotated coordinates
	if(rotate45) {
		// straight cursor
		const koord diff = koord( ((display_get_width()/raster)*zoom_out)/zoom_in, (((display_get_height())/(raster))*zoom_out)/zoom_in );
		koord ij = welt->gib_ij_off();
		welt_to_screen( ij );
		ij += pos;
		display_direct_line( ij.x-diff.x, ij.y-diff.y, ij.x+diff.x, ij.y-diff.y, COL_YELLOW);
		display_direct_line( ij.x+diff.x, ij.y-diff.y, ij.x+diff.x, ij.y+diff.y, COL_YELLOW);
		display_direct_line( ij.x+diff.x, ij.y+diff.y, ij.x-diff.x, ij.y+diff.y, COL_YELLOW);
		display_direct_line( ij.x-diff.x, ij.y+diff.y, ij.x-diff.x, ij.y-diff.y, COL_YELLOW);
	}
	else {
		// rotate cursor
		const koord diff = koord( ((display_get_width()/raster)*zoom_out*0.707106)/zoom_in, (((display_get_height())/(raster))*zoom_out*0.707106)/zoom_in );
		koord ij = welt->gib_ij_off();
		welt_to_screen( ij );
		ij += pos;
		koord view[4];
		view[0] = ij + koord( -diff.x+diff.y, -diff.x-diff.y );
		view[1] = ij + koord( -diff.x-diff.y, -diff.x+diff.y );
		view[2] = ij + koord( diff.x-diff.y, diff.x+diff.y );
		view[3] = ij + koord( diff.x+diff.y, diff.x-diff.y );
		for(  int i=0;  i<4;  i++  ) {
			display_direct_line( view[i].x, view[i].y, view[(i+1)%4].x, view[(i+1)%4].y, COL_YELLOW);
		}
	}

	if (fab) {
		draw_fab_connections(fab, event_get_last_control_shift()&1 ? COL_RED : COL_WHITE, pos);
		koord fabpos = koord( pos.x + fab->pos.x, pos.y + fab->pos.y );
		welt_to_screen( fabpos );
		const koord boxpos = fabpos + koord(10, 0);
		const char * name = translator::translate(fab->gib_name());
		display_ddd_proportional_clip(boxpos.x, boxpos.y, proportional_string_width(name)+8, 0, 10, COL_WHITE, name, true);
	}
}
