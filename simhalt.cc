/*
 * Copyright (c) 1997 - 2001 Hansj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * Haltestellen fuer Simutrans
 * 03.2000 getrennt von simfab.cc
 *
 * Hj. Malthaner
 */
#include <algorithm>

#include "freight_list_sorter.h"

#include "simcity.h"
#include "simcolor.h"
#include "simconvoi.h"
#include "simdebug.h"
#include "simfab.h"
#include "simgraph.h"
#include "simhalt.h"
#include "simintr.h"
#include "simline.h"
#include "simmem.h"
#include "simmesg.h"
#include "simplan.h"
#include "simtools.h"
#include "player/simplay.h"
#include "simwin.h"
#include "simworld.h"
#include "simware.h"

#include "bauer/hausbauer.h"
#include "bauer/warenbauer.h"

#include "besch/ware_besch.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/strasse.h"
#include "boden/wege/weg.h"

#include "dataobj/einstellungen.h"
#include "dataobj/fahrplan.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"
#include "dataobj/warenziel.h"

#include "dings/gebaeude.h"
#include "dings/label.h"

#include "gui/halt_info.h"
#include "gui/halt_detail.h"
#include "gui/karte.h"

#include "utils/simstring.h"

#include "vehicle/simpeople.h"

karte_t *haltestelle_t::welt = NULL;

slist_tpl<halthandle_t> haltestelle_t::alle_haltestellen;

stringhashtable_tpl<halthandle_t> haltestelle_t::all_names;

uint8 haltestelle_t::status_step = 0;
uint8 haltestelle_t::reconnect_counter = 0;


static vector_tpl<convoihandle_t>stale_convois;
static vector_tpl<linehandle_t>stale_lines;


void haltestelle_t::reset_routing()
{
	reconnect_counter = welt->get_schedule_counter()-1;
}


void haltestelle_t::step_all()
{
	// tell all stale convois to reroute their goods
	if(  !stale_convois.empty()  ) {
		convoihandle_t cnv = stale_convois.pop_back();
		if(  cnv.is_bound()  ) {
			cnv->check_freight();
		}
	}
	// same for stale lines
	if(  !stale_lines.empty()  ) {
		linehandle_t line = stale_lines.pop_back();
		if(  line.is_bound()  ) {
			line->check_freight();
		}
	}

	static slist_tpl<halthandle_t>::iterator iter( alle_haltestellen.begin() );
	if (alle_haltestellen.empty()) {
		return;
	}
	const uint8 schedule_counter = welt->get_schedule_counter();
	if (reconnect_counter != schedule_counter) {
		// always start with reconnection, rerouting will happen after complete reconnection
		status_step = RECONNECTING;
		reconnect_counter = schedule_counter;
		iter = alle_haltestellen.begin();
	}

	sint16 units_remaining = 128;
	for (; iter != alle_haltestellen.end(); ++iter) {
		if (units_remaining <= 0) return;

		// iterate until the specified number of units were handled
		if(  !(*iter)->step(status_step, units_remaining)  ) {
			// too much rerouted => needs to continue at next round!
			return;
		}
	}

	if (status_step == RECONNECTING) {
		// reroute in next call
		status_step = REROUTING;
	}
	else if (status_step == REROUTING) {
		status_step = 0;
	}
	iter = alle_haltestellen.begin();
}


/* we allow only for a single stop per planquadrat
 * this will only return something if this stop belongs to same player or is public, or is a dock (when on water)
 */
halthandle_t haltestelle_t::get_halt(const karte_t *welt, const koord3d pos, const spieler_t *sp )
{
	const grund_t *gr = welt->lookup(pos);
	if(gr) {
		if(gr->get_halt().is_bound()  &&  spieler_t::check_owner(sp,gr->get_halt()->get_besitzer())  ) {
			return gr->get_halt();
		}
		// no halt? => we do the water check
		if(gr->ist_wasser()) {
			// may catch bus stops close to water ...
			const planquadrat_t *plan = welt->lookup(pos.get_2d());
			const uint8 cnt = plan->get_haltlist_count();
			// first check for own stop
			for(  uint8 i=0;  i<cnt;  i++  ) {
				halthandle_t halt = plan->get_haltlist()[i];
				if(  halt->get_besitzer()==sp  &&  halt->get_station_type()&dock  ) {
					return halt;
				}
			}
			// then for public stop
			for(  uint8 i=0;  i<cnt;  i++  ) {
				halthandle_t halt = plan->get_haltlist()[i];
				if(  halt->get_besitzer()==welt->get_spieler(1)  &&  halt->get_station_type()&dock  ) {
					return halt;
				}
			}
			// so: nothing found
		}
	}
	return halthandle_t();
}


koord haltestelle_t::get_basis_pos() const
{
	return get_basis_pos3d().get_2d();
}


koord3d haltestelle_t::get_basis_pos3d() const
{
	if (tiles.empty()) {
		return koord3d::invalid;
	}
	assert(tiles.front().grund->get_pos().get_2d() == init_pos);
	return tiles.front().grund->get_pos();
}


/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(karte_t *welt, koord pos, spieler_t *sp)
{
	haltestelle_t * p = new haltestelle_t(welt, pos, sp);
	return p->self;
}


/*
 * removes a ground tile from a station
 * @author prissi
 */
bool haltestelle_t::remove(karte_t *welt, spieler_t *sp, koord3d pos)
{
	grund_t *bd = welt->lookup(pos);

	// wrong ground?
	if(bd==NULL) {
		dbg->error("haltestelle_t::remove()","illegal ground at %d,%d,%d", pos.x, pos.y, pos.z);
		return false;
	}

	halthandle_t halt = bd->get_halt();
	if(!halt.is_bound()) {
		dbg->error("haltestelle_t::remove()","no halt at %d,%d,%d", pos.x, pos.y, pos.z);
		return false;
	}

DBG_MESSAGE("haltestelle_t::remove()","removing segment from %d,%d,%d", pos.x, pos.y, pos.z);
	// otherwise there will be marked tiles left ...
	halt->mark_unmark_coverage(false);

	// only try to remove connected buildings, when still in list to avoid infinite loops
	if(  halt->rem_grund(bd)  ) {
		// remove station building?
		gebaeude_t* gb = bd->find<gebaeude_t>();
		if(gb) {
			DBG_MESSAGE("haltestelle_t::remove()",  "removing building" );
			hausbauer_t::remove( welt, sp, gb );
			bd = NULL;	// no need to recalc image
			// removing the building could have destroyed this halt already
			if (!halt.is_bound()){
				return true;
			}
		}
	}

	if(!halt->existiert_in_welt()) {
DBG_DEBUG("haltestelle_t::remove()","remove last");
		// all deleted?
DBG_DEBUG("haltestelle_t::remove()","destroy");
		haltestelle_t::destroy( halt );
	}

	// if building was removed this is false!
	if(bd) {
		bd->calc_bild();
		reliefkarte_t::get_karte()->calc_map_pixel(pos.get_2d());
	}
	return true;
}



/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(karte_t *welt, loadsave_t *file)
{
	haltestelle_t *p = new haltestelle_t(welt, file);
	return p->self;
}


/**
 * Station destruction method.
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy(halthandle_t const halt)
{
	delete halt.get_rep();
}


/**
 * Station destruction method.
 * Da destroy() alle_haltestellen modifiziert kann kein Iterator benutzt
 * werden! V. Meyer
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy_all(karte_t *welt)
{
	haltestelle_t::welt = welt;
	while (!alle_haltestellen.empty()) {
		halthandle_t halt = alle_haltestellen.front();
		destroy(halt);
	}
	status_step = 0;
}


haltestelle_t::haltestelle_t(karte_t* wl, loadsave_t* file)
{
	last_loading_step = wl->get_steps();

	welt = wl;

	waren = (vector_tpl<ware_t> **)calloc( warenbauer_t::get_max_catg_index(), sizeof(vector_tpl<ware_t> *) );
	connections = new vector_tpl<connection_t>[ warenbauer_t::get_max_catg_index() ];
	serving_schedules = new uint8[ warenbauer_t::get_max_catg_index() ];

	for ( uint8 i = 0; i < warenbauer_t::get_max_catg_index(); i++ ) {
		serving_schedules[i] = 0;
	}

	status_color = COL_YELLOW;

	reconnect_counter = welt->get_schedule_counter()-1;

	enables = NOT_ENABLED;

	// @author hsiegeln
	sortierung = freight_list_sorter_t::by_name;
	resort_freight_info = true;

	rdwr(file);

	markers[ self.get_id() ] = current_marker;

	alle_haltestellen.append(self);
}


haltestelle_t::haltestelle_t(karte_t* wl, koord k, spieler_t* sp)
{
	self = halthandle_t(this);
	assert( !alle_haltestellen.is_contained(self) );
	alle_haltestellen.append(self);

	markers[ self.get_id() ] = current_marker;

	last_loading_step = wl->get_steps();
	welt = wl;

	this->init_pos = k;
	besitzer_p = sp;

	enables = NOT_ENABLED;
	// force total reouting
	reconnect_counter = welt->get_schedule_counter()-1;
	last_catg_index = 255;

	waren = (vector_tpl<ware_t> **)calloc( warenbauer_t::get_max_catg_index(), sizeof(vector_tpl<ware_t> *) );
	connections = new vector_tpl<connection_t>[ warenbauer_t::get_max_catg_index() ];
	serving_schedules = new uint8[ warenbauer_t::get_max_catg_index() ];

	for ( uint8 i = 0; i < warenbauer_t::get_max_catg_index(); i++ ) {
		serving_schedules[i] = 0;
	}

	status_color = COL_YELLOW;

	sortierung = freight_list_sorter_t::by_name;
	init_financial_history();

	if(welt->ist_in_kartengrenzen(k)) {
		welt->access(k)->set_halt(self);
	}
}


haltestelle_t::~haltestelle_t()
{
	assert(self.is_bound());

	// first: remove halt from all lists
	int i=0;
	while(alle_haltestellen.is_contained(self)) {
		alle_haltestellen.remove(self);
		i++;
	}
	if (i != 1) {
		dbg->error("haltestelle_t::~haltestelle_t()", "handle %i found %i times in haltlist!", self.get_id(), i );
	}

	// do not forget the players list ...
	if(besitzer_p!=NULL) {
		besitzer_p->halt_remove(self);
	}

	// free name
	set_name(NULL);

	// remove from ground and planquadrat haltlists
	koord ul(32767,32767);
	koord lr(0,0);
	while(  !tiles.empty()  ) {
		koord pos = tiles.remove_first().grund->get_pos().get_2d();
		planquadrat_t *pl = welt->access(pos);
		assert(pl);
		pl->set_halt( halthandle_t() );
		for( uint8 i=0;  i<pl->get_boden_count();  i++  ) {
			pl->get_boden_bei(i)->set_halt( halthandle_t() );
		}
		// bounding box for adjustments
		if(ul.x>pos.x ) ul.x = pos.x;
		if(ul.y>pos.y ) ul.y = pos.y;
		if(lr.x<pos.x ) lr.x = pos.x;
		if(lr.y<pos.y ) lr.y = pos.y;
	}

	/* remove probably remaining halthandle at init_pos
	 * (created during loadtime for stops without ground) */
	planquadrat_t* pl = welt->access(init_pos);
	if(pl  &&  pl->get_halt()==self) {
		pl->set_halt( halthandle_t() );
	}

	// remove from all haltlists
	uint16 const cov = welt->get_settings().get_station_coverage();
	ul.x = max(0, ul.x - cov);
	ul.y = max(0, ul.y - cov);
	lr.x = min(welt->get_groesse_x(), lr.x + 1 + cov);
	lr.y = min(welt->get_groesse_y(), lr.y + 1 + cov);
	for(  int y=ul.y;  y<lr.y;  y++  ) {
		for(  int x=ul.x;  x<lr.x;  x++  ) {
			planquadrat_t *plan = welt->access(x,y);
			if(plan->get_haltlist_count()>0) {
				plan->remove_from_haltlist( welt, self );
			}
		}
	}

	destroy_win( magic_halt_info + self.get_id() );
	destroy_win( magic_halt_detail + self.get_id() );

	// finally detach handle
	// before it is needed for clearing up the planqudrat and tiles
	self.detach();

	for(unsigned i=0; i<warenbauer_t::get_max_catg_index(); i++) {
		if (waren[i]) {
			FOR(vector_tpl<ware_t>, const &w, *waren[i]) {
				fabrik_t::update_transit(&w, false);
			}
			delete waren[i];
			waren[i] = NULL;
		}
	}
	free( waren );
	delete[] connections;
	delete[] serving_schedules;

	// routes may have changed without this station ...
	verbinde_fabriken();
}


