#include "../simevent.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../vehicle/simvehikel.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../boden/grund.h"
#include "../simfab.h"
#include "../simcity.h"
#include "karte.h"
#include "fahrplan_gui.h"

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

// color for the land
const uint8 reliefkarte_t::map_type_color[MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND] =
{
	97, 99, 19, 21, 23,
	160, 161, 162, 163, 164, 165, 166, 167, 205, 206, 207, 173, 175, 214
};

const uint8 reliefkarte_t::severity_color[MAX_SEVERITY_COLORS] =
{
	106, 2, 85, 86, 29, 30, 171, 71, 39, 132
};

// Kenfarben fuer die Karte
#define STRASSE_KENN      (208)
#define SCHIENE_KENN      (185)
#define CHANNEL_KENN      (147)
#define MONORAIL_KENN      (153)
#define RUNWAY_KENN      (28)
#define POWERLINE_KENN      (55)
#define HALT_KENN         COL_RED
#define BUILDING_KENN      COL_GREY3


// converts karte koordinates to screen corrdinates
void
reliefkarte_t::karte_to_screen( koord &k ) const
{
	if(rotate45) {
		// 45 rotate view
		sint16 x = welt->get_groesse_y() + (k.x-k.y);
		k.y = (k.x+k.y);
		k.x = x;
	}
	k.x = (k.x*zoom_out)/zoom_in;
	k.y = (k.y*zoom_out)/zoom_in;
}


// and retransform
inline void
reliefkarte_t::screen_to_karte( koord &k ) const
{
	k = koord( (k.x*zoom_in)/zoom_out, (k.y*zoom_in)/zoom_out );
	if(rotate45) {
		k.x = (k.x+k.y-welt->get_groesse_y())/2;
		k.y = k.y - k.x;
	}
}


uint8
reliefkarte_t::calc_severity_color(sint32 amount, sint32 max_value)
{
	if(max_value!=0) {
		// color array goes from light blue to red
		sint32 severity = amount * MAX_SEVERITY_COLORS / max_value;
		if(severity>=MAX_SEVERITY_COLORS) {
			severity = MAX_SEVERITY_COLORS-1;
		}
		else if(severity < 0) {
			severity = 0;
		}
		return reliefkarte_t::severity_color[severity];
	}
	return reliefkarte_t::severity_color[0];
}



void
reliefkarte_t::set_relief_farbe(koord k, const int color)
{
	// if map is in normal mode, set new color for map
	// otherwise do nothing
	// result: convois will not "paint over" special maps
	if (relief==NULL  ||  !welt->ist_in_kartengrenzen(k)) {
		return;
	}

	karte_to_screen(k);

	if(rotate45) {
		// since isometric is distorted
		const int xw = zoom_in>=2 ? 1 : 2*zoom_out;
		for (int x = 0; x < xw; x++) {
			for (int y = 0; y < zoom_out; y++) {
				relief->at(k.x + x, k.y + y) = color;
			}
		}
	}
	else {
		for (int x = 0; x < zoom_out; x++) {
			for (int y = 0; y < zoom_out; y++) {
				relief->at(k.x + x, k.y + y) = color;
			}
		}
	}
}



void
reliefkarte_t::set_relief_farbe_area(koord k, int areasize, uint8 color)
{
	koord p;
	karte_to_screen(k);
	areasize *= zoom_out;
	if(rotate45) {
		k -= koord( 0, areasize/2 );
		k.x = clamp( k.x, areasize/2, relief->get_width()-areasize/2-1 );
		k.y = clamp( k.y, 0, relief->get_height()-areasize-1 );
		for (p.x = 0; p.x<areasize; p.x++) {
			for (p.y = 0; p.y<areasize; p.y++) {
				koord pos = koord( k.x+(p.x-p.y)/2, k.y+(p.x+p.y)/2 );
				relief->at(pos.x, pos.y) = color;
			}
		}
	}
	else {
		k -= koord( areasize/2, areasize/2 );
		k.x = clamp( k.x, 0, relief->get_width()-areasize-1 );
		k.y = clamp( k.y, 0, relief->get_height()-areasize-1 );
		for (p.x = 0; p.x<areasize; p.x++) {
			for (p.y = 0; p.y<areasize; p.y++) {
				relief->at(p.x+k.x, p.y+k.y) = color;
			}
		}
	}
}



/**
 * calculates ground color for position (hoehe - grundwasser).
 * @author Hj. Malthaner
 */