void haltestelle_t::rotate90( const sint16 y_size )
{
	init_pos.rotate90( y_size );

	// rotate waren destinations
	// iterate over all different categories
	for(unsigned i=0; i<warenbauer_t::get_max_catg_index(); i++) {
		if(waren[i]) {
			vector_tpl<ware_t>& warray = *waren[i];
			for (size_t j = warray.get_count(); j-- != 0;) {
				ware_t& ware = warray[j];
				if(ware.menge>0) {
					ware.rotate90(welt, y_size);
				}
				else {
					// empty => remove
					warray.remove_at(j);
				}
			}
		}
	}

	// relinking factories
	verbinde_fabriken();
}



const char* haltestelle_t::get_name() const
{
	const char *name = "Unknown";
	if (tiles.empty()) {
		name = "Unnamed";
	}
	else {
		grund_t* bd = welt->lookup(get_basis_pos3d());
		if(bd  &&  bd->get_flag(grund_t::has_text)) {
			name = bd->get_text();
		}
	}
	return name;
}



/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
 */
void haltestelle_t::set_name(const char *new_name)
{
	grund_t *gr = welt->lookup(get_basis_pos3d());
	if(gr) {
		if(gr->get_flag(grund_t::has_text)) {
			halthandle_t h = all_names.remove(gr->get_text());
			if(h!=self) {
				DBG_MESSAGE("haltestelle_t::set_name()","removing name %s already used!",gr->get_text());
			}
		}
		if(!gr->find<label_t>()) {
			gr->set_text( new_name );
			if(new_name  &&  all_names.set(gr->get_text(),self).is_bound() ) {
 				DBG_MESSAGE("haltestelle_t::set_name()","name %s already used!",new_name);
			}
		}
		// Knightly : need to update the title text of the associated halt detail and info dialogs, if present
		halt_detail_t *const details_frame = dynamic_cast<halt_detail_t *>( win_get_magic( magic_halt_detail + self.get_id() ) );
		if(  details_frame  ) {
			details_frame->set_name( get_name() );
		}
		halt_info_t *const info_frame = dynamic_cast<halt_info_t *>( win_get_magic( magic_halt_info + self.get_id() ) );
		if(  info_frame  ) {
			info_frame->set_name( get_name() );
		}
	}
}


// returns the number of % in a printf string ....
static int count_printf_param( const char *str )
{
	int count = 0;
	while( *str!=0  ) {
		if(  *str=='%'  ) {
			str++;
			if(  *str!='%'  ) {
				count++;
			}
		}
		str ++;
	}
	return count;
}


// creates stops with unique! names
char* haltestelle_t::create_name(koord const k, char const* const typ)
{
	int const lang = welt->get_settings().get_name_language_id();
	stadt_t *stadt = welt->suche_naechste_stadt(k);
	const char *stop = translator::translate(typ,lang);
	cbuffer_t buf;

	// this fails only, if there are no towns at all!
	if(stadt==NULL) {
		// get a default name
		buf.printf( translator::translate("land stop %i %s",lang), get_besitzer()->get_haltcount(), stop );
		return strdup(buf);
	}

	// now we have a city
	const char *city_name = stadt->get_name();
	sint16 li_gr = stadt->get_linksoben().x - 2;
	sint16 re_gr = stadt->get_rechtsunten().x + 2;
	sint16 ob_gr = stadt->get_linksoben().y - 2;
	sint16 un_gr = stadt->get_rechtsunten().y + 2;

	// strings for intown / outside of town
	const bool inside = (li_gr < k.x  &&  re_gr > k.x  &&  ob_gr < k.y  &&  un_gr > k.y);
	const bool suburb = !inside  &&  (li_gr - 6 < k.x  &&  re_gr + 6 > k.x  &&  ob_gr - 6 < k.y  &&  un_gr + 6 > k.y);

	if (!welt->get_settings().get_numbered_stations()) {
		static const koord next_building[24] = {
			koord( 0, -1), // nord
			koord( 1,  0), // ost
			koord( 0,  1), // sued
			koord(-1,  0), // west
			koord( 1, -1), // nordost
			koord( 1,  1), // suedost
			koord(-1,  1), // suedwest
			koord(-1, -1), // nordwest
			koord( 0, -2),	// double nswo
			koord( 2,  0),
			koord( 0,  2),
			koord(-2,  0),
			koord( 1, -2),	// all the remaining 3s
			koord( 2, -1),
			koord( 2,  1),
			koord( 1,  2),
			koord(-1,  2),
			koord(-2,  1),
			koord(-2, -1),
			koord(-1, -2),
			koord( 2, -2),	// and now all buildings with distance 4
			koord( 2,  2),
			koord(-2,  2),
			koord(-2, -2)
		};

		// standard names:
		// order: factory, attraction, direction, normal name
		// prissi: first we try a factory name

		// is there a translation for factory defined?
		const char *fab_base_text = "%s factory %s %s";
		const char *fab_base = translator::translate(fab_base_text,lang);
		if(  fab_base_text != fab_base  ) {
			slist_tpl<fabrik_t *>fabs;
			if (self.is_bound()) {
				// first factories (so with same distance, they have priority)
				int this_distance = 999;
				FOR(slist_tpl<fabrik_t*>, const f, get_fab_list()) {
					int distance = koord_distance(f->get_pos().get_2d(), k);
					if(  distance < this_distance  ) {
						fabs.insert(f);
						this_distance = distance;
					}
					else {
						fabs.append(f);
					}
				}
			}
			else {
				// since the distance are presorted, we can just append for a good choice ...
				for(  int test=0;  test<24;  test++  ) {
					fabrik_t *fab = fabrik_t::get_fab(welt,k+next_building[test]);
					if(fab  &&  fabs.is_contained(fab)) {
						fabs.append(fab);
					}
				}
			}

			// are there fabs?
			FOR(slist_tpl<fabrik_t*>, const f, fabs) {
				// with factories
				buf.printf(fab_base, city_name, f->get_name(), stop);
				if(  !all_names.get(buf).is_bound()  ) {
					return strdup(buf);
				}
				buf.clear();
			}
		}

		// no fabs or all names used up already
		// is there a translation for buildings defined?
		const char *building_base_text = "%s building %s %s";
		const char *building_base = translator::translate(building_base_text,lang);
		if(  building_base_text != building_base  ) {
			// check for other special building (townhall, monument, tourst attraction)
			for (int i=0; i<24; i++) {
				grund_t *gr = welt->lookup_kartenboden( next_building[i] + k);
				if(gr==NULL  ||  gr->get_typ()!=grund_t::fundament) {
					// no building here
					continue;
				}
				// since closes coordinates are tested first, we do not need to not sort this
				const char *building_name = NULL;
				const gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb==NULL) {
					// field may have foundations but no building
					continue;
				}
				// now we have a building here
				if (gb->is_monument()) {
					building_name = translator::translate(gb->get_name(),lang);
				}
				else if (gb->ist_rathaus() ||
					gb->get_tile()->get_besch()->get_utyp() == haus_besch_t::attraction_land || // land attraction
					gb->get_tile()->get_besch()->get_utyp() == haus_besch_t::attraction_city) { // town attraction
					building_name = make_single_line_string(translator::translate(gb->get_tile()->get_besch()->get_name(),lang), 2);
				}
				else {
					// normal town house => not suitable for naming
					continue;
				}
				// now we have a name: try it
				buf.printf( building_base, city_name, building_name, stop );
				if(  !all_names.get(buf).is_bound()  ) {
					return strdup(buf);
				}
				buf.clear();
			}
		}

		// if there are street names, use them
		if(  inside  ||  suburb  ) {
			buf.clear();
			vector_tpl<char *> street_names( translator::get_street_name_list() );
			while(  !street_names.empty()  ) {
				const uint32 idx = simrand(street_names.get_count());

				buf.clear();
				if (cbuffer_t::check_format_strings("%s %s", street_names[idx])) {
					buf.printf( street_names[idx], city_name, stop );
					if(  !all_names.get(buf).is_bound()  ) {
						return strdup(buf);
					}
				}
				// remove this entry
				street_names[idx] = street_names.back();
				street_names.pop_back();
			}
			buf.clear();
		}

		// still all names taken => then try the normal naming scheme ...
		char numbername[10];
		if(inside) {
			strcpy( numbername, "0center" );
		} else if(suburb) {
			// close to the city we use a different scheme, with suburbs
			strcpy( numbername, "0suburb" );
		}
		else {
			strcpy( numbername, "0extern" );
		}

		const char *dirname = NULL;
		static const char *diagonal_name[4] = { "nordwest", "nordost", "suedost", "suedwest" };
		static const char *direction_name[4] = { "nord", "ost", "sued", "west" };

		uint8 const rot = welt->get_settings().get_rotation();
		if (k.y < ob_gr  ||  (inside  &&  k.y*3 < (un_gr+ob_gr+ob_gr))  ) {
			if (k.x < li_gr) {
				dirname = diagonal_name[(4 - rot) % 4];
			}
			else if (k.x > re_gr) {
				dirname = diagonal_name[(5 - rot) % 4];
			}
			else {
				dirname = direction_name[(4 - rot) % 4];
			}
		} else if (k.y > un_gr  ||  (inside  &&  k.y*3 > (un_gr+un_gr+ob_gr))  ) {
			if (k.x < li_gr) {
				dirname = diagonal_name[(3 - rot) % 4];
			}
			else if (k.x > re_gr) {
				dirname = diagonal_name[(6 - rot) % 4];
			}
			else {
				dirname = direction_name[(6 - rot) % 4];
			}
		} else {
			if (k.x <= stadt->get_pos().x) {
				dirname = direction_name[(3 - rot) % 4];
			}
			else {
				dirname = direction_name[(5 - rot) % 4];
			}
		}
		dirname = translator::translate(dirname,lang);

		// Try everything to get a unique name
		while(true) {
			// well now try them all from "0..." over "9..." to "A..." to "Z..."
			for(  int i=0;  i<10+26;  i++  ) {
				numbername[0] = i<10 ? '0'+i : 'A'+i-10;
				const char *base_name = translator::translate(numbername,lang);
				if(base_name==numbername) {
					// not translated ... try next
					continue;
				}
				// allow for names without direction
				uint8 count_s = count_printf_param( base_name );
				if(count_s==3) {
					if (cbuffer_t::check_format_strings("%s %s %s", base_name) ) {
						// ok, try this name, if free ...
						buf.printf( base_name, city_name, dirname, stop );
					}
				}
				else {
					if (cbuffer_t::check_format_strings("%s %s", base_name) ) {
						// ok, try this name, if free ...
						buf.printf( base_name, city_name, stop );
					}
				}
				if(  buf.len()>0  &&  !all_names.get(buf).is_bound()  ) {
					return strdup(buf);
				}
				buf.clear();
			}
			// here we did not find a suitable name ...
			// ok, no suitable city names, try the suburb ones ...
			if(  strcmp(numbername+1,"center")==0  ) {
				strcpy( numbername, "0suburb" );
			}
			// ok, no suitable suburb names, try the external ones (if not inside city) ...
			else if(  strcmp(numbername+1,"suburb")==0  &&  !inside  ) {
				strcpy( numbername, "0extern" );
			}
			else {
				// no suitable unique name found at all ...
				break;
			}
		}
	}

	/* so far we did not found a matching station name
	 * as a last resort, we will try numbered names
	 * (or the user requested this anyway)
	 */

	// strings for intown / outside of town
	const char *base_name = translator::translate( inside ? "%s city %d %s" : "%s land %d %s", lang);

	// finally: is there a stop with this name already?
	for(  uint32 i=1;  i<65536;  i++  ) {
		buf.printf( base_name, city_name, i, stop );
		if(  !all_names.get(buf).is_bound()  ) {
			return strdup(buf);
		}
		buf.clear();
	}

	// emergency measure: But before we should run out of handles anyway ...
	assert(0);
	return strdup("Unnamed");
}



// add convoi to loading
void haltestelle_t::request_loading( convoihandle_t cnv )
{
	if(  !loading_here.is_contained(cnv)  ) {
		loading_here.append (cnv );
	}
	if(  last_loading_step != welt->get_steps()  ) {
		last_loading_step = welt->get_steps();
		// now iterate over all convois
		for(  slist_tpl<convoihandle_t>::iterator i = loading_here.begin(), end = loading_here.end();  i != end;  ) {
			convoihandle_t const c = *i;
			if (c.is_bound() && c->get_state() == convoi_t::LOADING) {
				// now we load into convoi
				c->hat_gehalten(self);
			}
			if (c.is_bound() && c->get_state() == convoi_t::LOADING) {
				++i;
			}
			else {
				i = loading_here.erase( i );
			}
		}
	}
}



bool haltestelle_t::step(uint8 what, sint16 &units_remaining)
{
	switch(what) {
		case RECONNECTING:
			units_remaining -= (rebuild_connections()/256)+2;
			break;
		case REROUTING:
			if(  !reroute_goods(units_remaining)  ) {
				return false;
			}
			recalc_status();
			break;
		default:
			break;
	}
	return true;
}



/**
 * Called every month
 * @author Hj. Malthaner
 */
void haltestelle_t::neuer_monat()
{
	if(  welt->get_active_player()==besitzer_p  &&  status_color==COL_RED  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("%s\nis crowded."), get_name() );
		welt->get_message()->add_message(buf, get_basis_pos(),message_t::full, PLAYER_FLAG|besitzer_p->get_player_nr(), IMG_LEER );
		enables &= (PAX|POST|WARE);
	}

	// hsiegeln: roll financial history
	for (int j = 0; j<MAX_HALT_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	// number of waitung should be constant ...
	financial_history[0][HALT_WAITING] = financial_history[1][HALT_WAITING];
}



/**
 * Called after schedule calculation of all stations is finished
 * will distribute the goods to changed routes (if there are any)
 * returns true upon completion
 * @author Hj. Malthaner
 */
bool haltestelle_t::reroute_goods(sint16 &units_remaining)
{
	if(  last_catg_index==255  ) {
		last_catg_index = 0;
	}

	for(  ; last_catg_index<warenbauer_t::get_max_catg_index(); last_catg_index++) {

		if(  units_remaining<=0  ) {
			return false;
		}

		if(waren[last_catg_index]) {

			// first: clean out the array
			vector_tpl<ware_t> * warray = waren[last_catg_index];
			vector_tpl<ware_t> * new_warray = new vector_tpl<ware_t>(warray->get_count());

			for (size_t j = warray->get_count(); j-- != 0;) {
				ware_t & ware = (*warray)[j];

				if(ware.menge==0) {
					continue;
				}

				// since also the factory halt list is added to the ground, we can use just this ...
				if(  welt->lookup(ware.get_zielpos())->is_connected(self)  ) {
					// we are already there!
					if(  ware.to_factory  ) {
						liefere_an_fabrik(ware);
					}
					continue;
				}

				// add to new array
				new_warray->append( ware );
			}

			// delete, if nothing connects here
			if(  new_warray->empty()  ) {
				if(  connections[last_catg_index].empty()  ) {
					// no connections from here => delete
					delete new_warray;
					new_warray = NULL;
				}
			}

			// replace the array
			delete waren[last_catg_index];
			waren[last_catg_index] = new_warray;

			// if something left
			// re-route goods to adapt to changes in world layout,
			// remove all goods whose destination was removed from the map
			if (waren[last_catg_index] && !waren[last_catg_index]->empty()) {

				vector_tpl<ware_t> &warray = *waren[last_catg_index];
				uint32 last_ware_index = 0;
				units_remaining -= warray.get_count();
				while(  last_ware_index<warray.get_count()  ) {
					search_route_resumable(warray[last_ware_index]);
					if(  warray[last_ware_index].get_ziel()==halthandle_t()  ) {
						// remove invalid destinations
						fabrik_t::update_transit( &warray[last_ware_index], false);
						warray.remove_at(last_ware_index);
					}
					else {
						++last_ware_index;
					}
				}
			}
		}
	}
	// likely the display must be updated after this
	resort_freight_info = true;
	last_catg_index = 255;	// all categories are rerouted
	return true;	// all updated ...
}



/*
 * connects a factory to a halt
 */
void haltestelle_t::verbinde_fabriken()
{
	// unlink all
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->unlink_halt(self);
	}
	fab_list.clear();

	// then reconnect
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		koord const p = i.grund->get_pos().get_2d();

		int const cov = welt->get_settings().get_station_coverage();
		FOR(vector_tpl<fabrik_t*>, const fab, fabrik_t::sind_da_welche(welt, p - koord(cov, cov), p + koord(cov, cov))) {
			if(!fab_list.is_contained(fab)) {
				// water factories can only connect to docks
				if(  fab->get_besch()->get_platzierung() != fabrik_besch_t::Wasser  ||  (station_type & dock) > 0  ) {
					// do no link to oil rigs via stations ...
					fab_list.insert(fab);
					fab->link_halt(self);
				}
			}
		}
	}
}



/*
 * removes factory to a halt
 */
void haltestelle_t::remove_fabriken(fabrik_t *fab)
{
	fab_list.remove(fab);
}


/**
 * Rebuilds the list of connections to directly reachable halts
 * Returns the number of stops considered
 * @author Hj. Malthaner
 */
#define WEIGHT_WAIT (8)
#define WEIGHT_HALT (1)
// the minimum weight of a connection from a transfer halt
#define WEIGHT_MIN (WEIGHT_WAIT+WEIGHT_HALT)
sint32 haltestelle_t::rebuild_connections()
{
	// Knightly : halts which either immediately precede or succeed self halt in serving schedules
	static vector_tpl<halthandle_t> consecutive_halts[256];
	// Dwachs : halts which either immediately precede or succeed self halt in currently processed schedule
	static vector_tpl<halthandle_t> consecutive_halts_fpl[256];
	// remember max number of consecutive halts for one schedule
	uint8 max_consecutive_halts_fpl[256];
	MEMZERON(max_consecutive_halts_fpl, warenbauer_t::get_max_catg_index());
	// Knightly : previous halt supporting the ware categories of the serving line
	static halthandle_t previous_halt[256];

	// Hajo: first, remove all old entries
	for(  uint8 i=0;  i<warenbauer_t::get_max_catg_index();  i++  ){
		connections[i].clear();
		serving_schedules[i] = 0;
		consecutive_halts[i].clear();
	}
	resort_freight_info = true;	// might result in error in routing

	last_catg_index = 255;	// must reroute everything
	sint32 connections_searched = 0;

// DBG_MESSAGE("haltestelle_t::rebuild_destinations()", "Adding new table entries");

	const spieler_t *owner;
	schedule_t *fpl;
	const minivec_tpl<uint8> *goods_catg_index;

	minivec_tpl<uint8> supported_catg_index(32);

	/*
	 * In the first loops:
	 * lines==true => search for lines
	 * After this:
	 * lines==false => search for single convoys without lines
	 */
	bool lines = true;
	uint32 current_index = 0;
	while(  lines  ||  current_index < registered_convoys.get_count()  ) {

		// Now, collect the "fpl", "owner" and "add_catg_index" from line resp. convoy.
		if(  lines  ) {
			if(  current_index >= registered_lines.get_count()  ) {
				// We have looped over all lines.
				lines = false;
				current_index = 0;	// Knightly : start over for registered lineless convoys
				continue;
			}

			const linehandle_t line = registered_lines[current_index];
			++current_index;

			owner = line->get_besitzer();
			fpl = line->get_schedule();
			goods_catg_index = &line->get_goods_catg_index();
		}
		else {
			const convoihandle_t cnv = registered_convoys[current_index];
			++current_index;

			owner = cnv->get_besitzer();
			fpl = cnv->get_schedule();
			goods_catg_index = &cnv->get_goods_catg_index();
		}

		// find the index from which to start processing
		uint8 start_index = 0;
		while(  start_index < fpl->get_count()  &&  get_halt( welt, fpl->eintrag[start_index].pos, owner ) != self  ) {
			++start_index;
		}
		++start_index;	// the next index after self halt; it's okay to be out-of-range

		// determine goods category indices supported by this halt
		supported_catg_index.clear();
		FOR(minivec_tpl<uint8>, const catg_index, *goods_catg_index) {
			if(  is_enabled(catg_index)  ) {
				supported_catg_index.append(catg_index);
				previous_halt[catg_index] = self;
				consecutive_halts_fpl[catg_index].clear();
			}
		}

		if(  supported_catg_index.empty()  ) {
			// this halt does not support the goods categories handled by the line/lineless convoy
			continue;
		}

		INT_CHECK("simhalt.cc 612");

		// now we add the schedule to the connection array
		uint16 aggregate_weight = WEIGHT_WAIT;
		for(  uint8 j=0;  j<fpl->get_count();  ++j  ) {

			halthandle_t current_halt = get_halt(welt, fpl->eintrag[(start_index+j)%fpl->get_count()].pos, owner );
			if(  !current_halt.is_bound()  ) {
				// ignore way points
				continue;
			}
			if(  current_halt == self  ) {
				// Knightly : check for consecutive halts which precede self halt
				FOR(minivec_tpl<uint8>, const catg_index, supported_catg_index) {
					if(  previous_halt[catg_index]!=self  ) {
						consecutive_halts[catg_index].append_unique(previous_halt[catg_index]);
						consecutive_halts_fpl[catg_index].append_unique(previous_halt[catg_index]);
						previous_halt[catg_index] = self;
					}
				}
				// reset aggregate weight
				aggregate_weight = WEIGHT_WAIT;
				continue;
			}

			aggregate_weight += WEIGHT_HALT;

			FOR(minivec_tpl<uint8>, const catg_index, supported_catg_index) {
				if(  current_halt->is_enabled(catg_index)  ) {
					// Knightly : check for consecutive halts which succeed self halt
					if(  previous_halt[catg_index] == self  ) {
						consecutive_halts[catg_index].append_unique(current_halt);
						consecutive_halts_fpl[catg_index].append_unique(current_halt);
					}
					previous_halt[catg_index] = current_halt;

					// either add a new connection or update the weight of an existing connection where necessary
					connection_t *const existing_connection = connections[catg_index].insert_unique_ordered( connection_t( current_halt, aggregate_weight ), connection_t::compare );
					if(  existing_connection  &&  aggregate_weight<existing_connection->weight  ) {
						existing_connection->weight = aggregate_weight;
					}
				}
			}
		}

		FOR(minivec_tpl<uint8>, const catg_index, supported_catg_index) {
			if(  consecutive_halts_fpl[catg_index].get_count() > max_consecutive_halts_fpl[catg_index]  ) {
				max_consecutive_halts_fpl[catg_index] = consecutive_halts_fpl[catg_index].get_count();
			}
		}
		connections_searched += fpl->get_count();
	}
	for(  uint8 i=0;  i<warenbauer_t::get_max_catg_index();  i++  ){
		if(  !consecutive_halts[i].empty()  ) {
			if(  consecutive_halts[i].get_count() == max_consecutive_halts_fpl[i]  ) {
				// one schedule reaches all consecutive halts -> this is not transfer halt
				serving_schedules[i] = 1u;
			}
			else {
				serving_schedules[i] = 2u;
			}
		}
	}
	return connections_searched;
}


/**
 * Data for route searching
 */
haltestelle_t::halt_data_t haltestelle_t::halt_data[65536];
binary_heap_tpl<haltestelle_t::route_node_t> haltestelle_t::open_list;
uint8 haltestelle_t::markers[65536];
uint8 haltestelle_t::current_marker = 0;
/**
 * Data for resumable route search
 */
halthandle_t haltestelle_t::last_search_origin;
uint8 haltestelle_t::last_search_ware_catg_idx = 255;
/**
 * This routine tries to find a route for a good packet (ware)
 * it will be called for
 *  - new goods (either from simcity.cc or simfab.cc)
 *  - goods that transfer and cannot be joined with other goods
 *  - during rerouting
 * Therefore this routine eats up most of the performance in
 * later games. So all changes should be done with this in mind!
 *
 * If no route is found, ziel and zwischenziel are unbound handles.
 * If next_to_ziel in not NULL, it will get the koordinate of the stop
 * previous to target. Can be used to create passengers/mail back the
 * same route back
 *
 * if USE_ROUTE_SLIST_TPL is defined, the list template will be used.
 * However, this is about 50% slower.
 *
 * @author Hj. Malthaner/prissi/gerw/Knightly
 */