uint8 reliefkarte_t::calc_hoehe_farbe(const sint16 hoehe, const sint16 grundwasser)
{
	return map_type_color[clamp( (hoehe-grundwasser)+MAX_MAP_TYPE_WATER-1, 0, MAX_MAP_TYPE_WATER+MAX_MAP_TYPE_LAND )];
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
	if(gr->get_halt().is_bound()) {
		color = HALT_KENN;
	}
	else {
		switch(gr->get_typ()) {
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
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
						color = COL_GREY3;
					}
					else {
						color = fab->get_kennfarbe();
					}
				}
				break;
			case grund_t::wasser:
				{
					// object at zero is either factory or boat
					gebaeude_t *gb = gr->find<gebaeude_t>();
					fabrik_t *fab = gb ? gb->get_fabrik() : NULL;
					if(fab==NULL) {
#ifndef DOUBLE_GROUNDS
						sint16 height = (gr->get_grund_hang()&1);
#else
						sint16 height = (gr->get_grund_hang()%3);
#endif
						color = calc_hoehe_farbe((welt->lookup_hgt(gr->get_pos().get_2d())/Z_TILE_STEP)+height, welt->get_grundwasser()/Z_TILE_STEP);
						//color = COL_BLUE;	// water with boat?
					}
					else {
						color = fab->get_kennfarbe();
					}
				}
				break;
			// normal ground ...
			default:
				if(gr->hat_wege()) {
					switch(gr->get_weg_nr(0)->get_waytype()) {
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
					const leitung_t* lt = gr->find<leitung_t>();
					if(lt!=NULL) {
						color = POWERLINE_KENN;
					}
					else {
#ifndef DOUBLE_GROUNDS
						sint16 height = (gr->get_grund_hang()&1);
#else
						sint16 height = (gr->get_grund_hang()%3);
#endif
						color = calc_hoehe_farbe((gr->get_hoehe()/Z_TILE_STEP)+height, welt->get_grundwasser()/Z_TILE_STEP);
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
	if(plan==NULL) {
		return;
	}
	const grund_t *gr=plan->get_boden_bei(plan->get_boden_count()-1);

	// first use ground color
	set_relief_farbe( k, calc_relief_farbe(gr) );

	switch(mode) {
		// show passenger coverage
		// display coverage
		case MAP_PASSENGER:
			{
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()    &&  halt->get_pax_enabled()) {
					set_relief_farbe_area(k, (welt->get_einstellungen()->get_station_coverage()*2)+1, halt->get_besitzer()->get_player_color1()+3 );
				}
			}
			break;

		// show mail coverage
		// display coverage
		case MAP_MAIL:
			{
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()  &&  halt->get_post_enabled()) {
					set_relief_farbe_area(k, (welt->get_einstellungen()->get_station_coverage()*2)+1,halt->get_besitzer()->get_player_color1()+3 );
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
				const weg_t *w=gr->get_weg_nr(0);
				if(w) {
					cargo = w->get_statistics(WAY_STAT_GOODS);
					const weg_t *w=gr->get_weg_nr(1);
					if(w) {
						cargo += w->get_statistics(WAY_STAT_GOODS);
					}
					if(cargo>0) {
						if(cargo>max_cargo) {
							max_cargo = cargo;
						}
						set_relief_farbe(k, calc_severity_color(cargo, max_cargo));
					}
				}
			}
			break;

		// show station status
		case MAP_STATUS:
			{
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()) {
					const spieler_t* owner = halt->get_besitzer();
					if (owner == welt->get_active_player() || owner == welt->get_spieler(1)) {
						set_relief_farbe_area(k, 3, halt->get_status_farbe());
					}
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
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()) {
					const spieler_t* owner = halt->get_besitzer();
					if (owner == welt->get_active_player() || owner == welt->get_spieler(1)) {
						// get number of last month's arrived convois
						sint32 arrived = halt->get_finance_history(1, HALT_CONVOIS_ARRIVED);
						if(arrived>max_convoi_arrived) {
							max_convoi_arrived = arrived;
						}
						set_relief_farbe_area(k, 3, calc_severity_color(arrived, max_convoi_arrived));
					}
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
				const weg_t *w=gr->get_weg_nr(0);
				if(w) {
					passed = w->get_statistics(WAY_STAT_CONVOIS);
					const weg_t *w=gr->get_weg_nr(1);
					if(w) {
						passed += w->get_statistics(WAY_STAT_CONVOIS);
					}
					if (passed>0) {
						if(passed>max_passed) {
							max_passed = passed;
						}
						passed = ((passed*3)<<(MAX_SEVERITY_COLORS-2)/max_passed) + 1;
						int log_passed = MAX_SEVERITY_COLORS-1;
						while( (1<<log_passed) > passed) {
							log_passed --;
						}
						set_relief_farbe_area(k, 1, calc_severity_color(log_passed+1, MAX_SEVERITY_COLORS ));
					}
					else {
						set_relief_farbe_area(k, 1, reliefkarte_t::severity_color[0] );
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
			else if (gr->get_halt().is_bound()) {
				halthandle_t halt = gr->get_halt();
				// only show player's haltestellen
				const spieler_t* owner = halt->get_besitzer();
				if (owner == welt->get_active_player() || owner == welt->get_spieler(1)) {
					 sint32 arrived=halt->get_finance_history(1, HALT_DEPARTED)-halt->get_finance_history(1, HALT_ARRIVED);
					if(arrived>max_arrived) {
						max_arrived = arrived;
					}
					const uint8 color = calc_severity_color( arrived, max_arrived );
					set_relief_farbe_area(k, 3, color);
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
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()) {
					const spieler_t* owner = halt->get_besitzer();
					if (owner == welt->get_active_player() || owner == welt->get_spieler(1)) {
						sint32 departed=halt->get_finance_history(1, HALT_ARRIVED)-halt->get_finance_history(1, HALT_DEPARTED);
						if(departed>max_departed) {
							max_departed = departed;
						}
						const uint8 color = calc_severity_color( departed, max_departed );
						set_relief_farbe_area(k, 3, color );
					}
				}
			}
			break;

		// show waiting goods
		case MAP_WAITING:
			{
				halthandle_t halt = gr->get_halt();
				if (halt.is_bound()) {
					const spieler_t* owner = halt->get_besitzer();
					if (owner == welt->get_active_player() || owner == welt->get_spieler(1)) {
						// we need to sum up only for seperate capacities
						sint32 total_capacity = welt->get_einstellungen()->is_seperate_halt_capacities() ? halt->get_capacity(0)+halt->get_capacity(1)+halt->get_capacity(2) : halt->get_capacity(0);
						const uint8 color = calc_severity_color(halt->get_finance_history(0, HALT_WAITING), total_capacity );
						set_relief_farbe_area(k, 3, color );
					}
				}
			}
			break;

		// show tracks: white: no electricity, red: electricity, yellow: signal
		case MAP_TRACKS:
			// show track
			if (gr->hat_weg(track_wt)) {
				const schiene_t * sch = (const schiene_t *) (gr->get_weg(track_wt));
				if(sch->is_electrified()) {
					set_relief_farbe(k, COL_RED);
				}
				else {
					set_relief_farbe(k, COL_WHITE);
				}
				// show signals
				if(sch->has_sign()  ||  sch->has_signal()) {
					set_relief_farbe(k, COL_YELLOW);
				}
			}
			break;

		// show max speed (if there)
		case MAX_SPEEDLIMIT:
			{
				sint32 speed=gr->get_max_speed();
				if(speed) {
					set_relief_farbe(k, calc_severity_color(gr->get_max_speed(), 450));
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
				const leitung_t* lt = gr->find<leitung_t>();
				if(lt!=NULL) {
					sint32 capacity=lt->get_net()->get_capacity();
					if(capacity>max_capacity) {
						max_capacity = capacity;
					}
					set_relief_farbe(k, calc_severity_color(capacity,max_capacity) );
				}
			}
			break;

		case MAP_FOREST:
			if(gr->get_top()>1  &&  gr->obj_bei(gr->get_top()-1)->get_typ()==ding_t::baum) {
				set_relief_farbe(k, COL_GREEN );
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
			koord( ((welt->get_groesse_y()+zoom_in)*zoom_out)/zoom_in+((welt->get_groesse_x()+zoom_in)*zoom_out)/zoom_in+1, ((welt->get_groesse_y()+zoom_in-1)*zoom_out)/zoom_in+((welt->get_groesse_x()+zoom_in-1)*zoom_out)/zoom_in ) :
			koord( ((welt->get_groesse_x()+zoom_in-1)*zoom_out)/zoom_in, ((welt->get_groesse_y()+zoom_in-1)*zoom_out)/zoom_in );
	if((sint16)relief->get_width()!=size.x  ||  (sint16)relief->get_height()!=size.y) {
		delete relief;
		relief = new array2d_tpl<unsigned char> (size.x,size.y);
		set_groesse(size);	// of the gui_komponete to adjust scroll bars
		if(rotate45) {
			memset( (void *)(relief->to_array()), COL_BLACK, size.x*size.y );
		}
	}

	// redraw the map
	koord k;
	for(k.y=0; k.y<welt->get_groesse_y(); k.y++) {
		for(k.x=0; k.x<welt->get_groesse_x(); k.x++) {
			calc_map_pixel(k);
		}
	}

	// mark all vehicle positions
	for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
		convoihandle_t cnv = *i;
		for (uint16 i = 0; i < cnv->get_vehikel_anzahl(); i++) {
			set_relief_farbe(cnv->get_vehikel(i)->get_pos().get_2d(), VEHIKEL_KENN);
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
		const weighted_vector_tpl<gebaeude_t *> &ausflugsziele = welt->get_ausflugsziele();
		for (weighted_vector_tpl<gebaeude_t*>::const_iterator i = ausflugsziele.begin(), end = ausflugsziele.end(); i != end; ++i) {
			int pax = (*i)->get_passagier_level();
			if (max_tourist_ziele < pax) max_tourist_ziele = pax;
		}
		for (weighted_vector_tpl<gebaeude_t*>::const_iterator i = ausflugsziele.begin(), end = ausflugsziele.end(); i != end; ++i) {
			const gebaeude_t* g = *i;
			koord pos = g->get_pos().get_2d();
			set_relief_farbe_area( pos, 7, calc_severity_color(g->get_passagier_level(), max_tourist_ziele));
		}
		return;
	}

	// since we do iterate the factory info list, this must be done here
	if(mode==MAP_FACTORIES) {
		slist_iterator_tpl <fabrik_t *> iter (welt->get_fab_list());
		while(iter.next()) {
			koord pos = iter.get_current()->get_pos().get_2d();
			set_relief_farbe_area( pos, 9, COL_BLACK );
			set_relief_farbe_area( pos, 7, iter.get_current()->get_kennfarbe() );
		}
		return;
	}

	if(mode==MAP_DEPOT) {
		slist_iterator_tpl <depot_t *> iter (depot_t::get_depot_list());
		while(iter.next()) {
			koord pos = iter.get_current()->get_pos().get_2d();
			// offset of one to avoid
			static uint8 depot_typ_to_color[12]={ COL_ORANGE, COL_YELLOW, COL_RED, 0, 0, 0, 0, 0, 0, COL_PURPLE, COL_DARK_RED, COL_DARK_ORANGE };
			set_relief_farbe_area(pos, 3, depot_typ_to_color[iter.get_current()->get_typ()-ding_t::bahndepot] );
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
	fpl = NULL;
	fpl_player_nr = 0;
	is_show_schedule = false;
}



reliefkarte_t::~reliefkarte_t()
{
	if(relief != NULL) {
		delete relief;
	}
}



reliefkarte_t *
reliefkarte_t::get_karte()
{
	if(single_instance == NULL) {
		single_instance = new reliefkarte_t();
	}
	return single_instance;
}



void
reliefkarte_t::set_welt(karte_t *welt)
{
	this->welt = welt;			// Welt fuer display_win() merken
	if(relief) {
		delete relief;
		relief = NULL;
	}
	rotate45 = false;

	if(welt) {
		koord size = koord( (welt->get_groesse_x()*zoom_out)/zoom_in+1, (welt->get_groesse_y()*zoom_out)/zoom_in+1 );
DBG_MESSAGE("reliefkarte_t::set_welt()","welt size %i,%i",size.x,size.y);
		relief = new array2d_tpl<unsigned char> (size.x,size.y);
		set_groesse(size);
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
	if(mode!=MAP_TOWN) {
		calc_map();
	}
}




// these two are the only gui_container specific routines


// handle event
void
reliefkarte_t::infowin_event(const event_t *ev)
{
	koord k( ev->mx, ev->my );
	screen_to_karte( k );

	// get factory under mouse cursor
	last_world_pos = k;

	// recenter
	if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) {
		welt->set_follow_convoi( convoihandle_t() );
		int z = 0;
		if(welt->ist_in_kartengrenzen(k)) {
			z = welt->min_hgt(k);
		}
		welt->change_world_position(koord3d(k,z));
	}
}



// helper function for redraw: factory connections
const fabrik_t* reliefkarte_t::draw_fab_connections(const uint8 colour, const koord pos) const
{
	const fabrik_t* const fab = fabrik_t::get_fab(welt, last_world_pos);
	if(fab) {
		koord fabpos = fab->get_pos().get_2d();
		karte_to_screen( fabpos );
		fabpos += pos;
		const vector_tpl<koord>& lieferziele = event_get_last_control_shift() & 1 ? fab->get_suppliers() : fab->get_lieferziele();
		for (vector_tpl<koord>::const_iterator i = lieferziele.begin(), end = lieferziele.end(); i != end; ++i) {
			koord lieferziel = *i;
			const fabrik_t * fab2 = fabrik_t::get_fab(welt, lieferziel);
			if (fab2) {
				karte_to_screen( lieferziel );
				const koord end = lieferziel+pos;
				display_direct_line(fabpos.x, fabpos.y, end.x, end.y, colour);
				display_fillbox_wh_clip(end.x, end.y, 3, 3, ((welt->get_zeit_ms() >> 10) & 1) == 0 ? COL_RED : COL_WHITE, true);

				const koord boxpos = end + koord(10, 0);
				const char * name = translator::translate(fab2->get_name());
				display_ddd_proportional_clip(boxpos.x, boxpos.y, proportional_string_width(name)+8, 0, 5, COL_WHITE, name, true);
			}
		}
	}
	return fab;
}


// draw current schedule
void reliefkarte_t::draw_schedule(const koord pos) const
{
	assert(fpl!=NULL  ||  fpl->maxi()>0);

	koord first_koord;
	koord last_koord;
	const uint8 color = welt->get_spieler(fpl_player_nr)->get_player_color1()+1;

	// get stop list from schedule
	for( int i=0;  i<fpl->maxi();  i++  ) {
		koord new_koord = fpl->eintrag[i].pos.get_2d();
		karte_to_screen( new_koord );
		new_koord += pos;

		if(i>0) {
			// draw line from stop to stop
			display_direct_line(last_koord.x, last_koord.y, new_koord.x, new_koord.y, 127);
		}
		else {
			first_koord = new_koord;
		}
		//check, if mouse is near coordinate
		if(abs_distance(last_world_pos,fpl->eintrag[i].pos.get_2d())<=2) {
			// draw stop name with an index
			cbuffer_t buf(256);
			buf.clear();
			buf.printf( translator::translate("(%i)-"), i+1 );
			fahrplan_gui_t::gimme_short_stop_name(buf, welt, fpl, i, 240);
			display_ddd_proportional_clip(new_koord.x+10, new_koord.y+7, proportional_string_width(buf)+8, 0, color, COL_WHITE, buf, true);
		}
		// box at station
		display_fillbox_wh_clip(new_koord.x, new_koord.y, 4, 4, color, true);
		last_koord = new_koord;
	}
	// draw line back to first stop
	display_direct_line(last_koord.x, last_koord.y, first_koord.x, first_koord.y, 127);
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

	// draw city limit
	if(mode==MAP_CITYLIMIT) {

		const PLAYER_COLOR_VAL color = 127;

		// get city list
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		// for all cities
		for(  weighted_vector_tpl<stadt_t*>::const_iterator i = staedte.begin(), end = staedte.end();  i != end;  ++i  ) {
			const stadt_t* stadt = *i;
			koord k[4];
			k[0] =	stadt->get_linksoben(); // top left
			k[2] =	stadt->get_rechtsunten(); // bottom right

			// calculate and draw the rotated coordinates
			if(rotate45) {

				k[1] =  koord(k[0].x, k[2].y); // bottom left
				k[3] =  koord(k[2].x, k[0].y); // top right

				k[0] += koord(0, -1); // top left
				karte_to_screen(k[0]);
				k[0] = k[0] + pos;

				karte_to_screen(k[1]); // bottom left
				k[1] = k[1] + pos;

				k[2] += koord(1, 0); // bottom right
				karte_to_screen(k[2]);
				k[2] += pos;

				k[3] += koord(1, -1); // top right
				karte_to_screen(k[3]);
				k[3] += pos;

				display_direct_line(k[0].x, k[0].y, k[1].x, k[1].y, color);
				display_direct_line(k[1].x, k[1].y, k[2].x, k[2].y, color);
				display_direct_line(k[2].x, k[2].y, k[3].x, k[3].y, color);
				display_direct_line(k[3].x, k[3].y, k[0].x, k[0].y, color);
			}
			else {
				karte_to_screen(k[0]);
				k[0] = k[0] + pos + koord(-1, -1);

				k[2] += koord(1, 1);
				karte_to_screen(k[2]);
				k[2] += pos;

				display_direct_line(k[0].x, k[0].y, k[0].x, k[2].y, color);
				display_direct_line(k[2].x, k[0].y, k[2].x, k[2].y, color);
				display_direct_line(k[0].x, k[0].y, k[2].x, k[0].y, color);
				display_direct_line(k[0].x, k[2].y, k[2].x, k[2].y, color);
			}
		}

	}


	// if we do not do this here, vehicles would erase the town names
	// ADD: if CRTL key is pressed, temporary show the name
	if( mode==MAP_TOWN  ||  event_get_last_control_shift()==2 ) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		for (weighted_vector_tpl<stadt_t*>::const_iterator i = staedte.begin(), end = staedte.end(); i != end; ++i) {
			const stadt_t* stadt = *i;
			koord p = stadt->get_pos();
			const char * name = stadt->get_name();

			int w = proportional_string_width(name);
			karte_to_screen( p );
			p.x = clamp( p.x, 0, relief->get_width()-w );
			p += pos;
			display_proportional_clip( p.x, p.y, name, ALIGN_LEFT, COL_WHITE, true);
		}
	}

	// zoom/resize "selection box" in map
	// this must be rotated by 45 degree (sin45=cos45=0,5*sqrt(2)=0.707...)
	const sint16 raster=get_tile_raster_width();

	// calculate and draw the rotated coordinates
	if(rotate45) {
		// straight cursor
		const koord diff = koord( ((display_get_width()/raster)*zoom_out)/zoom_in, (((display_get_height()*2)/(raster))*zoom_out)/zoom_in );
		koord ij = welt->get_world_position();
		karte_to_screen( ij );
		ij += pos;
		display_direct_line( ij.x-diff.x, ij.y-diff.y, ij.x+diff.x, ij.y-diff.y, COL_YELLOW);
		display_direct_line( ij.x+diff.x, ij.y-diff.y, ij.x+diff.x, ij.y+diff.y, COL_YELLOW);
		display_direct_line( ij.x+diff.x, ij.y+diff.y, ij.x-diff.x, ij.y+diff.y, COL_YELLOW);
		display_direct_line( ij.x-diff.x, ij.y+diff.y, ij.x-diff.x, ij.y-diff.y, COL_YELLOW);
	}
	else {
		// rotate cursor
		const koord diff = koord( (display_get_width()*zoom_out)/(raster*zoom_in*2), (display_get_height()*zoom_out)/(raster*zoom_in) );
		koord ij = welt->get_world_position();
		karte_to_screen( ij );
		ij += pos;
		koord view[4];
		view[0] = ij + koord( -diff.y+diff.x, -diff.y-diff.x );
		view[1] = ij + koord( -diff.y-diff.x, -diff.y+diff.x );
		view[2] = ij + koord( diff.y-diff.x, diff.y+diff.x );
		view[3] = ij + koord( diff.y+diff.x, diff.y-diff.x );
		for(  int i=0;  i<4;  i++  ) {
			display_direct_line( view[i].x, view[i].y, view[(i+1)%4].x, view[(i+1)%4].y, COL_YELLOW);
		}
	}

	// draw a halt name, if it is under the cursor of a schedule
	if(is_show_schedule  &&  fpl) {
		if(fpl->maxi()<=0) {
			fpl = NULL;
		}
		else {
			draw_schedule(pos);
		}
	}
	else if(is_show_fab) {
		// draw factory connections, if on a factory
		const fabrik_t* const fab = draw_fab_connections(event_get_last_control_shift() & 1 ? COL_RED : COL_WHITE, pos);
		if(fab) {
			koord fabpos = fab->get_pos().get_2d();
			karte_to_screen( fabpos );
			const koord boxpos = fabpos + koord(10, 0) + pos;
			const char * name = translator::translate(fab->get_name());
			display_ddd_proportional_clip(boxpos.x, boxpos.y, proportional_string_width(name)+8, 0, 10, COL_WHITE, name, true);
		}
	}
}