int haltestelle_t::search_route( const halthandle_t *const start_halts, const uint16 start_halt_count, const bool no_routing_over_overcrowding, ware_t &ware, ware_t *const return_ware )
{
	const uint8 ware_catg_idx = ware.get_besch()->get_catg_index();

	// since also the factory halt list is added to the ground, we can use just this ...
	const planquadrat_t *const plan = welt->lookup( ware.get_zielpos() );
	const halthandle_t *const halt_list = plan->get_haltlist();
	// but we can only use a subset of these
	static vector_tpl<halthandle_t> end_halts(16);
	end_halts.clear();
	for( uint32 h=0;  h<plan->get_haltlist_count();  ++h ) {
		halthandle_t halt = halt_list[h];
		if(  halt.is_bound()  &&  halt->is_enabled(ware_catg_idx)  ) {
			// check if this is present in the list of start halts
			for(  uint16 s=0;  s<start_halt_count;  ++s  ) {
				if(  halt==start_halts[s]  ) {
					// destination halt is also a start halt -> within walking distance
					ware.set_ziel( start_halts[s] );
					ware.set_zwischenziel( halthandle_t() );
					if(  return_ware  ) {
						return_ware->set_ziel( start_halts[s] );
						return_ware->set_zwischenziel( halthandle_t() );
					}
					return ROUTE_WALK;
				}
			}
			end_halts.append(halt);
		}
	}

	if(  end_halts.empty()  ) {
		// no end halt found
		ware.set_ziel( halthandle_t() );
		ware.set_zwischenziel( halthandle_t() );
		if(  return_ware  ) {
			return_ware->set_ziel( halthandle_t() );
			return_ware->set_zwischenziel( halthandle_t() );
		}
		return NO_ROUTE;
	}
	// invalidate search history
	last_search_origin = halthandle_t();

	// set current marker
	++current_marker;
	if(  current_marker==0  ) {
		MEMZERON(markers, halthandle_t::get_size());
		current_marker = 1u;
	}

	// initialisations for end halts => save some checking inside search loop
	FOR(vector_tpl<halthandle_t>, const e, end_halts) {
		uint16 const halt_id = e.get_id();
		halt_data[ halt_id ].best_weight = 65535u;
		halt_data[ halt_id ].destination = 1u;
		markers[ halt_id ] = current_marker;
	}

	uint16 const max_transfers = welt->get_settings().get_max_transfers();
	uint16 const max_hops      = welt->get_settings().get_max_hops();
	uint16 allocation_pointer = 0;
	uint16 best_destination_weight = 65535u;		// best weight among all destinations

	open_list.clear();

	uint32 overcrowded_nodes = 0;
	// initialise the origin node(s)
	for(  ;  allocation_pointer<start_halt_count;  ++allocation_pointer  ) {
		halthandle_t start_halt = start_halts[allocation_pointer];

		open_list.insert( route_node_t(start_halt, 0) );

		halt_data_t & start_data = halt_data[ start_halt.get_id() ];
		start_data.best_weight = 65535u;
		start_data.destination = 0;
		start_data.depth       = 0;
		start_data.overcrowded = no_routing_over_overcrowding  &&  start_halt->is_overcrowded(ware_catg_idx);
		start_data.transfer    = halthandle_t();
		overcrowded_nodes     += start_data.overcrowded;

		markers[ start_halt.get_id() ] = current_marker;
	}

	// here the normal routing with overcrowded stops is done
	do {
		route_node_t current_node = open_list.pop();
		// do not use aggregate_weight as it is _not_ the weight of the current_node
		// there might be a heuristic weight added

		const uint16 current_halt_id = current_node.halt.get_id();
		halt_data_t & current_halt_data = halt_data[ current_halt_id ];
		overcrowded_nodes -= current_halt_data.overcrowded;

		if(  overcrowded_nodes  &&  overcrowded_nodes == open_list.get_count()  ) {
			// all route go over overcrowded stations
			return ROUTE_OVERCROWDED;
		}
		if(  current_halt_data.destination  ) {
			// destination found
			ware.set_ziel( current_node.halt );
			assert(current_halt_data.transfer.get_id() != 0);
			if(  return_ware  ) {
				// next transfer for the reverse route
				// if the end halt and its connections contain more than one transfer halt then
				// the transfer halt may not be the last transfer of the forward route
				// (the rerouting will happen in haltestelle_t::hole_ab)
				return_ware->set_zwischenziel(current_halt_data.transfer);
				// count the connected transfer halts (including end halt)
				uint8 t = current_node.halt->is_transfer(ware_catg_idx);
				FOR(vector_tpl<connection_t>, const& i, current_node.halt->connections[ware_catg_idx]) {
					if (t > 1) break;
					t += i.halt.is_bound() && i.halt->is_transfer(ware_catg_idx);
				}
				return_ware->set_zwischenziel(  t<=1  ?  current_halt_data.transfer  : halthandle_t());
			}
			// find the next transfer
			halthandle_t transfer_halt = current_node.halt;
			while(  halt_data[ transfer_halt.get_id() ].depth > 1   ) {
				transfer_halt = halt_data[ transfer_halt.get_id() ].transfer;
			}
			ware.set_zwischenziel(transfer_halt);
			if(  return_ware  ) {
				// return ware's destination halt is the start halt of the forward trip
				assert( halt_data[ transfer_halt.get_id() ].transfer.get_id() );
				return_ware->set_ziel( halt_data[ transfer_halt.get_id() ].transfer );
			}
			return current_halt_data.overcrowded ? ROUTE_OVERCROWDED : ROUTE_OK;
		}

		// check if the current halt is already in closed list
		if(  current_halt_data.best_weight==0  ) {
			// shortest path to the current halt has already been found earlier
			continue;
		}

		if(  current_halt_data.depth > max_transfers  ) {
			// maximum transfer limit is reached -> do not add reachable halts to open list
			continue;
		}

		FOR(vector_tpl<connection_t>, const& current_conn, current_node.halt->connections[ware_catg_idx]) {

			// halt may have been deleted or joined => test if still valid
			if(  !current_conn.halt.is_bound()  ) {
				// removal seems better though ...
				continue;
			}

			// since these are precalculated, they should be always pointing to a valid ground
			// (if not, we were just under construction, and will be fine after 16 steps)
			const uint16 reachable_halt_id = current_conn.halt.get_id();

			const bool overcrowded_transfer = no_routing_over_overcrowding  &&  (current_halt_data.overcrowded  ||  current_conn.halt->is_overcrowded(ware_catg_idx) );

			if(  markers[ reachable_halt_id ]!=current_marker  ) {
				// Case : not processed before

				// indicate that this halt has been processed
				markers[ reachable_halt_id ] = current_marker;

				if(  current_conn.halt.is_bound()  &&  current_conn.halt->is_transfer(ware_catg_idx)  &&  allocation_pointer<max_hops  ) {
					// Case : transfer halt
					const uint16 total_weight = current_halt_data.best_weight + current_conn.weight;

					if(  total_weight < best_destination_weight  ) {

						halt_data[ reachable_halt_id ].best_weight = total_weight;
						halt_data[ reachable_halt_id ].destination = 0;
						halt_data[ reachable_halt_id ].depth       = current_halt_data.depth + 1u;
						halt_data[ reachable_halt_id ].transfer    = current_node.halt;
						halt_data[ reachable_halt_id ].overcrowded = overcrowded_transfer;
						overcrowded_nodes                         += overcrowded_transfer;

						allocation_pointer++;
						// as the next halt is not a destination add WEIGHT_MIN
						open_list.insert( route_node_t(current_conn.halt, total_weight + WEIGHT_MIN) );
					}
					else {
						// Case: non-optimal transfer halt -> put in closed list
						halt_data[ reachable_halt_id ].best_weight = 0;
					}
				}
				else {
					// Case: halt is removed / no transfer halt -> put in closed list
					halt_data[ reachable_halt_id ].best_weight = 0;
				}

			}	// if not processed before
			else if(  halt_data[ reachable_halt_id ].best_weight!=0  ) {
				// Case : processed before but not in closed list : that is, in open list
				//			--> can only be destination halt or transfer halt

				uint16 total_weight = current_halt_data.best_weight + current_conn.weight;

				if(  total_weight<halt_data[ reachable_halt_id ].best_weight  &&  total_weight<best_destination_weight  &&  allocation_pointer<max_hops  ) {
					// new weight is lower than lowest weight --> create new node and update halt data

					halt_data[ reachable_halt_id ].best_weight = total_weight;
					// no need to update destination, as halt nature (as destination or transfer) will not change
					halt_data[ reachable_halt_id ].depth       = current_halt_data.depth + 1u;
					halt_data[ reachable_halt_id ].transfer    = current_node.halt;
					halt_data[ reachable_halt_id ].overcrowded = overcrowded_transfer;
					overcrowded_nodes                         += overcrowded_transfer;

					if (halt_data[ reachable_halt_id ].destination) {
						best_destination_weight = total_weight;
					}
					else {
						// as the next halt is not a destination add WEIGHT_MIN
						total_weight += WEIGHT_MIN;
					}

					allocation_pointer++;
					open_list.insert( route_node_t(current_conn.halt, total_weight) );
				}
			}	// else if not in closed list
		}	// for each connection entry

		// indicate that the current halt is in closed list
		current_halt_data.best_weight = 0;

	} while(  !open_list.empty()  );

	// if the loop ends, nothing was found
	ware.set_ziel( halthandle_t() );
	ware.set_zwischenziel( halthandle_t() );
	if(  return_ware  ) {
		return_ware->set_ziel( halthandle_t() );
		return_ware->set_zwischenziel( halthandle_t() );
	}
	return NO_ROUTE;
}


void haltestelle_t::search_route_resumable(  ware_t &ware   )
{
	const uint8 ware_catg_idx = ware.get_besch()->get_catg_index();

	// continue search if start halt and good category did not change
	const bool resume_search = last_search_origin == self  &&  ware_catg_idx == last_search_ware_catg_idx;

	if (!resume_search) {
		last_search_origin = self;
		last_search_ware_catg_idx = ware_catg_idx;
		// set current marker
		++current_marker;
		if(  current_marker==0  ) {
			MEMZERON(markers, halthandle_t::get_size());
			current_marker = 1u;
		}
	}

	// remember destination nodes, to reset them before returning
	static vector_tpl<uint16> dest_indices(16);
	dest_indices.clear();

	uint16 best_destination_weight = 65535u;

	// reset next transfer and destination halt to null -> if they remain null after search, no route can be found
	ware.set_ziel( halthandle_t() );
	ware.set_zwischenziel( halthandle_t() );
	// find suitable destination halts for the ware packet's target position
	const planquadrat_t *const plan = welt->lookup( ware.get_zielpos() );
	const halthandle_t *const halt_list = plan->get_haltlist();

	// check halt list for presence of current halt
	for( uint8 h = 0;  h<plan->get_haltlist_count(); ++h  ) {
		if (halt_list[h] == self) {
			// a destination halt is the same as the current halt -> no route searching is necessary
			ware.set_ziel( self );
			return;
		}
	}

	// check explored connection
	if (resume_search) {
		for( uint8 h=0;  h<plan->get_haltlist_count();  ++h  ) {
			const halthandle_t halt = halt_list[h];
			if (markers[ halt.get_id() ]==current_marker  &&  halt_data[ halt.get_id() ].best_weight < best_destination_weight  &&  halt.is_bound()) {
				best_destination_weight = halt_data[ halt.get_id() ].best_weight;
				ware.set_ziel( halt );
				ware.set_zwischenziel( halt_data[ halt.get_id() ].transfer );
			}
		}
		// for all halts with halt_data.weight < explored_weight one of the best routes is found
		const uint16 explored_weight = open_list.empty()  ? 65535u : open_list.front().aggregate_weight;

		if (best_destination_weight <= explored_weight) {
			// we explored best route to this destination in last run
			// (any other not yet explored connection will have larger weight)
			// no need to search route for this ware
			return;
		}
	}
	// find suitable destination halt(s), if any
	for( uint8 h=0;  h<plan->get_haltlist_count();  ++h  ) {
		const halthandle_t halt = halt_list[h];
		if(  halt.is_bound()  &&  halt->is_enabled(ware_catg_idx)  ) {
			// initialisations for destination halts => save some checking inside search loop
			if(  markers[ halt.get_id() ]!=current_marker  ) {
				// first time -> initialise marker and all halt data
				markers[ halt.get_id() ] = current_marker;
				halt_data[ halt.get_id() ].best_weight = 65535u;
				halt_data[ halt.get_id() ].destination = true;
			}
			else {
				// initialised before -> only update destination bit set
				halt_data[ halt.get_id() ].destination = true;
			}
			dest_indices.append(halt.get_id());
		}
	}

	if(  dest_indices.empty()  ) {
		// no destination halt found or current halt is the same as (all) the destination halt(s)
		return;
	}

	uint16 const max_transfers = welt->get_settings().get_max_transfers();
	uint16 const max_hops      = welt->get_settings().get_max_hops();

	static uint16 allocation_pointer;
	if (!resume_search) {
		open_list.clear();

		// initialise the origin node
		allocation_pointer = 1u;
		open_list.insert( route_node_t(self, 0) );

		halt_data_t & start_data = halt_data[ self.get_id() ];
		start_data.best_weight = 0;
		start_data.destination = 0;
		start_data.depth       = 0;
		start_data.transfer    = halthandle_t();

		markers[ self.get_id() ] = current_marker;
	}

	while(  !open_list.empty()  ) {

		if (best_destination_weight <= open_list.front().aggregate_weight) {
			// best route to destination found already
			break;
		}

		route_node_t current_node = open_list.pop();

		const uint16 current_halt_id = current_node.halt.get_id();
		const uint16 current_weight = current_node.aggregate_weight;
		halt_data_t & current_halt_data = halt_data[ current_halt_id ];

		// check if the current halt is already in closed list (or removed)
		if(  !current_node.halt.is_bound()  ) {
			continue;
		}
		else if(  current_halt_data.best_weight < current_weight) {
			// shortest path to the current halt has already been found earlier
			// assert(markers[ current_halt_id ]==current_marker);
			continue;
		}
		else {
			// no need to update weight, as it is already the right one
			// assert(current_halt_data.best_weight == current_weight);
		}

		if(  current_halt_data.destination  ) {
			// destination found
			ware.set_ziel( current_node.halt );
			ware.set_zwischenziel( current_halt_data.transfer );
			// update best_destination_weight to leave loop due to first check above
			best_destination_weight = current_weight;
			// if this destination halt is not a transfer halt -> do not proceed to process its reachable halt(s)
			if(  !current_node.halt->is_transfer(ware_catg_idx)  ) {
				continue;
			}
		}

		if(  current_halt_data.depth > max_transfers  ) {
			// maximum transfer limit is reached -> do not add reachable halts to open list
			continue;
		}

		FOR(vector_tpl<connection_t>, const& current_conn, current_node.halt->connections[ware_catg_idx]) {
			const uint16 reachable_halt_id = current_conn.halt.get_id();

			const uint16 total_weight = current_weight + current_conn.weight;

			if(  !current_conn.halt.is_bound()  ) {
				// Case: halt removed -> make sure we never visit it again
				markers[ reachable_halt_id ] = current_marker;
				halt_data[ reachable_halt_id ].best_weight = 0;
			}
			else if(  markers[ reachable_halt_id ]!=current_marker  ) {
				// Case : not processed before and not destination

				// indicate that this halt has been processed
				markers[ reachable_halt_id ] = current_marker;

				// update data
				halt_data[ reachable_halt_id ].best_weight = total_weight;
				halt_data[ reachable_halt_id ].destination = false; // reset necessary if this was set by search_route
				halt_data[ reachable_halt_id ].depth       = current_halt_data.depth + 1u;
				halt_data[ reachable_halt_id ].transfer    = current_halt_data.transfer.get_id() ? current_halt_data.transfer : current_conn.halt;

				if(  current_conn.halt->is_transfer(ware_catg_idx)  &&  allocation_pointer<max_hops  ) {
					// Case : transfer halt
					allocation_pointer++;
					open_list.insert( route_node_t(current_conn.halt, total_weight) );
				}
			}	// if not processed before
			else {
				// Case : processed before (or destination halt)
				//        -> need to check whether we can reach it with smaller weight
				if(  total_weight<halt_data[ reachable_halt_id ].best_weight  ) {
					// new weight is lower than lowest weight --> update halt data

					halt_data[ reachable_halt_id ].best_weight = total_weight;
					halt_data[ reachable_halt_id ].transfer    = current_halt_data.transfer.get_id() ? current_halt_data.transfer : current_conn.halt;

					// for transfer/destination nodes create new node
					if ( (halt_data[ reachable_halt_id ].destination  ||  current_conn.halt->is_transfer(ware_catg_idx) )  &&  allocation_pointer<max_hops ) {
						halt_data[ reachable_halt_id ].depth = current_halt_data.depth + 1u;
						allocation_pointer++;
						open_list.insert( route_node_t(current_conn.halt, total_weight) );
					}
				}
			}	// else processed before
		}	// for each connection entry
	}

	// clear destinations since we may want to do another search with the same current_marker
	FOR(vector_tpl<uint16>, const i, dest_indices) {
		halt_data[i].destination = false;
		if (halt_data[i].best_weight == 65535u) {
			// not processed -> reset marker
			--markers[i];
		}
	}
}


/**
 * Found route and station uncrowded
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_happy(int n)
{
	book(n, HALT_HAPPY);
	recalc_status();
}


/**
 * Station in wlaking distance
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_walked(int n)
{
	book(n, HALT_WALKED);
}


/**
 * Station crowded
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_unhappy(int n)
{
	book(n, HALT_UNHAPPY);
	recalc_status();
}


/**
 * Found no route
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_no_route(int n)
{
	book(n, HALT_NOROUTE);
}



void haltestelle_t::liefere_an_fabrik(const ware_t& ware) const
{
	fabrik_t *const factory = fabrik_t::get_fab( welt, ware.get_zielpos() );
	if(  factory  ) {
		factory->liefere_an(ware.get_besch(), ware.menge);
	}
}


/* retrieves a ware packet for any destination in the list
 * needed, if the factory in question wants to remove something
 */
bool haltestelle_t::recall_ware( ware_t& w, uint32 menge )
{
	w.menge = 0;
	vector_tpl<ware_t> *warray = waren[w.get_besch()->get_catg_index()];
	if(warray!=NULL) {
		FOR(vector_tpl<ware_t>, & tmp, *warray) {
			// skip empty entries
			if(tmp.menge==0  ||  w.get_index()!=tmp.get_index()  ||  w.get_zielpos()!=tmp.get_zielpos()) {
				continue;
			}

			// not too much?
			if(tmp.menge > menge) {
				// not all can be loaded
				tmp.menge -= menge;
				w.menge = menge;
			}
			else {
				// leave an empty entry => joining will more often work
				w.menge = tmp.menge;
				tmp.menge = 0;
			}
			book(w.menge, HALT_ARRIVED);
			fabrik_t::update_transit( &w, false );
			resort_freight_info = true;
			return true;
		}
	}
	// nothing to take out
	return false;
}



// will load something compatible with wtyp into the car which schedule is fpl
void haltestelle_t::hole_ab( slist_tpl<ware_t> &fracht, const ware_besch_t *wtyp, uint32 maxi, const schedule_t *fpl, const spieler_t *sp )
{
	// prissi: first iterate over the next stop, then over the ware
	// might be a little slower, but ensures that passengers to nearest stop are served first
	// this allows for separate high speed and normal service
	vector_tpl<ware_t> *warray = waren[wtyp->get_catg_index()];

	if (warray && !warray->empty()) {
		// da wir schon an der aktuellem haltestelle halten
		// startet die schleife ab 1, d.h. dem naechsten halt
		const uint8 count = fpl->get_count();
		for(  uint8 i=1; i<count; i++  ) {
			const uint8 wrap_i = (i + fpl->get_aktuell()) % count;

			const halthandle_t plan_halt = haltestelle_t::get_halt(welt, fpl->eintrag[wrap_i].pos, sp);
			if(plan_halt == self) {
				// we will come later here again ...
				break;
			}
			else if(  plan_halt.is_bound()  ) {

				// The random offset will ensure that all goods have an equal chance to be loaded.
				sint32 offset = simrand(warray->get_count());
				for(  uint32 i=0;  i<warray->get_count();  i++  ) {
					ware_t &tmp = (*warray)[ i+offset ];

					// prevent overflow (faster than division)
					if(  i+offset+1>=warray->get_count()  ) {
						offset -= warray->get_count();
					}

					// skip empty entries
					if(tmp.menge==0) {
						continue;
					}

					// goods without route -> returning passengers/mail
					if(  !tmp.get_zwischenziel().is_bound()  ) {
						search_route_resumable(tmp);
						if (!tmp.get_ziel().is_bound()) {
							// no route anymore
							tmp.menge = 0;
							continue;
						}
					}

					// compatible car and right target stop?
					if(  tmp.get_zwischenziel()==plan_halt  ) {

						if(  plan_halt->is_overcrowded(wtyp->get_catg_index())  ) {
							if (welt->get_settings().is_avoid_overcrowding() && tmp.get_ziel() != plan_halt) {
								// do not go for transfer to overcrowded transfer stop
								continue;
							}
						}

						// not too much?
						ware_t neu(tmp);
						if(  tmp.menge > maxi  ) {
							// not all can be loaded
							neu.menge = maxi;
							tmp.menge -= maxi;
							maxi = 0;
						}
						else {
							maxi -= tmp.menge;
							// leave an empty entry => joining will more often work
							tmp.menge = 0;
						}
						fracht.insert(neu);

						book(neu.menge, HALT_DEPARTED);
						resort_freight_info = true;

						if (maxi==0) {
							return;
						}
					}
				}

				// nothing there to load
			}
		}
	}
}



uint32 haltestelle_t::get_ware_summe(const ware_besch_t *wtyp) const
{
	int sum = 0;
	const vector_tpl<ware_t> * warray = waren[wtyp->get_catg_index()];
	if(warray!=NULL) {
		FOR(vector_tpl<ware_t>, const& i, *warray) {
			if (wtyp->get_index() == i.get_index()) {
				sum += i.menge;
			}
		}
	}
	return sum;
}



uint32 haltestelle_t::get_ware_fuer_zielpos(const ware_besch_t *wtyp, const koord zielpos) const
{
	const vector_tpl<ware_t> * warray = waren[wtyp->get_catg_index()];
	if(warray!=NULL) {
		FOR(vector_tpl<ware_t>, const& ware, *warray) {
			if(wtyp->get_index()==ware.get_index()  &&  ware.get_zielpos()==zielpos) {
				return ware.menge;
			}
		}
	}
	return 0;
}


bool haltestelle_t::vereinige_waren(const ware_t &ware)
{
	// pruefen ob die ware mit bereits wartender ware vereinigt werden kann
	vector_tpl<ware_t> * warray = waren[ware.get_besch()->get_catg_index()];
	if(warray!=NULL) {
		FOR(vector_tpl<ware_t>, & tmp, *warray) {
			// join packets with same destination
			if(ware.same_destination(tmp)) {
				if(  ware.get_zwischenziel().is_bound()  &&  ware.get_zwischenziel()!=self  ) {
					// update route if there is newer route
					tmp.set_zwischenziel( ware.get_zwischenziel() );
				}
				tmp.menge += ware.menge;
				resort_freight_info = true;
				return true;
			}
		}
	}
	return false;
}



// put the ware into the internal storage
// take care of all allocation neccessary
void haltestelle_t::add_ware_to_halt(ware_t ware)
{
	// now we have to add the ware to the stop
	vector_tpl<ware_t> * warray = waren[ware.get_besch()->get_catg_index()];
	if(warray==NULL) {
		// this type was not stored here before ...
		warray = new vector_tpl<ware_t>(4);
		waren[ware.get_besch()->get_catg_index()] = warray;
	}
	// the ware will be put into the first entry with menge==0
	resort_freight_info = true;
	FOR(vector_tpl<ware_t>, & i, *warray) {
		if (i.menge == 0) {
			i = ware;
			return;
		}
	}
	// here, if no free entries found
	warray->append(ware);
}



/* same as liefere an, but there will be no route calculated,
 * since it hase be calculated just before
 * (execption: route contains us as intermediate stop)
 * @author prissi
 */
uint32 haltestelle_t::starte_mit_route(ware_t ware)
{
	if(ware.get_ziel()==self) {
		if(  ware.to_factory  ) {
			// muss an fabrik geliefert werden
			liefere_an_fabrik(ware);
		}
		// already there: finished (may be happen with overlapping areas and returning passengers)
		return ware.menge;
	}

	// no valid next stops? Or we are the next stop?
	if(ware.get_zwischenziel()==self) {
		dbg->error("haltestelle_t::starte_mit_route()","route cannot contain us as first transfer stop => recalc route!");
		if(  search_route( &self, 1u, false, ware )==NO_ROUTE  ) {
			// no route found?
			dbg->error("haltestelle_t::starte_mit_route()","no route found!");
			return ware.menge;
		}
	}

	// passt das zu bereits wartender ware ?
	if(vereinige_waren(ware)) {
		// dann sind wir schon fertig;
		return ware.menge;
	}

	// add to internal storage
	add_ware_to_halt(ware);

	return ware.menge;
}



/* Recieves ware and tries to route it further on
 * if no route is found, it will be removed
 * @author prissi
 */
uint32 haltestelle_t::liefere_an(ware_t ware)
{
	// no valid next stops?
	if(!ware.get_ziel().is_bound()  ||  !ware.get_zwischenziel().is_bound()) {
		// write a log entry and discard the goods
dbg->warning("haltestelle_t::liefere_an()","%d %s delivered to %s have no longer a route to their destination!", ware.menge, translator::translate(ware.get_name()), get_name() );
		return ware.menge;
	}

	// did we arrived?
	if(  welt->lookup(ware.get_zielpos())->is_connected(self)  ) {
		if(  ware.to_factory  ) {
			// muss an fabrik geliefert werden
			liefere_an_fabrik(ware);
		}
		else if(  ware.get_besch() == warenbauer_t::passagiere  ) {
			// arriving passenger may create pedestrians
			if(  welt->get_settings().get_show_pax()  ) {
				int menge = ware.menge;
				FOR( slist_tpl<tile_t>, const& i, tiles ) {
					if (menge <= 0) {
						break;
					}
					menge = erzeuge_fussgaenger(welt, i.grund->get_pos(), menge);
				}
				INT_CHECK("simhalt 938");
			}
		}
		return ware.menge;
	}

	// do we have already something going in this direction here?
	if(  vereinige_waren(ware)  ) {
		return ware.menge;
	}

	// not near enough => we need to do a rerouting
	search_route_resumable(ware);
	if (!ware.get_ziel().is_bound()) {
		DBG_MESSAGE("haltestelle_t::liefere_an()","%s: delivered goods (%d %s) to ??? via ??? could not be routed to their destination!",get_name(), ware.menge, translator::translate(ware.get_name()) );
		// target halt no longer there => delete and remove from fab in transit
		fabrik_t::update_transit( &ware, false );
		return ware.menge;
	}
	// passt das zu bereits wartender ware ?
	if(vereinige_waren(ware)) {
		// dann sind wir schon fertig;
		return ware.menge;
	}
	// add to internal storage
	add_ware_to_halt(ware);

	return ware.menge;
}


void haltestelle_t::info(cbuffer_t & buf) const
{
	if(  translator::get_lang()->utf_encoded  ) {
		utf8 happy[4], unhappy[4];
		happy[ utf16_to_utf8( 0x263A, happy ) ] = 0;
		unhappy[ utf16_to_utf8( 0x2639, unhappy ) ] = 0;
		buf.printf(translator::translate("Passengers %d %s, %d %s, %d no route"), get_pax_happy(), happy, get_pax_unhappy(), unhappy, get_pax_no_route());
	}
	else {
		buf.printf(translator::translate("Passengers %d %c, %d %c, %d no route"), get_pax_happy(), 30, get_pax_unhappy(), 31, get_pax_no_route());
	}
	buf.append("\n\n");
}


/**
 * @param buf the buffer to fill
 * @return Goods description text (buf)
 * @author Hj. Malthaner
 */
void haltestelle_t::get_freight_info(cbuffer_t & buf)
{
	if(resort_freight_info) {
		// resort only inf absolutely needed ...
		resort_freight_info = false;
		buf.clear();

		for(unsigned i=0; i<warenbauer_t::get_max_catg_index(); i++) {
			const vector_tpl<ware_t> * warray = waren[i];
			if(warray) {
				freight_list_sorter_t::sort_freight(*warray, buf, (freight_list_sorter_t::sort_mode_t)sortierung, NULL, "waiting", welt);
			}
		}
	}
}



void haltestelle_t::get_short_freight_info(cbuffer_t & buf) const
{
	bool got_one = false;

	for(unsigned int i=0; i<warenbauer_t::get_waren_anzahl(); i++) {
		const ware_besch_t *wtyp = warenbauer_t::get_info(i);
		if(gibt_ab(wtyp)) {

			// ignore goods with sum=zero
			const int summe=get_ware_summe(wtyp);
			if(summe>0) {

				if(got_one) {
					buf.append(", ");
				}

				buf.printf("%d%s %s", summe, translator::translate(wtyp->get_mass()), translator::translate(wtyp->get_name()));

				got_one = true;
			}
		}
	}

	if(got_one) {
		buf.append(" ");
		buf.append(translator::translate("waiting"));
		buf.append("\n");
	}
	else {
		buf.append(translator::translate("no goods waiting"));
		buf.append("\n");
	}
}



void haltestelle_t::zeige_info()
{
	create_win( new halt_info_t(welt, self), w_info, magic_halt_info + self.get_id() );
}


sint64 haltestelle_t::calc_maintenance() const
{
	sint64 maintenance = 0;
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (gebaeude_t* const gb = i.grund->find<gebaeude_t>()) {
			maintenance += welt->get_settings().maint_building * gb->get_tile()->get_besch()->get_level();
		}
	}
	return maintenance;
}



// changes this to a public transfer exchange stop
void haltestelle_t::make_public_and_join( spieler_t *sp )
{
	spieler_t *public_owner=welt->get_spieler(1);
	sint64 total_costs = 0;
	slist_tpl<halthandle_t> joining;

	// only something to do if not yet owner ...
	if(besitzer_p!=public_owner) {
		// now recalculate maintenance
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			grund_t* const gr = i.grund;
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb) {
				spieler_t *gb_sp=gb->get_besitzer();
				sint64 const costs = welt->get_settings().maint_building * gb->get_tile()->get_besch()->get_level();
				total_costs += costs;
				spieler_t::add_maintenance( gb_sp, (sint32)-costs );
				gb->set_besitzer(public_owner);
				gb->set_flag(ding_t::dirty);
				spieler_t::add_maintenance(public_owner, (sint32)costs );
			}
			// ok, valid start, now we can join them
			for( uint8 i=0;  i<8;  i++  ) {
				const planquadrat_t *pl2 = welt->lookup(gr->get_pos().get_2d()+koord::neighbours[i]);
				if(  pl2  ) {
					halthandle_t halt = pl2->get_halt();
					if(  halt.is_bound()  &&  halt->get_besitzer()==public_owner  &&  !joining.is_contained(halt)  ) {
						joining.append(halt);
					}
				}
			}
		}
		// transfer ownership
		spieler_t::accounting( sp, -total_costs*60, get_basis_pos(), COST_CONSTRUCTION);
		besitzer_p->halt_remove(self);
		besitzer_p = public_owner;
		public_owner->halt_add(self);
	}

	// set name to name of first public stop
	if(  !joining.empty()  ) {
		set_name( joining.front()->get_name());
	}

	while(!joining.empty()) {
		// join this halt with me
		halthandle_t halt = joining.remove_first();

		// now with the second stop
		while(  halt.is_bound()  &&  halt!=self  ) {
			// add statistics
			for(  int month=0;  month<MAX_MONTHS;  month++  ) {
				for(  int type=0;  type<MAX_HALT_COST;  type++  ) {
					financial_history[month][type] += halt->financial_history[month][type];
					halt->financial_history[month][type] = 0;	// to avoind counting twice
				}
			}

			// we always take the first remaining tile and transfer it => more safe
			koord3d t = halt->get_basis_pos3d();
			grund_t *gr = welt->lookup(t);
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if(gb) {
				// there are also water tiles, which may not have a buidling
				spieler_t *gb_sp=gb->get_besitzer();
				if(public_owner!=gb_sp) {
					spieler_t *gb_sp=gb->get_besitzer();
					sint64 const costs = welt->get_settings().maint_building * gb->get_tile()->get_besch()->get_level();
					spieler_t::add_maintenance( gb_sp, (sint32)-costs );
					spieler_t::accounting(gb_sp, costs*60, gr->get_pos().get_2d(), COST_CONSTRUCTION);
					gb->set_besitzer(public_owner);
					gb->set_flag(ding_t::dirty);
					spieler_t::add_maintenance( public_owner, (sint32)costs );
				}
			}
			// transfer tiles to us
			halt->rem_grund(gr);
			add_grund(gr);
			// and check for existence
			if(!halt->existiert_in_welt()) {
				// transfer goods
				halt->transfer_goods(self);

				destroy(halt);
			}
		}
	}

	// tell the world of it ...
	if(  sp->get_player_nr()!=1  &&  umgebung_t::networkmode  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("%s at (%i,%i) now public stop."), get_name(), get_basis_pos().x, get_basis_pos().y );
		welt->get_message()->add_message( buf, get_basis_pos(), message_t::ai, PLAYER_FLAG|sp->get_player_nr(), IMG_LEER );
	}

	recalc_station_type();
}


void haltestelle_t::transfer_goods(halthandle_t halt)
{
	if (!self.is_bound() || !halt.is_bound()) {
		return;
	}
	// transfer goods to halt
	for(uint8 i=0; i<warenbauer_t::get_max_catg_index(); i++) {
		const vector_tpl<ware_t> * warray = waren[i];
		if (warray) {
			FOR(vector_tpl<ware_t>, const& j, *warray) {
				halt->add_ware_to_halt(j);
			}
			delete waren[i];
			waren[i] = NULL;
		}
	}
}


// private helper function for recalc_station_type()
void haltestelle_t::add_to_station_type( grund_t *gr )
{
	// init in any case ...
	if(  tiles.empty()  ) {
		capacity[0] = 0;
		capacity[1] = 0;
		capacity[2] = 0;
		enables &= CROWDED;	// clear flags
		station_type = invalid;
	}

	const gebaeude_t* gb = gr->find<gebaeude_t>();
	const haus_besch_t *besch=gb?gb->get_tile()->get_besch():NULL;

	if(  gr->ist_wasser()  &&  gb  ) {
		// may happend around oil rigs and so on
		station_type |= dock;
		// for water factories
		if(besch) {
			// enabled the matching types
			enables |= besch->get_enabled();
			if (welt->get_settings().is_seperate_halt_capacities()) {
				if(besch->get_enabled()&1) {
					capacity[0] += besch->get_level()*32;
				}
				if(besch->get_enabled()&2) {
					capacity[1] += besch->get_level()*32;
				}
				if(besch->get_enabled()&4) {
					capacity[2] += besch->get_level()*32;
				}
			}
			else {
				// no sperate capacities: sum up all
				capacity[0] += besch->get_level()*32;
				capacity[2] = capacity[1] = capacity[0];
			}
		}
		return;
	}

	if(  besch==NULL  ) {
		// no besch, but solid gound?!?
		dbg->error("haltestelle_t::get_station_type()","ground belongs to halt but no besch?");
		return;
	}

	// there is only one loading bay ...
	switch (besch->get_utyp()) {
		case haus_besch_t::ladebucht:    station_type |= loadingbay;   break;
		case haus_besch_t::hafen:
		case haus_besch_t::binnenhafen:  station_type |= dock;         break;
		case haus_besch_t::bushalt:      station_type |= busstop;      break;
		case haus_besch_t::airport:      station_type |= airstop;      break;
		case haus_besch_t::monorailstop: station_type |= monorailstop; break;

		case haus_besch_t::bahnhof:
			if (gr->hat_weg(monorail_wt)) {
				station_type |= monorailstop;
			}
			else {
				station_type |= railstation;
			}
			break;

		// two ways on ground can only happen for tram tracks on streets, there buses and trams can stop
		case haus_besch_t::generic_stop:
			switch (besch->get_extra()) {
				case road_wt:
					station_type |= (besch->get_enabled()&3)!=0 ? busstop : loadingbay;
					if (gr->has_two_ways()) { // tram track on street
						station_type |= tramstop;
					}
					break;
				case water_wt:       station_type |= dock;            break;
				case air_wt:         station_type |= airstop;         break;
				case monorail_wt:    station_type |= monorailstop;    break;
				case track_wt:       station_type |= railstation;     break;
				case tram_wt:
					station_type |= tramstop;
					if (gr->has_two_ways()) { // tram track on street
						station_type |= (besch->get_enabled()&3)!=0 ? busstop : loadingbay;
					}
					break;
				case maglev_wt:      station_type |= maglevstop;      break;
				case narrowgauge_wt: station_type |= narrowgaugestop; break;
				default: ;
			}
			break;
		default: ;
	}

	// enabled the matching types
	enables |= besch->get_enabled();
	if (welt->get_settings().is_seperate_halt_capacities()) {
		if(besch->get_enabled()&1) {
			capacity[0] += besch->get_level()*32;
		}
		if(besch->get_enabled()&2) {
			capacity[1] += besch->get_level()*32;
		}
		if(besch->get_enabled()&4) {
			capacity[2] += besch->get_level()*32;
		}
	}
	else {
		// no sperate capacities: sum up all
		capacity[0] += besch->get_level()*32;
		capacity[2] = capacity[1] = capacity[0];
	}
}

/*
 * recalculated the station type(s)
 * since it iterates over all ground, this is better not done too often, because line management and station list
 * queries this information regularely; Thus, we do this, when adding new ground
 * This recalculates also the capacity from the building levels ...
 * @author Weber/prissi
 */
void haltestelle_t::recalc_station_type()
{
	capacity[0] = 0;
	capacity[1] = 0;
	capacity[2] = 0;
	enables &= CROWDED;	// clear flags
	station_type = invalid;

	// iterate over all tiles
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		grund_t* const gr = i.grund;
		add_to_station_type( gr );
	}
	// and set halt info again
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		i.grund->set_halt( self );
	}
	recalc_status();
}



int haltestelle_t::erzeuge_fussgaenger(karte_t *welt, koord3d pos, int anzahl)
{
	fussgaenger_t::erzeuge_fussgaenger_an(welt, pos, anzahl);
	for(int i=0; i<4 && anzahl>0; i++) {
		fussgaenger_t::erzeuge_fussgaenger_an(welt, pos+koord::nsow[i], anzahl);
	}
	return anzahl;
}



void haltestelle_t::rdwr(loadsave_t *file)
{
	xml_tag_t h( file, "haltestelle_t" );

	sint32 spieler_n;
	koord3d k;

	// will restore halthandle_t after loading
	if(file->get_version() > 110005) {
		if(file->is_saving()) {
			uint16 halt_id = self.is_bound() ? self.get_id() : 0;
			file->rdwr_short(halt_id);
		}
		else {
			uint16 halt_id;
			file->rdwr_short(halt_id);
			self.set_id(halt_id);
			self = halthandle_t(this, halt_id);
		}
	}
	else {
		if (file->is_loading()) {
			self = halthandle_t(this);
		}
	}

	if(file->is_saving()) {
		spieler_n = welt->sp2num( besitzer_p );
	}

	if(file->get_version()<99008) {
		init_pos.rdwr( file );
	}
	file->rdwr_long(spieler_n);

	if(file->get_version()<=88005) {
		bool dummy;
		file->rdwr_bool(dummy); // pax
		file->rdwr_bool(dummy); // post
		file->rdwr_bool(dummy);	// ware
	}

	if(file->is_loading()) {
		besitzer_p = welt->get_spieler(spieler_n);
		k.rdwr( file );
		while(k!=koord3d::invalid) {
			grund_t *gr = welt->lookup(k);
			if(!gr) {
				dbg->error("haltestelle_t::rdwr()", "invalid position %s", k.get_str() );
				gr = welt->lookup_kartenboden(k.get_2d());
				dbg->error("haltestelle_t::rdwr()", "setting to %s", gr->get_pos().get_str() );
			}
			// during loading and saving halts will be referred by their base postion
			// so we may alrady be defined ...
			if(gr->get_halt().is_bound()) {
				dbg->warning( "haltestelle_t::rdwr()", "bound to ground twice at (%i,%i)!", k.x, k.y );
			}
			// prissi: now check, if there is a building -> we allow no longer ground without building!
			const gebaeude_t* gb = gr->find<gebaeude_t>();
			const haus_besch_t *besch=gb?gb->get_tile()->get_besch():NULL;
			if(besch) {
				add_grund( gr );
			}
			else {
				dbg->warning("haltestelle_t::rdwr()", "will no longer add ground without building at %s!", k.get_str() );
			}
			k.rdwr( file );
		}
	}
	else {
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			k = i.grund->get_pos();
			k.rdwr( file );
		}
		k = koord3d::invalid;
		k.rdwr( file );
	}

	short count;
	const char *s;
	init_pos = tiles.empty() ? koord::invalid : tiles.front().grund->get_pos().get_2d();
	if(file->is_saving()) {
		for(unsigned i=0; i<warenbauer_t::get_max_catg_index(); i++) {
			vector_tpl<ware_t> *warray = waren[i];
			if(warray) {
				s = "y";	// needs to be non-empty
				file->rdwr_str(s);
				count = warray->get_count();
				file->rdwr_short(count);
				FOR(vector_tpl<ware_t>, & ware, *warray) {
					ware.rdwr(welt,file);
				}
			}
		}
		s = "";
		file->rdwr_str(s);

	}
	else {
		// restoring all goods in the station
		char s[256];
		file->rdwr_str(s, lengthof(s));
		while(*s) {
			file->rdwr_short(count);
			if(count>0) {
				for(int i = 0; i < count; i++) {
					// add to internal storage (use this function, since the old categories were different)
					ware_t ware(welt,file);
					if(  ware.menge>0  &&  welt->ist_in_kartengrenzen(ware.get_zielpos())  ) {
						add_ware_to_halt(ware);
						if(  file->get_version() <= 112000  ) {
							// restore intransit information
							fabrik_t::update_transit( &ware, true );
						}
					}
					else if(  ware.menge>0  ) {
						dbg->error( "haltestelle_t::rdwr()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str() );
					}
				}
			}
			file->rdwr_str(s, lengthof(s));
		}

		// old games save the list with stations
		// however, we have to rebuilt them anyway for the new format
		if(file->get_version()<99013) {
			file->rdwr_short(count);
			warenziel_t dummy;
			for(int i=0; i<count; i++) {
				dummy.rdwr(file);
			}
		}

	}

	if(  file->get_version()>=111001  ) {
		for (int j = 0; j<MAX_HALT_COST; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
	}
	else {
		// old history did not know about walked pax
		for (int j = 0; j<7; j++) {
			for (size_t k = MAX_MONTHS; k-- != 0;) {
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][HALT_WALKED] = 0;
		}
	}
}



void haltestelle_t::laden_abschliessen()
{
	stale_convois.clear();
	stale_lines.clear();
	// fix good destination coordinates
	for(unsigned i=0; i<warenbauer_t::get_max_catg_index(); i++) {
		if(waren[i]) {
			vector_tpl<ware_t> * warray = waren[i];
			FOR(vector_tpl<ware_t>, & j, *warray) {
				j.laden_abschliessen(welt, besitzer_p);
			}
			// merge identical entries (should only happen with old games)
			for(unsigned j=0; j<warray->get_count(); j++) {
				if(  (*warray)[j].menge==0  ) {
					continue;
				}
				for(unsigned k=j+1; k<warray->get_count(); k++) {
					if(  (*warray)[k].menge>0  &&  (*warray)[j].same_destination( (*warray)[k] )  ) {
						(*warray)[j].menge += (*warray)[k].menge;
						(*warray)[k].menge = 0;
					}
				}
			}
		}
	}

	// what kind of station here?
	recalc_station_type();

	// handle name for old stations which don't exist in kartenboden
	grund_t* bd = welt->lookup(get_basis_pos3d());
	if(bd!=NULL  &&  !bd->get_flag(grund_t::has_text) ) {
		// restore label und bridges
		grund_t* bd_old = welt->lookup_kartenboden(get_basis_pos());
		if(bd_old) {
			// transfer name (if there)
			const char *name = bd->get_text();
			if(name) {
				set_name( name );
				bd_old->set_text( NULL );
			}
			else {
				set_name( "Unknown" );
			}
		}
	}
	else {
		const char *current_name = bd->get_text();
		if(  all_names.get(current_name).is_bound()  &&  fabrik_t::get_fab(welt, get_basis_pos())==NULL  ) {
			// try to get a new name ...
			const char *new_name;
			if(  station_type & airstop  ) {
				new_name = create_name( get_basis_pos(), "Airport" );
			}
			else if(  station_type & dock  ) {
				new_name = create_name( get_basis_pos(), "Dock" );
			}
			else if(  station_type & (railstation|monorailstop|maglevstop|narrowgaugestop)  ) {
				new_name = create_name( get_basis_pos(), "BF" );
			}
			else {
				new_name = create_name( get_basis_pos(), "H" );
			}
			dbg->warning("haltestelle_t::set_name()","name already used: \'%s\' -> \'%s\'", current_name, new_name );
			bd->set_text( new_name );
			current_name = new_name;
		}
		all_names.set( current_name, self );
	}
	recalc_status();
	reconnect_counter = welt->get_schedule_counter()-1;
}



void haltestelle_t::book(sint64 amount, int cost_type)
{
	assert(cost_type <= MAX_HALT_COST);
	financial_history[0][cost_type] += amount;
}



void haltestelle_t::init_financial_history()
{
	for (int j = 0; j<MAX_HALT_COST; j++)
	{
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][j] = 0;
		}
	}
}



/**
 * Calculates a status color for status bars
 * @author Hj. Malthaner
 */
void haltestelle_t::recalc_status()
{
	status_color = financial_history[0][HALT_CONVOIS_ARRIVED] > 0 ? COL_GREEN : COL_YELLOW;

	// since the status is ordered ...
	uint8 status_bits = 0;

	MEMZERO(overcrowded);

	long total_sum = 0;
	if(get_pax_enabled()) {
		sint32 max_ware = get_capacity(0);
		total_sum += get_ware_summe(warenbauer_t::passagiere);
		if(total_sum>max_ware) {
			overcrowded[0] |= 1;
		}
		if(get_pax_unhappy() > 40 ) {
			status_bits = (total_sum>max_ware+200 || (total_sum>max_ware  &&  get_pax_unhappy()>200)) ? 2 : 1;
		}
		else if(total_sum>max_ware) {
			status_bits = total_sum>max_ware+200 ? 2 : 1;
		}
	}

	if(get_post_enabled()) {
		sint32 max_ware = get_capacity(1);
		sint32 post = get_ware_summe(warenbauer_t::post);
		total_sum += post;
		if(post>max_ware) {
			status_bits |= post>max_ware+200 ? 2 : 1;
			overcrowded[0] |= 2;
		}
	}

	// now for all goods
	if(status_color!=COL_RED  &&  get_ware_enabled()) {
		const int count = warenbauer_t::get_waren_anzahl();
		sint32 max_ware = get_capacity(2);
		for (int i = 3; i < count; ++i) {
			ware_besch_t const* const wtyp = warenbauer_t::get_info(i);
			long ware_sum = get_ware_summe(wtyp);
			total_sum += ware_sum;
			if(ware_sum>max_ware) {
				status_bits |= ware_sum > max_ware + 32 || enables & CROWDED ? 2 : 1;
				overcrowded[wtyp->get_catg_index()/8] |= 1<<(wtyp->get_catg_index()%8);
			}
		}
	}

	// take the worst color for status
	if(  status_bits  ) {
		status_color = status_bits&2 ? COL_RED : COL_ORANGE;
	}
	else {
		status_color = (financial_history[0][HALT_WAITING]+financial_history[0][HALT_DEPARTED] == 0) ? COL_YELLOW : COL_GREEN;
	}

	financial_history[0][HALT_WAITING] = total_sum;
}



/**
 * Draws some nice colored bars giving some status information
 * @author Hj. Malthaner
 */
void haltestelle_t::display_status(sint16 xpos, sint16 ypos) const
{
	// ignore freight that cannot reach to this station
	sint16 count = 0;
	for( unsigned i=0;  i<warenbauer_t::get_waren_anzahl(); i++) {
		if(i==2) continue;	// ignore freight none
		if(gibt_ab(warenbauer_t::get_info(i))) {
			count ++;
		}
	}

	ypos -= 11;
	xpos -= (count*4 - get_tile_raster_width())/2;
	sint16 x = xpos;
	uint32 max_capacity;

	for( unsigned i=0;  i<warenbauer_t::get_waren_anzahl(); i++) {
		if(i==2) continue;	// ignore freight none
		const ware_besch_t *wtyp = warenbauer_t::get_info(i);
		if(gibt_ab(wtyp)) {
			if(i<2) {
				max_capacity = get_capacity(i);
			}
			else {
				max_capacity = get_capacity(2);
			}
			const uint32 sum = get_ware_summe(wtyp);
			uint32 v = min(sum, max_capacity);
			if(max_capacity>512) {
				v = 2+(v*128)/max_capacity;
			}
			else {
				v = (v/4)+2;
			}

			display_fillbox_wh_clip(xpos, ypos-v-1, 1, v, COL_GREY4, true);
			display_fillbox_wh_clip(xpos+1, ypos-v-1, 2, v, wtyp->get_color(), true);
			display_fillbox_wh_clip(xpos+3, ypos-v-1, 1, v, COL_GREY1, true);

			// Hajo: show up arrow for capped values
			if(sum > max_capacity) {
				display_fillbox_wh_clip(xpos+1, ypos-v-6, 2, 4, COL_WHITE, true);
				display_fillbox_wh_clip(xpos, ypos-v-5, 4, 1, COL_WHITE, true);
			}

			xpos += 4;
		}
	}

	// status color box below
	display_fillbox_wh_clip(x-1-4, ypos, count*4+12-2, 4, get_status_farbe(), true);
}



bool haltestelle_t::add_grund(grund_t *gr)
{
	assert(gr!=NULL);

	// neu halt?
	if(  tiles.is_contained(gr)  ) {
		return false;
	}

	koord pos = gr->get_pos().get_2d();
	add_to_station_type( gr );
	gr->set_halt( self );
	tiles.append( gr );

	// appends this to the ground
	// after that, the surrounding ground will know of this station
	int const cov = welt->get_settings().get_station_coverage();
	for (int y = -cov; y <= cov; y++) {
		for (int x = -cov; x <= cov; x++) {
			koord p=pos+koord(x,y);
			planquadrat_t *plan = welt->access(p);
			if(plan) {
				plan->add_to_haltlist( self );
				plan->get_kartenboden()->set_flag(grund_t::dirty);
			}
		}
	}
	welt->access(pos)->set_halt(self);

	// since suddenly other factories may be connect to us too
	verbinde_fabriken();

	// check if we have to register line(s) and/or lineless convoy(s) which serve this halt
	vector_tpl<linehandle_t> check_line(0);
	if(  get_besitzer()==welt->get_spieler(1)  ) {
		// must iterate over all players lines ...
		for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
			if(  welt->get_spieler(i)  ) {
				welt->get_spieler(i)->simlinemgmt.get_lines(simline_t::line, &check_line);
				FOR(  vector_tpl<linehandle_t>, const j, check_line  ) {
					// only add unknown lines
					if(  !registered_lines.is_contained(j)  &&  j->count_convoys() > 0  ) {
						FOR(  minivec_tpl<linieneintrag_t>, const& k, j->get_schedule()->eintrag  ) {
							if(  get_halt(welt, k.pos, j->get_besitzer()) == self  ) {
								registered_lines.append(j);
								break;
							}
						}
					}
				}
			}
		}
		// Knightly : iterate over all convoys
		FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
			// only check lineless convoys which are not yet registered
			if(  !cnv->get_line().is_bound()  &&  !registered_convoys.is_contained(cnv)  ) {
				const schedule_t *const fpl = cnv->get_schedule();
				if(  fpl  ) {
					FOR(minivec_tpl<linieneintrag_t>, const& k, fpl->eintrag) {
						if (get_halt(welt, k.pos, get_besitzer()) == self) {
							registered_convoys.append(cnv);
							break;
						}
					}
				}
			}
		}
	}
	else {
		get_besitzer()->simlinemgmt.get_lines(simline_t::line, &check_line);
		FOR( vector_tpl<linehandle_t>, const j, check_line ) {
			// only add unknown lines
			if(  !registered_lines.is_contained(j)  &&  j->count_convoys() > 0  ) {
					FOR( minivec_tpl<linieneintrag_t>, const& k, j->get_schedule()->eintrag ) {
					if(  get_halt(welt, k.pos, get_besitzer()) == self  ) {
						registered_lines.append(j);
						break;
					}
				}
			}
		}
		// Knightly : iterate over all convoys
		FOR( vector_tpl<convoihandle_t>, const cnv, welt->convoys() ) {
			// only check lineless convoys which have matching ownership and which are not yet registered
			if(  !cnv->get_line().is_bound()  &&  cnv->get_besitzer()==get_besitzer()  &&  !registered_convoys.is_contained(cnv)  ) {
				const schedule_t *const fpl = cnv->get_schedule();
				if(  fpl  ) {
					FOR(  minivec_tpl<linieneintrag_t>, const& k, fpl->eintrag  ) {
						if(  get_halt(welt, k.pos, get_besitzer()) == self  ) {
							registered_convoys.append(cnv);
							break;
						}
					}
				}
			}
		}
	}

	if(  welt->lookup(pos)->get_halt() != self  ||  !gr->is_halt()  ) {
		dbg->error( "haltestelle_t::add_grund()", "no ground added to (%s)", gr->get_pos().get_str() );
	}
	init_pos = tiles.front().grund->get_pos().get_2d();
	welt->set_schedule_counter();

	return true;
}



bool haltestelle_t::rem_grund(grund_t *gr)
{
	// namen merken
	if(!gr) {
		return false;
	}

	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i == tiles.end()) {
		// was not part of station => do nothing
		dbg->error("haltestelle_t::rem_grund()","removed illegal ground from halt");
		return false;
	}

	// first tile => remove name from this tile ...
	char buf[256];
	const char* station_name_to_transfer = NULL;
	if (i == tiles.begin() && i->grund->get_name()) {
		tstrncpy(buf, get_name(), lengthof(buf));
		station_name_to_transfer = buf;
		set_name(NULL);
	}

	// now remove tile from list
	tiles.erase(i);
	welt->set_schedule_counter();
	init_pos = tiles.empty() ? koord::invalid : tiles.front().grund->get_pos().get_2d();

	// re-add name
	if (station_name_to_transfer != NULL  &&  !tiles.empty()) {
		label_t *lb = tiles.front().grund->find<label_t>();
		if(lb) {
			delete lb;
		}
		set_name( station_name_to_transfer );
	}

	planquadrat_t *pl = welt->access( gr->get_pos().get_2d() );
	if(pl) {
		// no longer connected (upper level)
		gr->set_halt(halthandle_t());
		// still connected elsewhere?
		for(unsigned i=0;  i<pl->get_boden_count();  i++  ) {
			if(pl->get_boden_bei(i)->get_halt()==self) {
				// still connected with other ground => do not remove from plan ...
				DBG_DEBUG("haltestelle_t::rem_grund()", "keep floor, count=%i", tiles.get_count());
				return true;
			}
		}
		DBG_DEBUG("haltestelle_t::rem_grund()", "remove also floor, count=%i", tiles.get_count());
		// otherwise remove from plan ...
		pl->set_halt(halthandle_t());
		pl->get_kartenboden()->set_flag(grund_t::dirty);
	}

	int const cov = welt->get_settings().get_station_coverage();
	for (int y = -cov; y <= cov; y++) {
		for (int x = -cov; x <= cov; x++) {
			planquadrat_t *pl = welt->access( gr->get_pos().get_2d()+koord(x,y) );
			if(pl) {
				pl->remove_from_haltlist(welt,self);
				pl->get_kartenboden()->set_flag(grund_t::dirty);
			}
		}
	}

	// needs to be done, if this was a dock
	recalc_station_type();

	// factory reach may have been changed ...
	verbinde_fabriken();

	// remove lines eventually
	for(  size_t j = registered_lines.get_count();  j-- != 0;  ) {
		bool ok = false;
		FOR(  minivec_tpl<linieneintrag_t>, const& k, registered_lines[j]->get_schedule()->eintrag  ) {
			if(  get_halt(welt, k.pos, registered_lines[j]->get_besitzer()) == self  ) {
				ok = true;
				break;
			}
		}
		// need removal?
		if(!ok) {
			stale_lines.append_unique( registered_lines[j] );
			registered_lines.remove_at(j);
		}
	}

	// Knightly : remove registered lineless convoys as well
	for(  size_t j = registered_convoys.get_count();  j-- != 0;  ) {
		bool ok = false;
		FOR(  minivec_tpl<linieneintrag_t>, const& k, registered_convoys[j]->get_schedule()->eintrag  ) {
			if(  get_halt(welt, k.pos, registered_convoys[j]->get_besitzer()) == self  ) {
				ok = true;
				break;
			}
		}
		// need removal?
		if(  !ok  ) {
			stale_convois.append_unique( registered_convoys[j] );
			registered_convoys.remove_at(j);
		}
	}

	return true;
}



bool haltestelle_t::existiert_in_welt() const
{
	return !tiles.empty();
}


/* return the closest square that belongs to this halt
 * @author prissi
 */
koord haltestelle_t::get_next_pos( koord start ) const
{
	koord find = koord::invalid;

	if (!tiles.empty()) {
		// find the closest one
		int	dist = 0x7FFF;
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			koord const p = i.grund->get_pos().get_2d();
			int d = koord_distance(start, p );
			if(d<dist) {
				// ok, this one is closer
				dist = d;
				find = p;
			}
		}
	}
	return find;
}



/* marks a coverage area
 * @author prissi
 */
void haltestelle_t::mark_unmark_coverage(const bool mark) const
{
	// iterate over all tiles
	uint16 const cov = welt->get_settings().get_station_coverage();
	koord  const size(cov * 2 + 1, cov * 2 + 1);
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		welt->mark_area(i.grund->get_pos() - size / 2, size, mark);
	}
}



/* Find a tile where this type of vehicle could stop
 * @author prissi
 */
const grund_t *haltestelle_t::find_matching_position(const waytype_t w) const
{
	// iterate over all tiles
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (i.grund->hat_weg(w)) {
			return i.grund;
		}
	}
	return NULL;
}



/* checks, if there is an unoccupied loading bay for this kind of thing
 * @author prissi
 */
bool haltestelle_t::find_free_position(const waytype_t w,convoihandle_t cnv,const ding_t::typ d) const
{
	// iterate over all tiles
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (i.reservation == cnv || !i.reservation.is_bound()) {
			// not reseved
			grund_t* const gr = i.grund;
			assert(gr);
			// found a stop for this waytype but without object d ...
			if(gr->hat_weg(w)  &&  gr->suche_obj(d)==NULL) {
				// not occipied
				return true;
			}
		}
	}
	return false;
}


/* reserves a position (caution: railblocks work differently!
 * @author prissi
 */
bool haltestelle_t::reserve_position(grund_t *gr,convoihandle_t cnv)
{
	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i != tiles.end()) {
		if (i->reservation == cnv) {
//DBG_MESSAGE("haltestelle_t::reserve_position()","gr=%d,%d already reserved by cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
			return true;
		}
		// not reseved
		if (!i->reservation.is_bound()) {
			grund_t* gr = i->grund;
			if(gr) {
				// found a stop for this waytype but without object d ...
				vehikel_t const& v = *cnv->front();
				if (gr->hat_weg(v.get_waytype()) && !gr->suche_obj(v.get_typ())) {
					// not occipied
//DBG_MESSAGE("haltestelle_t::reserve_position()","sucess for gr=%i,%i cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
					i->reservation = cnv;
					return true;
				}
			}
		}
	}
//DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
	return false;
}


/* frees a reserved  position (caution: railblocks work differently!
 * @author prissi
 */
bool haltestelle_t::unreserve_position(grund_t *gr, convoihandle_t cnv)
{
	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i != tiles.end()) {
		if (i->reservation == cnv) {
			i->reservation = convoihandle_t();
			return true;
		}
	}
DBG_MESSAGE("haltestelle_t::unreserve_position()","failed for gr=%p",gr);
	return false;
}


/* can a convoi reserve this position?
 * @author prissi
 */
bool haltestelle_t::is_reservable(const grund_t *gr, convoihandle_t cnv) const
{
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (gr == i.grund) {
			if (i.reservation == cnv) {
DBG_MESSAGE("haltestelle_t::is_reservable()","gr=%d,%d already reserved by cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
				return true;
			}
			// not reseved
			if (!i.reservation.is_bound()) {
				// found a stop for this waytype but without object d ...
				vehikel_t const& v = *cnv->front();
				if (gr->hat_weg(v.get_waytype()) && !gr->suche_obj(v.get_typ())) {
					// not occipied
					return true;
				}
			}
			return false;
		}
	}
DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
	return false;
}


/* deletes factory references so map rotation won't segfault
*/
void haltestelle_t::release_factory_links()
{
	FOR(slist_tpl<fabrik_t*>, const f, fab_list) {
		f->unlink_halt(self);
	}
	fab_list.clear();
}
