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

#include "path_explorer.h"
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
#include "simsys.h"

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

#include "dings/gebaeude.h"
#include "dings/label.h"

#include "gui/halt_info.h"
#include "gui/halt_detail.h"
#include "gui/karte.h"

#include "utils/simstring.h"

#include "vehicle/simpeople.h"

#include "besch/ware_besch.h"

karte_t *haltestelle_t::welt = NULL;

slist_tpl<halthandle_t> haltestelle_t::alle_haltestellen;

stringhashtable_tpl<halthandle_t> haltestelle_t::all_names;

uint8 haltestelle_t::pedestrian_limit = 0;

static const uint8 pedestrian_generate_max = 16;

// controls the halt iterator in step_all():
static bool restart_halt_iterator = true;

void haltestelle_t::step_all()
{
	const uint32 count = alle_haltestellen.get_count();
	if (count) 
	{
		const uint32 loops = min(count, 256);
		static slist_tpl<halthandle_t>::iterator iter;
		for (uint32 i = 0; i < loops; ++i) 
		{
			if (restart_halt_iterator || iter == alle_haltestellen.end())
			{
				restart_halt_iterator = false;
				iter = alle_haltestellen.begin();
			}
			(*iter++)->step();
		}
	}
}


halthandle_t haltestelle_t::get_halt(const karte_t *welt, const koord pos, const spieler_t *sp )
{
	const planquadrat_t *plan = welt->lookup(pos);
	if(plan) 
	{
		if(plan->get_halt().is_bound()  && plan->get_halt()->check_access(sp)) 
		{
			return plan->get_halt();
		}
		// no halt? => we do the water check
		if(plan->get_kartenboden()->ist_wasser()) 
		{
			// may catch bus stops close to water ...
			const uint8 cnt = plan->get_haltlist_count();
			// first check for own stop
			for(uint8 i = 0; i < cnt; i++)  
			{
				if(plan->get_haltlist()[i]->get_besitzer() == sp) 
				{
					return plan->get_haltlist()[i];
				}
			}
			// then for other stops to which access is allowed
			// (This is second because it is preferable to dock at one's own
			// port to avoid charges)
			for(uint8 i = 0; i < cnt; i++) 
			{
				if(plan->get_haltlist()[i]->check_access(sp)) 
				{
					return plan->get_haltlist()[i];
				}
			}
			// so: nothing found
		}
	}
	return halthandle_t();
}


halthandle_t haltestelle_t::get_halt(const karte_t *welt, const koord3d pos, const spieler_t *sp)
{
	const grund_t *gr = welt->lookup(pos);
	if(gr) 
	{
		weg_t *w = gr->get_weg_nr(0);

		if(!w)
		{
			w = gr->get_weg_nr(1);
		}

		if(gr->get_halt().is_bound() && (gr->get_halt()->check_access(sp) || (w && spieler_t::check_owner(w->get_besitzer(), sp))))
		{
			return gr->get_halt();
		}
		// no halt? => we do the water check
		if(gr->ist_wasser()) 
		{
			// may catch bus stops close to water ...
			const planquadrat_t *plan = welt->lookup(pos.get_2d());
			const uint8 cnt = plan->get_haltlist_count();
			// first check for own stop
			for(uint8 i = 0; i < cnt; i++) 
			{
				if(plan->get_haltlist()[i]->get_besitzer() == sp) 
				{
					halthandle_t halt = plan->get_haltlist()[i];
					if(halt->get_station_type() & dock) 
					{
						return halt;
					}
				}
			}
			// then for other stops to which access is allowed
			// (This is second because it is preferable to dock at one's own
			// port to avoid charges)
			for(uint8 i = 0; i < cnt; i++) 
			{
				if(plan->get_haltlist()[i]->check_access(sp))  
				{
					halthandle_t halt = plan->get_haltlist()[i];
					if(halt->get_station_type() & dock) 
					{
						return halt;
					}
				}
			}
			// so: nothing found
		}
	}
	return halthandle_t();
}


koord haltestelle_t::get_basis_pos() const
{
	if (tiles.empty()) return koord::invalid;
	assert(tiles.front().grund->get_pos().get_2d() == init_pos);
	return tiles.front().grund->get_pos().get_2d();
}


koord3d haltestelle_t::get_basis_pos3d() const
{
	if (tiles.empty()) return koord3d::invalid;
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
bool haltestelle_t::remove(karte_t *welt, spieler_t *sp, koord3d pos, const char *&msg)
{
	msg = NULL;
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
		if(gb) 
		{
			DBG_MESSAGE("haltestelle_t::remove()",  "removing building" );
			if(gb->get_tile()->get_besch()->get_is_control_tower())
			{
				halt->remove_control_tower();
				halt->recalc_status();
			}
			hausbauer_t::remove( welt, sp, gb );
			bd = NULL;	// no need to recalc image
		}
	}

	if(!halt->existiert_in_welt()) {
DBG_DEBUG("haltestelle_t::remove()","remove last");
		// all deleted?
DBG_DEBUG("haltestelle_t::remove()","destroy");
		haltestelle_t::destroy( halt );
	}
	else {
DBG_DEBUG("haltestelle_t::remove()","not last");
		// acceptance and type may have been changed ... (due to post office/dock/railways station deletion)
 		halt->recalc_station_type();
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
void haltestelle_t::destroy(halthandle_t &halt)
{
	// just play safe: restart iterator at zero ...
	restart_halt_iterator = true;

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
}

haltestelle_t::haltestelle_t(karte_t* wl, loadsave_t* file)
{
	last_loading_step = wl->get_steps();

	welt = wl;

	const uint8 max_categories = warenbauer_t::get_max_catg_index();

	waren = (vector_tpl<ware_t> **)calloc( max_categories, sizeof(vector_tpl<ware_t> *) );
	non_identical_schedules = new uint8[ max_categories ];

	for ( uint8 i = 0; i < max_categories; i++ ) {
		non_identical_schedules[i] = 0;
	}
	waiting_times = new inthashtable_tpl<uint16, waiting_time_set >[max_categories];
	connexions = new quickstone_hashtable_tpl<haltestelle_t, connexion*>*[max_categories];

	// Knightly : create the actual connexion hash tables
	for(uint8 i = 0; i < max_categories; i ++)
	{
		connexions[i] = new quickstone_hashtable_tpl<haltestelle_t, connexion*>();
	}
	do_alternative_seats_calculation = true;

	status_color = COL_YELLOW;

	enables = NOT_ENABLED;

	// @author hsiegeln
	sortierung = freight_list_sorter_t::by_name;
	resort_freight_info = true;
	
	rdwr(file);

	alle_haltestellen.append(self);

	check_waiting = 0;

	// Added by : Knightly
	inauguration_time = 0;

	unload_repeat_counter = 0;
}


haltestelle_t::haltestelle_t(karte_t* wl, koord k, spieler_t* sp)
{
	self = halthandle_t(this);
	assert( !alle_haltestellen.is_contained(self) );
	alle_haltestellen.append(self);

	last_loading_step = wl->get_steps();
	welt = wl;

	this->init_pos = k;
	besitzer_p = sp;

	enables = NOT_ENABLED;

	last_catg_index = 255;	// force total reouting

	const uint8 max_categories = warenbauer_t::get_max_catg_index();

	waren = (vector_tpl<ware_t> **)calloc( max_categories, sizeof(vector_tpl<ware_t> *) );
	non_identical_schedules = new uint8[ max_categories ];

	for ( uint8 i = 0; i < max_categories; i++ ) {
		non_identical_schedules[i] = 0;
	}
	waiting_times = new inthashtable_tpl<uint16, waiting_time_set >[max_categories];
	connexions = new quickstone_hashtable_tpl<haltestelle_t, connexion*>*[max_categories];

	// Knightly : create the actual connexion hash tables
	for(uint8 i = 0; i < max_categories; i ++)
	{
		connexions[i] = new quickstone_hashtable_tpl<haltestelle_t, connexion*>();
	}
	do_alternative_seats_calculation = true;

	status_color = COL_YELLOW;

	sortierung = freight_list_sorter_t::by_name;
	init_financial_history();

	if(welt->ist_in_kartengrenzen(k)) {
		welt->access(k)->set_halt(self);
	}

	check_waiting = 0;

	check_nearby_halts();

	// Added by : Knightly
	inauguration_time = dr_time();

	unload_repeat_counter = 0;

	control_towers = 0;
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

	ITERATE(halts_within_walking_distance, n)
	{
		halthandle_t walking_distance_halt = get_halt_within_walking_distance(n);
		if ( walking_distance_halt.is_bound() )
		{
			walking_distance_halt->remove_halt_within_walking_distance(self);
		}
	}

	// clean waiting_times for each stop 
	// Inkelyad, November 2010
	FOR(slist_tpl<halthandle_t>, & current_halt, alle_haltestellen) 
	{
		for ( int category = 0; category < warenbauer_t::get_max_catg_index(); category++ )
		{
			waiting_times[category].remove(self.get_id());	
		}
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

	destroy_win((long)this);

	const uint8 max_categories = warenbauer_t::get_max_catg_index();

	for(uint8 i = 0; i < max_categories; i++) {
		if(waren[i]) {
			delete waren[i];
			waren[i] = NULL;
		}
	}
	free( waren );
	
	for(uint8 i = 0; i < max_categories; i++)
	{
		reset_connexions(i);
		delete connexions[i];
	}

	delete[] connexions;
	delete[] waiting_times;

	delete[] non_identical_schedules;

	// routes may have changed without this station ...
	verbinde_fabriken();
}


void haltestelle_t::rotate90( const sint16 y_size )
{
	init_pos.rotate90( y_size );

	// rotate waren destinations
	// iterate over all different categories
	for(uint32 i = 0; i < warenbauer_t::get_max_catg_index(); i++) 
	{
		if(waren[i]) 
		{
			vector_tpl<ware_t> * warray = waren[i];
			for(int j = warray->get_count() - 1; j >= 0; j--) 
			{
				ware_t & ware = (*warray)[j];
				if(ware.menge > 0) 
				{
					koord k = ware.get_zielpos();
					k.rotate90( y_size );
					// since we need to point at factory (0,0)
					fabrik_t *fab = fabrik_t::get_fab( welt, k );
					ware.set_zielpos( fab ? fab->get_pos().get_2d() : k );
				}
				else 
				{
					// empty => remove
					(*warray).remove_at( j );
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
				DBG_MESSAGE("haltestelle_t::set_name()","name %s already used!",gr->get_text());
			}
		}
		if(!gr->find<label_t>()) {
			gr->set_text( new_name );
			if(new_name  &&  !all_names.put(gr->get_text(),self)) {
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
					if (distance < this_distance) {
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
				const uint32 idx = simrand(street_names.get_count(), "char* haltestelle_t::create_name(koord const k, char const* const typ)");

				buf.clear();
				buf.printf( street_names[idx], city_name, stop );
				if(  !all_names.get(buf).is_bound()  ) {
					return strdup(buf);
				}
				// else remove this entry
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
					// ok, try this name, if free ...
					buf.printf( base_name, city_name, dirname, stop );
				}
				else {
					// ok, try this name, if free ...
					buf.printf( base_name, city_name, stop );
				}
				if(  !all_names.get(buf).is_bound()  ) {
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
	if(  !loading_here.is_contained(cnv)  ) 
	{
		loading_here.append(cnv);
	}
	if(  last_loading_step != welt->get_steps()  ) 
	{
		last_loading_step = welt->get_steps();

		// now iterate over all convois
		for(  slist_tpl<convoihandle_t>::iterator i = loading_here.begin(), end = loading_here.end();  i != end;  ) 
		{
			if(  (*i).is_bound()  &&  ((*i)->get_state()==convoi_t::LOADING || (*i)->get_state()==convoi_t::REVERSING) ) 
			{
				// now we load into convoi
				(*i)->hat_gehalten( self );
				++i;
			}
			else 
			{
				i = loading_here.erase( i );
			}
		}
	}
}



void haltestelle_t::step()
{
#ifdef DEBUG_SIMRAND_CALLS
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station");
#endif
	// Knightly : update status
	//   There is no idle state in Experimental
	//   as rerouting request may be sent via
	//   refresh_routing() instead of
	//   karte_t::set_schedule_counter()

	recalc_status();
	
	// Every 256 steps - check whether
	// passengers/goods have been waiting
	// too long.
	// Will overflow at 255.
	if(++check_waiting == 0)
	{
		vector_tpl<ware_t> *warray;
		for(uint16 j = 0; j < warenbauer_t::get_max_catg_index(); j ++)
		{
			warray = waren[j];
			if(warray == NULL)
			{
				continue;
			}
			for(uint32 i = 0; i < warray->get_count(); i++) 
			{
				ware_t &tmp = (*warray)[i];

				// skip empty entries
				if(tmp.menge == 0) 
				{
					continue;
				}
						
				uint16 waiting_minutes = convoi_t::get_waiting_minutes(welt->get_zeit_ms() - tmp.arrival_time);

				// Checks to see whether the freight has been waiting too long.
				// If so, discard it.
				if(tmp.get_besch()->get_speed_bonus() > 0)
				{
					// Only consider for discarding if the goods care about their timings.
					// Goods/passengers' maximum waiting times are proportionate to the length of the journey.
					const uint16 base_max_minutes = (welt->get_settings().get_passenger_max_wait() / tmp.get_besch()->get_speed_bonus()) * 10;  // Minutes are recorded in tenths
					halthandle_t h = haltestelle_t::get_halt(welt, tmp.get_zielpos(), besitzer_p);
					uint16 journey_time = 65535;
					path_explorer_t::get_catg_path_between(tmp.get_besch()->get_catg_index(), tmp.get_origin(), tmp.get_ziel(), journey_time, h);
					const uint16 thrice_journey = journey_time * 3;
					const uint16 min_minutes = base_max_minutes / 12;
					const uint16 max_minutes = base_max_minutes < thrice_journey ? base_max_minutes : max(thrice_journey, min_minutes);
#ifdef DEBUG_SIMRAND_CALLS
					if (talk && i == 2198)
						dbg->message("haltestelle_t::step", "%u) check %u of %u minutes: %u %s to \"%s\"", 
						i, waiting_minutes, max_minutes, tmp.menge, tmp.get_besch()->get_name(), tmp.get_ziel()->get_name());
#endif
					if(waiting_minutes > max_minutes)
					{
#ifdef DEBUG_SIMRAND_CALLS
						if (talk)
							dbg->message("haltestelle_t::step", "%u) discard after %u of %u minutes: %u %s to \"%s\"", 
							i, waiting_minutes, max_minutes, tmp.menge, tmp.get_besch()->get_name(), tmp.get_ziel()->get_name());
#endif

						// Waiting too long: discard
						if(tmp.is_passenger())
						{
							// Passengers - use unhappy graph.
							add_pax_unhappy(tmp.menge);
						}

						// Experimental 7.2 - if they are discarded, a refund is due.

						if(tmp.get_origin().is_bound() && get_besitzer()->get_player_cash_int() > 0)
						{
							// Cannot refund unless we know the origin.
							// Also, ought not refund unless the player is solvent. 
							// Players ought not be put out of business by refunds, as this makes gameplay too unpredictable, 
							// especially in online games, where joining one player's network to another might lead to a large
							// influx of passengers which one of the networks cannot cope with.
							const uint16 distance = shortest_distance(get_basis_pos(), tmp.get_origin()->get_basis_pos());
							if(distance > 0) // No point in calculating refund if passengers/goods are discarded from their origin stop.
							{
								// Refund is approximation: 2x distance at standard rate with no adjustments. 
								const sint64 refund_amount = ((tmp.menge * tmp.get_fare(distance) * 2000ll) + 1500ll) / 3000ll;
								
								besitzer_p->buche(-refund_amount, get_basis_pos(), COST_INCOME);
								linehandle_t account_line = get_preferred_line(tmp.get_zwischenziel(), tmp.get_catg());
								if(account_line.is_bound())
								{
									account_line->book(-refund_amount, LINE_PROFIT);
									account_line->book(-refund_amount, LINE_REFUNDS);
									get_besitzer()->buche(-refund_amount, COST_VEHICLE_RUN);
								}
								else
								{
									convoihandle_t account_convoy = get_preferred_convoy(tmp.get_zwischenziel(), tmp.get_catg());
									if(account_convoy.is_bound())
									{
										account_convoy->book(-refund_amount, convoi_t::CONVOI_PROFIT);
										account_convoy->book(-refund_amount, convoi_t::CONVOI_REFUNDS);
										get_besitzer()->buche(-refund_amount, COST_VEHICLE_RUN);
									}
								}
							}
						}
						
						// If goods/passengers leave, then they must register a waiting time, or else
						// overcrowded stops would have excessively low waiting times. Because they leave
						// before they have got transport, the waiting time registered must be increased
						// by 4x to reflect an estimate of how long that they would likely have had to
						// have waited to get transport.

						waiting_minutes *= 4;
						add_waiting_time(waiting_minutes, tmp.get_zwischenziel(), tmp.get_besch()->get_catg_index());
						
						// The goods/passengers leave.
						tmp.menge = 0;
					}		
				}
				
				if(waiting_minutes > 2 * get_average_waiting_time(tmp.get_zwischenziel(), tmp.get_besch()->get_catg_index()))
				{
					// Check to see whether these passengers/this freight has been waiting more than 2x as long
					// as the existing registered waiting times. If so, register the waiting time to prevent an
					// artificially low time from being recorded if there is a long service interval. 
					add_waiting_time(waiting_minutes, tmp.get_zwischenziel(), tmp.get_besch()->get_catg_index());
				}
			}
		}
	}
}



/**
 * Called every month
 * @author Hj. Malthaner
 */
void haltestelle_t::neuer_monat()
{
	if(  welt->get_active_player()==besitzer_p  &&  status_color==COL_RED  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("!0_STATION_CROWDED"), get_name() );
		welt->get_message()->add_message(buf, get_basis_pos(),message_t::full, PLAYER_FLAG|besitzer_p->get_player_nr(), IMG_LEER );
		enables &= (PAX|POST|WARE);
	}

	// If the waiting times have not been updated for too long, gradually re-set them; also increment the timing records.
	for ( int category = 0; category < warenbauer_t::get_max_catg_index(); category++ )
	{
		FOR(waiting_time_map, & iter, waiting_times[category])
		{
			// If the waiting time data are stale (more than two months old), gradually flush them.
			// After two months, values of 10 minutes are appended to the list of waiting times.
			// This helps to gradually reduce times which were high as a result of a one-off problem,
			// whilst still allowing rarely-travelled connections to have sensible waiting times.
			if(iter.value.month > 2)
			{
				for(int i = 0; i < 8; i ++)
				{
					iter.value.times.add_to_tail(19);
				}
				iter.value.times.clear();
				iter.value.month = 0;
			}
			// Update the waiting time timing records.
			iter.value.month ++;
		}
	}

	check_nearby_halts();

	// hsiegeln: roll financial history
	for (int j = 0; j<MAX_HALT_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
}


// Added by		: Knightly
// Adapted from : reroute_goods()
// Purpose		: re-route goods of a single ware category
uint32 haltestelle_t::reroute_goods(const uint8 catg)
{
	if(waren[catg])
	{
		vector_tpl<ware_t> * warray = waren[catg];
		const uint32 packet_count = warray->get_count();
		vector_tpl<ware_t> * new_warray = new vector_tpl<ware_t>(packet_count);

#ifdef DEBUG_SIMRAND_CALLS
		bool talk = catg == 0 && !strcmp(get_name(), "Newton Abbot Railway Station");

		if (talk)
			dbg->message("haltestelle_t::reroute_goods", "halt \"%s\", old packet count %u ", get_name(), packet_count);
#endif

		// Hajo:
		// Step 1: re-route goods now and then to adapt to changes in
		// world layout, remove all goods which destination was removed from the map
		// prissi;
		// also the empty entries of the array are cleared
		for(int j = packet_count - 1; j  >= 0; j--) 
		{
			ware_t & ware = (*warray)[j];

			if(ware.menge == 0) 
			{
				continue;
			}

			// since also the factory halt list is added to the ground, we can use just this ...
			const planquadrat_t* plan = welt->lookup(ware.get_zielpos());
			if(plan && plan->is_connected(self)) 
			{
				// we are already there!
				if(  ware.to_factory  )
				{
					liefere_an_fabrik(ware);
				}
				continue;
			}

			// check if this good can still reach its destination
			
			if(find_route(ware) == 65535)
			{
				// remove invalid destinations
				continue;
			}

			// If the passengers have re-routed so that they now
			// walk to the next transfer, go there immediately.
			if(ware.is_passenger() && is_within_walking_distance_of(ware.get_zwischenziel()) && !connexions[0]->get(ware.get_zwischenziel())->best_convoy.is_bound() && !connexions[0]->get(ware.get_zwischenziel())->best_line.is_bound())
			{
				erzeuge_fussgaenger(welt, get_basis_pos3d(), ware.menge);
				ware.get_zwischenziel()->liefere_an(ware);
				continue;
			}

			// add to new array
			new_warray->append( ware );
		}	

#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
			dbg->message("haltestelle_t::reroute_goods", "halt \"%s\", new packet count %u ", get_name(), new_warray->get_count());
#endif

		// delete, if nothing connects here
		if (new_warray->empty()) 
		{
			if(connexions[catg]->empty())
			{
				// no connections from here => delete
				delete new_warray;
				new_warray = NULL;
			}
		}

		// replace the array
		delete waren[catg];
		waren[catg] = new_warray;

		// likely the display must be updated after this
		resort_freight_info = true;

		return packet_count;
	}
	else
	{
		return 0;
	}
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
				fab_list.insert(fab);
				fab->link_halt(self);
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

uint16 haltestelle_t::get_average_waiting_time(halthandle_t halt, uint8 category) const
{
	inthashtable_tpl<uint16, haltestelle_t::waiting_time_set> * const wt = &waiting_times[category];
	if(wt->is_contained((halt.get_id())))
	{
		fixed_list_tpl<uint16, 32> times = waiting_times[category].get(halt.get_id()).times;
		const uint16 count = times.get_count();
		if(count > 0 && halt.is_bound())
		{
			uint32 total_times = 0;
			ITERATE(times,i)
			{
				total_times += times.get_element(i);
			}
			total_times /= count;
			// Minimum waiting time of 2 minutes (i.e., 20 tenths of a minute)
			// This simulates the overhead time needed to arrive at a stop, and 
			// board, etc. 
			return total_times >= 20 ? (uint16)total_times : 20;
		}
		return 19;
	}
	return 19;
}

linehandle_t haltestelle_t::get_preferred_line(halthandle_t transfer, uint8 category) const
{
	if(connexions[category]->empty() || connexions[category]->get(transfer) == NULL)
	{
		linehandle_t dummy;
		return dummy;
	}
	linehandle_t best_line = connexions[category]->get(transfer)->best_line;
	return best_line;
}

convoihandle_t haltestelle_t::get_preferred_convoy(halthandle_t transfer, uint8 category) const
{
	if(connexions[category]->empty() || connexions[category]->get(transfer) == NULL)
	{
		convoihandle_t dummy;
		return dummy;
	}
	convoihandle_t best_convoy = connexions[category]->get(transfer)->best_convoy;
	return best_convoy;
}

void haltestelle_t::reset_connexions(uint8 category)
{
	if(connexions[category]->empty())
	{
		// Nothing to do here
		return;
	}

	FOR(connexions_map, & iter, *connexions[category] ) 
	{
        delete iter.value;
	}
}

// Added by		: Knightly
// Adapted from : Jamespetts' code
// Purpose		: To notify relevant halts to rebuild connexions
// @jamespetts: modified the code to combine with previous method and provide options about partially delayed refreshes for performance.
void haltestelle_t::refresh_routing(const schedule_t *const sched, const minivec_tpl<uint8> &categories, const spieler_t *const player)
{
	halthandle_t tmp_halt;

	if(sched && player)
	{
		const uint8 catg_count = categories.get_count();

		for (uint8 i = 0; i < catg_count; i++)
		{
			path_explorer_t::refresh_category(categories[i]);
		}
	}
	else
	{
		dbg->error("convoi_t::refresh_routing()", "Schedule or player is NULL");
	}
}

minivec_tpl<halthandle_t>* haltestelle_t::build_destination_list(ware_t &ware)
{
	const ware_besch_t * warentyp = ware.get_besch();

	if(ware.get_zielpos() == koord::invalid && ware.get_ziel().is_bound())
	{
		ware.set_zielpos(ware.get_ziel()->get_basis_pos());
	}
		
	const koord ziel = ware.get_zielpos();

	// since also the factory halt list is added to the ground, we can use just this ...
	const planquadrat_t *const plan = welt->lookup( ware.get_zielpos() );
	const halthandle_t *const halt_list = plan->get_haltlist();
	// but we can only use a subset of these
	minivec_tpl<halthandle_t> *ziel_list = new minivec_tpl<halthandle_t>(plan->get_haltlist_count());

	for(uint16 h = 0; h < plan->get_haltlist_count(); h++) 
	{
		halthandle_t halt = halt_list[h];
		if(halt->is_enabled(warentyp)) 
		{
			ziel_list->append(halt);
		}
	}
	return ziel_list;
}

uint16 haltestelle_t::find_route(minivec_tpl<halthandle_t> *ziel_list, ware_t &ware, const uint16 previous_journey_time)
{
	uint16 journey_time = previous_journey_time;

	if(ziel_list->empty()) 
	{
		//no target station found
		ware.set_ziel(halthandle_t());
		ware.set_zwischenziel(halthandle_t());
		return 65535;
	}

	// check, if the shortest connection is not right to us ...
	if(ziel_list->is_contained(self)) 
	{
		ware.set_ziel(self);
		ware.set_zwischenziel( halthandle_t());
		return 65535;
	}


	// Now, find the best route from here.
	// Added by		: Knightly
	// Adapted from : James' code

	uint16 test_time;
	halthandle_t test_transfer;

	halthandle_t best_destination;
	halthandle_t best_transfer;

	const uint8 ware_catg = ware.get_besch()->get_catg_index();

	for (uint8 i = 0; i < ziel_list->get_count(); i++)
	{
		path_explorer_t::get_catg_path_between(ware_catg, self, (*ziel_list)[i], test_time, test_transfer);

		if( test_time < journey_time )
		{
			best_destination = (*ziel_list)[i];
			journey_time = test_time;
			best_transfer = test_transfer;
		}
	}
		
	if( journey_time < previous_journey_time )
	{
		ware.set_ziel(best_destination);
		ware.set_zwischenziel(best_transfer);
		return journey_time;
	}
	
	return journey_time;
}

uint16 haltestelle_t::find_route (ware_t &ware, const uint16 previous_journey_time)
{
	minivec_tpl<halthandle_t> *ziel_list = build_destination_list(ware);
	const uint16 journey_time = find_route(ziel_list, ware, previous_journey_time);
	delete ziel_list;
	return journey_time;
}

/**
 * Found route and station uncrowded
 * @author Hj. Malthaner
 * As of Simutrans-Experimental 7.2, 
 * this method is called instead when
 * passengers *arrive* at their destination.
 */
void haltestelle_t::add_pax_happy(int n)
{
	book(n, HALT_HAPPY);
	recalc_status();
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

// Found a route, but too slow.
// @author: jamespetts

void haltestelle_t::add_pax_too_slow(int n)
{
	book(n, HALT_TOO_SLOW);
}

/**
 * Found no route
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_no_route(int n)
{
	book(n, HALT_NOROUTE);
}


void haltestelle_t::liefere_an_fabrik(const ware_t& ware) const //"deliver to the factory" (Google)
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
			resort_freight_info = true;
			return true;
		}
	}
	// nothing to take out
	return false;
}


// will load something compatible with wtyp into the car which schedule is fpl
ware_t haltestelle_t::hole_ab(const ware_besch_t *wtyp, uint32 maxi, const schedule_t *fpl, const spieler_t *sp, convoi_t* cnv, bool overcrowded) //"hole from" (Google)
{
	// prissi: first iterate over the next stop, then over the ware
	// might be a little slower, but ensures that passengers to nearest stop are served first
	// this allows for separate high speed and normal service
	const uint8 count = fpl->get_count();
	vector_tpl<ware_t> *warray = waren[wtyp->get_catg_index()];

	if(warray != NULL) 
	{
#ifdef DEBUG_SIMRAND_CALLS_BG
		//if (!strcmp(get_name(), "Newton Abbot Railway Station"))
		//{
		//	dbg->message("haltestelle_t::hole_ab", "halt \"%s\", ware \"%s\": max %u", get_name(), wtyp->get_name(), warray->get_count());
		//	for (int i = 0; i < warray->get_count(); ++i)
		//	{
		//		char buf[16];
		//		ware_t &ware = (*warray)[i];
		//		sprintf(buf, "% 8u)", i);
		//		dbg->message(buf, "%u to \"%s\"", ware.menge, ware.get_ziel()->get_name());
		//	}
		//}
#endif
		uint32 accumulated_journey_time = 0;
		halthandle_t previous_halt = self;

		// uses fpl->increment_index to iterate over stops
		uint8 index = fpl->get_aktuell();
		bool reverse = cnv->get_reverse_schedule();
		fpl->increment_index(&index, &reverse);

		while (index != fpl->get_aktuell()) 
		{
			const halthandle_t plan_halt = haltestelle_t::get_halt(welt, fpl->eintrag[index].pos, sp);
			if(plan_halt == self) 
			{
				// we will come later here again ...
				break;
			}
			else if(plan_halt.is_bound() && warray->get_count() > 0) 
			{
				// Calculate the journey time for *this* convoy from here (if not already calculated)
				uint16 journey_time = 0;

				journey_time = cnv->get_average_journey_times_this_convoy_only()->get(id_pair(plan_halt.get_id(), previous_halt.get_id())).get_average();

				if(journey_time == 0)
				{
					sint32 average_speed = cnv->get_finance_history(1, convoi_t::CONVOI_AVERAGE_SPEED) > 0 ? cnv->get_finance_history(1, convoi_t::CONVOI_AVERAGE_SPEED) * 100 : cnv->get_finance_history(0, convoi_t::CONVOI_AVERAGE_SPEED) * 100;
					if(average_speed == 0)
					{
						// If the average speed is not initialised, take a guess to prevent perverse outcomes and possible deadlocks.
						average_speed = speed_to_kmh(cnv->get_min_top_speed()) * 50;
						average_speed = average_speed == 0 ? 1 : average_speed;
					}
						
					accumulated_journey_time += ((shortest_distance(plan_halt->get_basis_pos(), previous_halt->get_basis_pos()) 
													/ average_speed) *welt->get_settings().get_meters_per_tile() * 60);
				}		
								
				// The random offset will ensure that all goods have an equal chance to be loaded.
#ifdef DEBUG_SIMRAND_CALLS
				char buf[512];
				sprintf(buf, "haltestelle_t::hole_ab halt \"%s\", ware \"%s\"", this->get_name(), wtyp->get_name());
				uint32 offset = simrand(warray->get_count(), buf);
#else
				uint32 offset = simrand(warray->get_count(), "haltestelle_t::hole_ab");
#endif

				halthandle_t next_transfer;
				uint8 catg_index;

				for(uint32 i = 0;  i < warray->get_count();  i++) 
				{
					ware_t &tmp = (*warray)[ i+offset ];
					next_transfer = tmp.get_zwischenziel();
					catg_index = tmp.get_besch()->get_catg_index();

					// prevent overflow (faster than division)
					if(i + offset + 1 >= warray->get_count()) 
					{
						offset -= warray->get_count();
					}

					// skip empty entries
					if(tmp.menge == 0) 
					{
						continue;
					}					

					// compatible car and right target stop?
					if(next_transfer == plan_halt) 
					{
						const uint16 speed_bonus = tmp.get_besch()->get_speed_bonus();
						uint16 waiting_minutes = convoi_t::get_waiting_minutes(welt->get_zeit_ms() - tmp.arrival_time);
						// For goods that care about their speed, optimise loading for maximum speed of the journey.
						if(speed_bonus > 0)
						{												
							// Try to ascertain whether it would be quicker to board this convoy, or wait for a faster one.
							
							bool is_preferred = true;
							if(cnv->get_line().is_bound())
							{
								const linehandle_t best_line = get_preferred_line(next_transfer, catg_index);
								if(best_line.is_bound() && best_line != cnv->get_line())
								{
									is_preferred = false;
								}
							}
							else
							{
								const convoihandle_t best_convoy = get_preferred_convoy(next_transfer, catg_index);
								if(best_convoy.is_bound() && best_convoy.get_rep() != cnv)
								{
									is_preferred = false;
								}
							}

							// Thanks to cwlau9 for suggesting this formula (now heavily modified by Knightly and jamespetts)
							// July 2009
							if(!is_preferred)
							{
								const connexion* next_connexion = connexions[catg_index]->get(next_transfer);
								const uint16 average_waiting_minutes = next_connexion != NULL ? next_connexion->waiting_time : 15;
								const uint16 base_max_minutes = (welt->get_settings().get_passenger_max_wait() / speed_bonus) * 5;  
								const uint16 preferred_travelling_minutes = next_connexion != NULL ? next_connexion->journey_time : 15;
								// Minutes are recorded in tenths. One third max for this purpose.
								const uint16 max_minutes = base_max_minutes > preferred_travelling_minutes ? preferred_travelling_minutes : base_max_minutes;
								const sint16 preferred_advantage_minutes = accumulated_journey_time - preferred_travelling_minutes;

								// New formula: Carl Baker, Feb 2012
								
								bool much_faster = true;
								bool waiting_too_long = false;
								int how_much_slower = journey_time * 100;
								if (preferred_travelling_minutes > 1)
								{
									how_much_slower /= preferred_travelling_minutes;
								}
								 // Passengers will always board slower convoy if its journey time is within an acceptable
								 // tolerance of the fastest journey time. 
								 // The acceptable tolerance is scaled depending on journey time of faster convoy.
								 if (preferred_travelling_minutes <= 100 && how_much_slower < 160)
								 {
								  much_faster = false;
								 }
								 else if ((preferred_travelling_minutes > 100 && preferred_travelling_minutes <= 300) && (how_much_slower < 150))
								 {
									 much_faster = false;
								 }
								 else if ((preferred_travelling_minutes > 300 && preferred_travelling_minutes <= 600) && (how_much_slower < 140))
								 {
									 much_faster = false;
								 }
								 else if ((preferred_travelling_minutes > 600 && preferred_travelling_minutes <= 900) && (how_much_slower < 133))
								 {
									 much_faster = false;
								 }
								 else if ((preferred_travelling_minutes > 900 && preferred_travelling_minutes <= 1200) && (how_much_slower < 125))
								 {
									 much_faster = false;
								 }
								 else if ((preferred_travelling_minutes > 1200 && preferred_travelling_minutes <= 1800) && (how_much_slower < 122))
								 {
									 much_faster = false;
								 }
								 else if ((preferred_travelling_minutes > 1800) && (how_much_slower < 118))
								 {
									 much_faster = false;
								 } 
                     
								 // If passengers have been waiting a long time, they are more likely to board a slower convoy.
								 // But this is scaled so that a much slower convoy requires a much longer-than-expected wait.
								 if (much_faster == true)
								 {
									if ((how_much_slower <= 125) && ( waiting_minutes >= (average_waiting_minutes * 3)))
									{
										waiting_too_long = true;
									}
									else if ((how_much_slower > 125 && how_much_slower <= 150) && ( waiting_minutes >= (average_waiting_minutes * 4)))
									{
										waiting_too_long = true;
									}
									else if ((how_much_slower > 150 && how_much_slower <= 200) && ( waiting_minutes >= (average_waiting_minutes * 5)))
									{
									 waiting_too_long = true;
									}
									else if (how_much_slower > 200 && ( waiting_minutes >= (average_waiting_minutes * 6)))
									{
									 waiting_too_long = true;
									}
									else 
									{
										waiting_too_long = false;
									}
								 }
                     
								 // Passengers continue to wait for faster convoy if...
								 if ((much_faster == true) && (waiting_too_long == false))
								 {
									 continue;
								 } 
							}
						}

						const uint32 time_till_departure = (cnv->go_on_ticks - welt->get_zeit_ms());
						// Don't board a vehicle which is waiting for spacing until near its departure time
						// Assures that passengers will board the first train to leave, not the first train to arrive
						if ((fpl->get_current_eintrag().ladegrad > 0) && (fpl->get_spacing() > 0))
						{
							if ((welt->ticks_to_tenths_of_minutes(time_till_departure)) > 100)
							{
								continue;
							}
						}
	
						// Refuse to be overcrowded if alternative exists
						connexion * const next_connexion = connexions[catg_index]->get(next_transfer);
						if (next_connexion &&  overcrowded && next_connexion->alternative_seats )
						{
							continue;
						}

						// not too much?
						ware_t neu(tmp);
						if(  tmp.menge > maxi  ) 
						{
							// not all can be loaded
							neu.menge = maxi;
							tmp.menge -= maxi;
						}
						else 
						{
							// leave an empty entry => joining will more often work
							tmp.menge = 0;
						}
				
						book(neu.menge, HALT_DEPARTED);

						resort_freight_info = true;

						return neu;
					}
				}			
				// nothing there to load
			}

			// if the schedule is mirrored and has reached its end, break
			// as the convoi will be returning this way later.
			if( fpl->is_mirrored() && (index==0 || index==(count-1)) )
			{
				break;
			}

			fpl->increment_index(&index, &reverse);
		}
	}
	// empty quantity of required type -> no effect
	return ware_t (wtyp);
}

/** 
 * It will calculate number of free seats in all other (not cnv) convoys at stop
 * @author Inkelyad, adapted from hole_ab
 */
void haltestelle_t::update_alternative_seats(convoihandle_t cnv)
{
	if ( loading_here.get_count() > 1) {
		do_alternative_seats_calculation = true;
	}

	if (!do_alternative_seats_calculation )
	{
		return;
	}

	int catg_index =  warenbauer_t::passagiere->get_catg_index();
	FOR(connexions_map, const& iter, *(connexions[catg_index]))
	{
		iter.value->alternative_seats = 0;
	}

	if (loading_here.get_count() < 2 ) { // Alternatives don't exist, only one convoy here
		do_alternative_seats_calculation = false; // so we will not do clean-up again
		return;
	}

	for (slist_tpl<convoihandle_t>::iterator cnv_i = loading_here.begin(), end = loading_here.end();  cnv_i != end;  ++cnv_i)
	{
		if (!(*cnv_i).is_bound() || (*cnv_i) == cnv || ! (*cnv_i)->get_free_seats() )
		{
			continue;
		}
		const schedule_t *fpl = (*cnv_i)->get_schedule();
		const spieler_t *sp = (*cnv_i)->get_besitzer();
		const uint8 count = fpl->get_count();

		// uses fpl->increment_index to iterate over stops
		uint8 index = fpl->get_aktuell();
		bool reverse = cnv->get_reverse_schedule();
		fpl->increment_index(&index, &reverse);

		while (index != fpl->get_aktuell()) {
			const halthandle_t plan_halt = haltestelle_t::get_halt(welt, fpl->eintrag[index].pos, sp);
			if(plan_halt == self) 
			{
				// we will come later here again ...
				break;
			}
			if(plan_halt.is_bound() && plan_halt->get_pax_enabled()) 
			{
				connexion * const next_connexion = connexions[catg_index]->get(plan_halt);
				if (next_connexion) {
					next_connexion->alternative_seats += (*cnv_i)->get_free_seats();
				}			
			}

			// if the schedule is mirrored and has reached its end, break
			// as the convoi will be returning this way later.
			if( fpl->is_mirrored() && (index==0 || index==(count-1)) ) {
				break;
			}
			fpl->increment_index(&index, &reverse);
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


bool haltestelle_t::vereinige_waren(const ware_t &ware) //"unite were" (Google)
{
	// pruefen ob die ware mit bereits wartender ware vereinigt werden kann
	// "examine whether the ware with software already waiting to be united" (Google)

	vector_tpl<ware_t> * warray = waren[ware.get_besch()->get_catg_index()];
	if(warray != NULL) 
	{
		FOR(vector_tpl<ware_t>, & tmp, *warray) 
		{

			/*
			* OLD SYSTEM - did not take account of origins and timings when merging.
			*
			* // es wird auf basis von Haltestellen vereinigt
			* // prissi: das ist aber ein Fehler für alle anderen Güter, daher Zielkoordinaten für alles, was kein passagier ist ...
			*
			* //it is based on uniting stops. 
			* //prissi: but that is a mistake for all other goods, therefore, target coordinates for everything that is not a passenger ...
			* // (Google)
			*
			* if(ware.same_destination(tmp)) {
			*/

			// NEW SYSTEM
			// Adds more checks.
			// @author: jamespetts
			if(ware.can_merge_with(tmp))
			{
				if(  ware.get_zwischenziel().is_bound()  &&  ware.get_zwischenziel()!=self  ) 
				{
					// update route if there is newer route
					tmp.set_zwischenziel( ware.get_zwischenziel() );
				}

				// Merge waiting times.
				if(ware.menge > 0)
				{
					//The waiting time for ware will always be zero.
					tmp.arrival_time = welt->get_zeit_ms() - ((welt->get_zeit_ms() - tmp.arrival_time) * tmp.menge) / (tmp.menge + ware.menge);
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
void haltestelle_t::add_ware_to_halt(ware_t ware, bool from_saved)
{
#ifdef DEBUG_SIMRAND_CALLS
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station");
#endif
	//@author: jamespetts
	if(!from_saved)
	{
		ware.arrival_time = welt->get_zeit_ms();
	}	
	
	ware.set_last_transfer(self);

	// now we have to add the ware to the stop
	vector_tpl<ware_t> * warray = waren[ware.get_besch()->get_catg_index()];
	if(warray==NULL) 
	{
		// this type was not stored here before ...
		warray = new vector_tpl<ware_t>(4);
		waren[ware.get_besch()->get_catg_index()] = warray;
	}
	// the ware will be put into the first entry with menge==0
	resort_freight_info = true;
#ifdef DEBUG_SIMRAND_CALLS
	int n = 0;
#endif
	FOR(vector_tpl<ware_t>, & i, *warray) {
#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
		{
			if (!loading && n > 2900)
			{
				char buf[16];
				sprintf(buf, "% 8u)", n);
				dbg->message(buf, "%u to %s", i.menge, i.get_ziel()->get_name());
			}
			++n;
		}
#endif
		if (i.menge == 0) {
#ifdef DEBUG_SIMRAND_CALLS
			if (talk && warray->get_count() >= 2923)
			{
				dbg->message("haltestelle_t::add_ware_to_halt", "*halt \"%s\", ware \"%s\": packets %u", get_name(), ware.get_besch()->get_name(), warray->get_count());
				if (warray->get_count() == 2923)
				{
					int x = 0;
				}
			}
#endif
			i = ware;
			return;
		}
	}
	// here, if no free entries found
#ifdef DEBUG_SIMRAND_CALLS
	if (talk)
	{
		int n = warray->get_count();
		if (n >= 2923)
		{
			dbg->message("haltestelle_t::add_ware_to_halt", "halt \"%s\", ware \"%s\": packets %u", get_name(), ware.get_besch()->get_name(), warray->get_count());
			if (n == 2923)
			{
				int x = 0;
			}
		}
		else if (n >= 2190 && n <= 2200)
		{
			char buf[16];
			sprintf(buf, "% 8u)", n);
			dbg->message(buf, "%u", ware.menge);
		}
	}
#endif
	warray->append(ware);
}



/* same as liefere an, but there will be no route calculated,
 * since it hase be calculated just before
 * (execption: route contains us as intermediate stop)
 * @author prissi
 */
uint32 haltestelle_t::starte_mit_route(ware_t ware)
{
#ifdef DEBUG_SIMRAND_CALLS
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station");

	if (talk)
		dbg->message("haltestelle_t::starte_mit_route", "halt \"%s\", ware \"%s\": menge %u", get_name(), ware.get_besch()->get_name(), ware.menge);
#endif

	if(ware.get_ziel()==self) {
		if(  ware.to_factory  ) {
			// muss an fabrik geliefert werden
			liefere_an_fabrik(ware);
		}
		// already there: finished (may be happen with overlapping areas and returning passengers)
#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
			dbg->message("\t", "already finished");
#endif
		return ware.menge;
	}

	// no valid next stops? Or we are the next stop?
	if(ware.get_zwischenziel() == self) 
	{
		dbg->error("haltestelle_t::starte_mit_route()","route cannot contain us as first transfer stop => recalc route!");
		if(find_route(ware) == 65535)
		{
			// no route found?
			dbg->error("haltestelle_t::starte_mit_route()","no route found!");
			return ware.menge;
		}
	}

	// passt das zu bereits wartender ware ?
	if(vereinige_waren(ware)) {
		// dann sind wir schon fertig;
#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
			dbg->message("\t", "united with existing ware.");
#endif
		return ware.menge;
	}

	if(ware.is_passenger() && unload_repeat_counter < 3 && is_within_walking_distance_of(ware.get_zwischenziel()) && !connexions[0]->get(ware.get_zwischenziel())->best_convoy.is_bound() && !connexions[0]->get(ware.get_zwischenziel())->best_line.is_bound())
	{
		// If this is within walking distance of the next transfer, and there is not a faster way there, walk there.
		erzeuge_fussgaenger(welt, get_basis_pos3d(), ware.menge);
		unload_repeat_counter ++;
#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
			dbg->message("\t", "walking to %s", ware.get_zwischenziel()->get_name());
#endif
		return ware.get_zwischenziel()->liefere_an(ware);
	}
	else
	{
		// add to internal storage
		add_ware_to_halt(ware);
#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
		{
			const vector_tpl<ware_t> * warray = waren[ware.get_besch()->get_catg_index()];
			dbg->message("\t", "warray count %d", (*warray).get_count());
		}
#endif
		return ware.menge;
	}
}



/* Recieves ware and tries to route it further on
 * if no route is found, it will be removed
 * @author prissi
 */
uint32 haltestelle_t::liefere_an(ware_t ware)
{
#ifdef DEBUG_SIMRAND_CALLS
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station");
#endif

	// no valid next stops?
	if(!ware.get_ziel().is_bound()  ||  !ware.get_zwischenziel().is_bound()) 
	{
		// write a log entry and discard the goods
dbg->warning("haltestelle_t::liefere_an()","%d %s delivered to %s have no longer a route to their destination!", ware.menge, translator::translate(ware.get_name()), get_name() );
		return ware.menge;
	}

	// have we arrived?
	const planquadrat_t* plan = welt->lookup(ware.get_zielpos());
	if(plan && plan->is_connected(self)) 
	{
		if(ware.to_factory) 
		{
			// muss an fabrik geliefert werden
			liefere_an_fabrik(ware);
		}
		else if(ware.get_besch()==warenbauer_t::passagiere) 
		{
			// arriving passenger may create pedestrians
			if(welt->get_settings().get_show_pax()) {
				int menge = ware.menge;
				FOR(slist_tpl<tile_t>, const& i, tiles) {
					if (menge <= 0) break;
					menge = erzeuge_fussgaenger(welt, i.grund->get_pos(), menge);
				}
				INT_CHECK("simhalt 938");
			}
		}
#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
			dbg->message("haltestelle_t::liefere_an", "%d arrived at station \"%s\" waren[0].count %d", ware.menge, get_name(), get_warray(0)->get_count());
#endif
		return ware.menge;
	}

	// do we have already something going in this direction here?
	if(  vereinige_waren(ware)  ) 
	{
#ifdef DEBUG_SIMRAND_CALLS
	if (talk)
		dbg->message("haltestelle_t::liefere_an", "%d merged in station \"%s\" waren[0].count %d", ware.menge, get_name(), get_warray(0)->get_count());
#endif
		return ware.menge;
	}

	if(find_route(ware) == 65535)
	{
		// target no longer there => delete

		INT_CHECK("simhalt 1364");

		DBG_MESSAGE("haltestelle_t::liefere_an()","%s: delivered goods (%d %s) to ??? via ??? could not be routed to their destination!",get_name(), ware.menge, translator::translate(ware.get_name()) );
		return ware.menge;
	}
#if 1
	// passt das zu bereits wartender ware ?
	if(vereinige_waren(ware)) 
	{
		// dann sind wir schon fertig;
#ifdef DEBUG_SIMRAND_CALLS
	if (talk)
		dbg->message("haltestelle_t::liefere_an", "%d merged(2) in station \"%s\" waren[0].count %d", ware.menge, get_name(), get_warray(0)->get_count());
#endif
		return ware.menge;
	}
#endif
	
	if(ware.is_passenger() && unload_repeat_counter < 3 && is_within_walking_distance_of(ware.get_zwischenziel()) && !connexions[0]->get(ware.get_zwischenziel())->best_convoy.is_bound() && !connexions[0]->get(ware.get_zwischenziel())->best_line.is_bound() && ware.get_last_transfer().is_bound() && ware.get_last_transfer()->get_basis_pos() != ware.get_zwischenziel()->get_basis_pos())
	{
		// If this is within walking distance of the next transfer, and there is not a faster way there, walk there.
		erzeuge_fussgaenger(welt, get_basis_pos3d(), ware.menge);
		ware.set_last_transfer(self);
		unload_repeat_counter ++;
#ifdef DEBUG_SIMRAND_CALLS
		if (talk)
			dbg->message("haltestelle_t::liefere_an", "%d walk to station \"%s\" waren[0].count %d", ware.menge, ware.get_zwischenziel()->get_name(), get_warray(0)->get_count());
#endif
		return ware.get_zwischenziel()->liefere_an(ware);
	}
	else
	{
		// add to internal storage
		add_ware_to_halt(ware);
	}

#ifdef DEBUG_SIMRAND_CALLS
	if (talk)
		dbg->message("haltestelle_t::liefere_an", "%d waiting for transfer to station \"%s\" waren[0].count %d", ware.menge, ware.get_zwischenziel()->get_name(), get_warray(0)->get_count());
#endif
	return ware.menge;
}



#ifdef USE_QUOTE
// rating of this place ...
const char *
haltestelle_t::quote_bezeichnung(int quote, convoihandle_t cnv) const
{
    const char *str = "unbekannt";

    if(quote < 0) {
	str = translator::translate("miserabel");
    } else if(quote < 30) {
	str = translator::translate("schlecht");
    } else if(quote < 60) {
	str = translator::translate("durchschnitt");
    } else if(quote < 90) {
	str = translator::translate("gut");
    } else if(quote < 120) {
	str = translator::translate("sehr gut");
    } else if(quote < 150) {
	str = translator::translate("bestens");
    } else if(quote < 180) {
	str = translator::translate("excellent");
    } else {
	str = translator::translate("spitze");
    }

    return str;
}
#endif



void haltestelle_t::info(cbuffer_t & buf) const
{
	buf.printf(
		translator::translate("Passengers %d %c, %d %c, %d no route, %d too slow"),
		get_pax_happy(),
		30,
		get_pax_unhappy(),
		31,
		get_pax_no_route(),
		get_pax_too_slow()
		);
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
				freight_list_sorter_t::sort_freight(warray, buf, (freight_list_sorter_t::sort_mode_t)sortierung, NULL, "waiting", welt);
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
	FOR(slist_tpl<tile_t>, const& i, tiles) 
	{
		if (gebaeude_t* const gb = i.grund->find<gebaeude_t>()) 
		{
			const haus_besch_t* besch = gb->get_tile()->get_besch();
			if(besch->get_base_station_maintenance() == 2147483647)
			{
				// Default value - no specific maintenance set. Use the old method
				maintenance += welt->get_settings().maint_building * besch->get_level();
			}
			else
			{
				// New method - get the specified factor.
				maintenance += besch->get_station_maintenance();
			}
		}
	}
	return maintenance;
}


// changes this to a public transfer exchange stop
bool haltestelle_t::make_public_and_join( spieler_t *sp )
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
			if(gb) 
			{
				spieler_t *gb_sp=gb->get_besitzer();
				const haus_besch_t* besch = gb->get_tile()->get_besch();
				sint32 costs;
				if(besch->get_base_station_maintenance() == 2147483647)
				{
					// Default value - no specific maintenance set. Use the old method
					costs =welt->get_settings().maint_building * besch->get_level();
				}
				else
				{
					// New method - get the specified factor.
					costs = besch->get_station_maintenance();
				}
				total_costs += costs;
				if(!sp->can_afford(welt->calc_adjusted_monthly_figure(total_costs*60)))
				{
					// Bernd Gabriel: does anybody reassign the already disappropriated buildings?
					return false;
				}
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
	if (!joining.empty()) {
		set_name( joining.front()->get_name());
	}

	while(!joining.empty()) {
		// join this halt with me
		halthandle_t halt = joining.remove_first();

		// now with the second stop
		while(halt.is_bound()  &&  halt!=self) {
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
					sint32 costs;

					if(gb->get_tile()->get_besch()->get_base_station_maintenance() == 2147483647)
					{
						// Default value - no specific maintenance set. Use the old method
						costs = welt->get_settings().maint_building * gb->get_tile()->get_besch()->get_level();
					}
					else
					{
						// New method - get the specified factor.
						costs = gb->get_tile()->get_besch()->get_station_maintenance();
					}
					
					spieler_t::add_maintenance( gb_sp, -costs );

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
	return true;
}


void haltestelle_t::transfer_goods(halthandle_t halt)
{
	if (!self.is_bound() || !halt.is_bound()) {
		return;
	}
#ifdef DEBUG_SIMRAND_CALLS
	bool talk = !strcmp(get_name(), "Newton Abbot Railway Station") || !strcmp(halt->get_name(), "Newton Abbot Railway Station");
#endif
	// transfer goods to halt
	for(uint8 i=0; i<warenbauer_t::get_max_catg_index(); i++) {
		const vector_tpl<ware_t> * warray = waren[i];
		if (warray) {
			FOR(vector_tpl<ware_t>, const& j, *warray) {
				halt->add_ware_to_halt(j);
#ifdef DEBUG_SIMRAND_CALLS
				if (talk)
					dbg->message("haltestelle_t::transfer_goods", "%d transfer from station \"%s\"(warr cnt %d) to \"%s\"(warr cnt %d)", 
					    j.menge, get_name(), get_warray(i)->get_count(), halt->get_name(), halt->get_warray(i)->get_count());
#endif
			}
			delete waren[i];
			waren[i] = NULL;
		}
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
	stationtyp new_station_type = invalid;
	capacity[0] = 0;
	capacity[1] = 0;
	capacity[2] = 0;
	enables &= CROWDED;	// clear flags

	// iterate over all tiles
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		grund_t* const gr = i.grund;
		const gebaeude_t* gb = gr->find<gebaeude_t>();
		const haus_besch_t *besch=gb?gb->get_tile()->get_besch():NULL;

		if(gr->ist_wasser()) {
			// may happend around oil rigs and so on
			new_station_type |= dock;
			// for water factories
			if(besch) {
				// enabled the matching types
				enables |= besch->get_enabled();
				if( welt->get_settings().is_seperate_halt_capacities()  ) 
				{
					if(besch->get_enabled()&1) 
					{
						capacity[0] += besch->get_station_capacity();
					}
					if(besch->get_enabled()&2)
					{
						capacity[1] += besch->get_station_capacity();
					}
					if(besch->get_enabled()&4) 
					{
						capacity[2] += besch->get_station_capacity();
					}
				}
				else 
				{
					// no sperate capacities: sum up all
					capacity[0] += besch->get_station_capacity();
					capacity[2] = capacity[1] = capacity[0];
				}
			}
			continue;
		}

		if(besch==NULL) {
			// no besch, but solid gound?!?
			dbg->error("haltestelle_t::get_station_type()","ground belongs to halt but no besch?");
			continue;
		}
//if(besch) DBG_DEBUG("haltestelle_t::get_station_type()","besch(%p)=%s",besch,besch->get_name());

		// there is only one loading bay ...
		switch (besch->get_utyp()) {
			case haus_besch_t::ladebucht:    new_station_type |= loadingbay;   break;
			case haus_besch_t::hafen:
			case haus_besch_t::binnenhafen:  new_station_type |= dock;         break;
			case haus_besch_t::bushalt:      new_station_type |= busstop;      break;
			case haus_besch_t::airport:      new_station_type |= airstop;      break;
			case haus_besch_t::monorailstop: new_station_type |= monorailstop; break;

			case haus_besch_t::bahnhof:
				if (gr->hat_weg(monorail_wt)) {
					new_station_type |= monorailstop;
				} else {
					new_station_type |= railstation;
				}
				break;

			// two ways on ground can only happen for tram tracks on streets, there buses and trams can stop
			case haus_besch_t::generic_stop:
				switch (besch->get_extra()) {
					case road_wt:
						new_station_type |= (besch->get_enabled()&3)!=0 ? busstop : loadingbay;
						if (gr->has_two_ways()) { // tram track on street
							new_station_type |= tramstop;
						}
						break;
					case water_wt:       new_station_type |= dock;            break;
					case air_wt:         new_station_type |= airstop;         break;
					case monorail_wt:    new_station_type |= monorailstop;    break;
					case track_wt:       new_station_type |= railstation;     break;
					case tram_wt:
						new_station_type |= tramstop;
						if (gr->has_two_ways()) { // tram track on street
							new_station_type |= (besch->get_enabled()&3)!=0 ? busstop : loadingbay;
						}
						break;
					case maglev_wt:      new_station_type |= maglevstop;      break;
					case narrowgauge_wt: new_station_type |= narrowgaugestop; break;
				}
				break;
			default: break;
		}


		// enabled the matching types
		enables |= besch->get_enabled();
		if( welt->get_settings().is_seperate_halt_capacities()  ) {
			if(besch->get_enabled()&1) 
			{
				capacity[0] +=  besch->get_station_capacity();
			}
			if(besch->get_enabled()&2) 
			{
				capacity[1] +=  besch->get_station_capacity();
			}
			if(besch->get_enabled()&4) 
			{
				capacity[2] +=  besch->get_station_capacity();
			}
		}
		else {
			// no sperate capacities: sum up all
			capacity[0] +=  besch->get_station_capacity();
			capacity[2] = capacity[1] = capacity[0];
		}
	}
	station_type = new_station_type;
	recalc_status();

//DBG_DEBUG("haltestelle_t::recalc_station_type()","result=%x, capacity[0]=%i, capacity[1], capacity[2]",new_station_type,capacity[0],capacity[1],capacity[2]);
}



int haltestelle_t::erzeuge_fussgaenger(karte_t *welt, koord3d pos, int anzahl)
{
	if(pedestrian_limit < pedestrian_generate_max)
	{
		pedestrian_limit ++;
		fussgaenger_t::erzeuge_fussgaenger_an(welt, pos, anzahl);
		for(int i=0; i<4 && anzahl>0; i++) 
		{
			fussgaenger_t::erzeuge_fussgaenger_an(welt, pos+koord::nsow[i], anzahl);
		}
	}
	return anzahl;
}



void haltestelle_t::rdwr(loadsave_t *file)
{
	xml_tag_t h( file, "haltestelle_t" );

	sint32 spieler_n;
	koord3d k;
	unload_repeat_counter = 0;
#ifdef DEBUG_SIMRAND_CALLS
	loading = file->is_loading();
#endif
	// will restore halthandle_t after loading
	if(file->get_version() > 110005) 
	{
		if(file->is_saving()) 
		{
			if(!self.is_bound())
			{
				// Something has gone a bit wrong here, as the handle to self is not bound.
				if(!this)
				{
					// Probably superfluous, but best to be sure that this is really not a dud pointer.
					dbg->error("void haltestelle_t::rdwr(loadsave_t *file)", "Handle to self not bound when saving a halt");
					return;
				}
				if(self.get_rep() != this)
				{
					uint16 id = self.get_id();
					self = halthandle_t(this, id);
				}
			}
			uint16 halt_id = self.is_bound() ? self.get_id() : 0;
			file->rdwr_short(halt_id);
		}
		else 
		{
			uint16 halt_id;
			file->rdwr_short(halt_id);
			self.set_id(halt_id);
			if((file->get_experimental_version() >= 10 || file->get_experimental_version() == 0) && halt_id != 0)
			{
				self = halthandle_t(this, halt_id);
			}
			else
			{
				self = halthandle_t(this);
			}
		}
	}
	else 
	{
		if (file->is_loading()) 
		{
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
				if(gr)
				{
					dbg->error("haltestelle_t::rdwr()", "setting to %s", gr->get_pos().get_str() );
				}
			}
			// during loading and saving halts will be referred by their base postion
			// so we may alrady be defined ...
			if(gr && gr->get_halt().is_bound()) {
				dbg->warning( "haltestelle_t::rdwr()", "bound to ground twice at (%i,%i)!", k.x, k.y );
			}
			// prissi: now check, if there is a building -> we allow no longer ground without building!
			const gebaeude_t* gb = gr ? gr->find<gebaeude_t>() : NULL;
			const haus_besch_t *besch=gb ? gb->get_tile()->get_besch():NULL;
			if(besch) {
				add_grund( gr );
			}
			else {
				dbg->warning("haltestelle_t::rdwr()", "will no longer add ground without building at %s!", k.get_str() );
			}
			k.rdwr( file );
		}
	} else {
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			k = i.grund->get_pos();
			k.rdwr( file );
		}
		k = koord3d::invalid;
		k.rdwr( file );
	}

	// BG, 07-MAR-2010: the number of goods categories should be read from in the savegame, 
	// but currently it is not stored although goods and categories can be added by the user 
	// and thus do not depend on file version and therefore not predicatable by simutrans.
	
	uint8 max_catg_count_game = warenbauer_t::get_max_catg_index();
	uint8 max_catg_count_file;

	if(file->is_saving() || file->get_experimental_version() <= 10)
	{
		max_catg_count_file = max_catg_count_game; 
	}
	
	if(file->get_experimental_version() >= 11)
	{
		// Version 11 and above - the maximum category count is saved with the game.
		file->rdwr_byte(max_catg_count_file);
	}

	const char *s;
	init_pos = tiles.empty() ? koord::invalid : tiles.front().grund->get_pos().get_2d();
	if(file->is_saving()) 
	{
		for(unsigned i=0; i<max_catg_count_file; i++) 
		{
			vector_tpl<ware_t> *warray = waren[i];
			uint32 ware_count = 1;

			if(warray) 
			{
				s = "y";	// needs to be non-empty
				file->rdwr_str(s);
				if(file->get_experimental_version() <= 10)
				{
					const uint32 count = warray->get_count();
					uint16 short_count = min(count, 65535);
					file->rdwr_short(short_count);
				}
				else
				{
					// Experimental version 11 and above - very large/busy halts might
					// have a count > 65535, so use the proper uint32 value.
					
					// In previous versions, the use of "short" lead to corrupted saved
					// games when the value was larger than 32,767.

					uint32 count = warray->get_count();
					file->rdwr_long(count);
				}
				FOR(vector_tpl<ware_t>, & ware, *warray) 
				{
					if(file->get_experimental_version() < 11 && ware_count++ > 65535)
					{
						// Discard ware packets > 65535 if the version is < 11, as trying
						// to save greater than this number will corrupt the save.
						ware.menge = 0;
					}
					else
					{
						ware.rdwr(welt,file);
					}
				}
			}
		}
		s = "";
		file->rdwr_str(s);

#ifdef DEBUG_SIMRAND_CALLS
		if (waren[0])
		{
			if (!strcmp(get_name(), "Newton Abbot Railway Station"))
			{
				dbg->message("haltestelle_t::rdwr", "at stop \"%s\" waren[0]->get_count() is %u ", get_name(), waren[0]->get_count());
				//for (int i = 0; i < waren[0]->get_count(); ++i)
				//{
				//	char buf[16];
				//	const ware_t &ware = (*waren[0])[i];
				//	sprintf(buf, "% 8u)", i);
				//	dbg->message(buf, "%u to %s", ware.menge, ware.get_ziel()->get_name());
				//}
				//int x = 0;
			}
		}
#endif
	}
	else 
	{
		// restoring all goods in the station
		char s[256];
		file->rdwr_str(s, lengthof(s));
		uint32 count;
		while(*s) 
		{
			if(file->get_experimental_version() >= 11)
			{
				file->rdwr_long(count);
			}
			else
			{
				// Older versions stored only 16-bit count values.
				uint16 old_count = 0;
				file->rdwr_short(old_count);
				count = (uint32)old_count;
			}
			if(count > 0) 
			{
				for(uint32 i = 0; i < count; i++) 
				{
					// add to internal storage (use this function, since the old categories were different)
					ware_t ware(welt, file);
					if(ware.menge > 0  &&  welt->ist_in_kartengrenzen(ware.get_zielpos()))
					{
						add_ware_to_halt(ware, file->get_experimental_version() >= 2);
					}
					else if(  ware.menge>0  ) 
					{
						dbg->error( "haltestelle_t::rdwr()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str() );
					}
				}
			}
			file->rdwr_str(s, lengthof(s));
		}

		// old games save the list with stations
		// however, we have to rebuilt them anyway for the new format
		if(file->get_version()<99013) 
		{
			short count;
			file->rdwr_short(count);

			for(int i=0; i<count; i++) 
			{
				if(file->is_loading())
				{
					// Dummy loading and saving to maintain backwards compatibility
					koord dummy_koord;
					dummy_koord.rdwr(file);

					char dummy[256];
					file->rdwr_str(dummy,256);
				}
			}
			
		}


#ifdef DEBUG_SIMRAND_CALLS
		if (waren[0])
		{
			if (!strcmp(get_name(), "Newton Abbot Railway Station"))
			{
				dbg->message("haltestelle_t::rdwr", "at stop \"%s\" waren[0]->get_count() is %u ", get_name(), waren[0]->get_count());
				//for (int i = 0; i < waren[0]->get_count(); ++i)
				//{
				//	char buf[16];
				//	const ware_t &ware = (*waren[0])[i];
				//	sprintf(buf, "% 8u)", i);
				//	dbg->message(buf, "%u", ware.menge);
				//}
				//int x = 0;
			}
		}
#endif
	}

	if(file->get_experimental_version() >= 5)
	{
		for (int j = 0; j < 8 /*MAX_HALT_COST*/; j++) 
		{
			for (int k = MAX_MONTHS	- 1; k >= 0; k--) 
			{
				file->rdwr_longlong(financial_history[k][j]);
			}
		}
	}
	else
	{
		// Earlier versions did not have pax_too_slow
		for (int j = 0; j < 8 /*MAX_HALT_COST - 1*/; j++) 
		{
			for (int k = MAX_MONTHS - 1; k >= 0; k--) 
			{
				if(j == 7)
				{
					// Walked passengers in Standard
					// (Experimental stores these in cities, not stops)
					if(file->get_version() >= 111001)
					{
						sint64 dummy = 0;
						file->rdwr_longlong(dummy);
					}
					else
					{
						continue;
					}
				}
				else
				{
					file->rdwr_longlong(financial_history[k][j]);
				}
			}
		}
		for (int k = MAX_MONTHS - 1; k >= 0; k--) 
		{
			if(file->is_loading())
			{	
				financial_history[k][HALT_TOO_SLOW] = 0;
			}
		}
	}

	if(file->get_experimental_version() >= 2)
	{
		for(int i = 0; i < max_catg_count_file; i ++)
		{
			if(file->is_saving())
			{
				uint16 halts_count;
				halts_count = waiting_times[i].get_count();
				file->rdwr_short(halts_count);
				halthandle_t halt;

				FOR(waiting_time_map, & iter, waiting_times[i])
				{
					uint16 id = iter.key;

					if(file->get_experimental_version() >= 10)
					{
						file->rdwr_short(id);
					}
					else
					{
						halt.set_id(id);
						koord save_koord = koord::invalid;
						if(halt.is_bound())
						{
							save_koord = halt->get_basis_pos();
						}
						save_koord.rdwr(file);
					}
					
					uint8 waiting_time_count = iter.value.times.get_count();
					file->rdwr_byte(waiting_time_count);
					ITERATE(iter.value.times, i)
					{
						// Store each waiting time
						uint16 current_time = iter.value.times.get_element(i);
						file->rdwr_short(current_time);
					}

					if(file->get_experimental_version() >= 9)
					{
						waiting_time_set wt = iter.value;
						file->rdwr_byte(wt.month);
					}
				}
				halt.set_id(0);
			}

			else
			{
				waiting_times[i].clear();
				uint16 halts_count;
				file->rdwr_short(halts_count);
				uint16 id = 0;
				for(uint16 k = 0; k < halts_count; k ++)
				{
					if(file->get_experimental_version() >= 10)
					{
						file->rdwr_short(id);
					}
					else
					{
						koord halt_position;
						halt_position.rdwr(file);
						const planquadrat_t* plan = welt->lookup(halt_position);
						if(plan)
						{
							id = plan->get_halt().get_id();
						}
					}	

					fixed_list_tpl<uint16, 32> list;
					uint8 month;
					waiting_time_set set;
					uint8 waiting_time_count;
					file->rdwr_byte(waiting_time_count);
					for(uint8 j = 0; j < waiting_time_count; j ++)
					{
						uint16 current_time;
						file->rdwr_short(current_time);
						list.add_to_tail(current_time);
					}
					if(file->get_experimental_version() >= 9)
					{
						file->rdwr_byte(month);
					}
					else
					{
						month = 0;
					}
					set.month = month;
					set.times = list;
					waiting_times[i].put(id, set);
				}
			}
		}

		if(file->get_experimental_version() > 0 && file->get_experimental_version() < 3)
		{
			uint16 old_paths_timestamp = 0;
			uint16 old_connexions_timestamp = 0;
			bool old_reschedule = false;
			file->rdwr_short(old_paths_timestamp);
			file->rdwr_short(old_connexions_timestamp);
			file->rdwr_bool(old_reschedule);
		}
		else if(file->get_experimental_version() > 0 && file->get_experimental_version() < 10)
		{
			for(short i = 0; i < max_catg_count_file; i ++)
			{
				sint16 dummy;
				bool dummy_2;
				file->rdwr_short(dummy);
				file->rdwr_short(dummy);
				file->rdwr_bool(dummy_2);
			}
		}
	}

	if(file->get_experimental_version() >=9 && file->get_version() >= 110000)
	{
		file->rdwr_bool(do_alternative_seats_calculation);
	}

	if(file->get_experimental_version() >= 10)
	{
		if(file->is_saving())
		{
			uint32 count = halts_within_walking_distance.get_count();
			file->rdwr_long(count);
			ITERATE(halts_within_walking_distance, n)
			{
				halts_within_walking_distance[n].rdwr(file);
			}
		}
		else
		{
			// Loading
			uint32 count = 0;
			file->rdwr_long(count);
			halthandle_t halt;
			for(int n = 0; n < count; n ++)
			{
				halt.rdwr(file);
				add_halt_within_walking_distance(halt);
			}
		}
		if(file->get_version() >= 111002)
		{
			file->rdwr_byte(control_towers);
		}
	}
	else
	{
		if(file->is_loading())
		{
			check_nearby_halts();
		}
	}

	pedestrian_limit = 0;
#ifdef DEBUG_SIMRAND_CALLS
	loading = false;
#endif
}



//"Load lock" (Google)
void haltestelle_t::laden_abschliessen(bool need_recheck_for_walking_distance)
{
	// fix good destination coordinates
	for(uint8 i = 0; i < warenbauer_t::get_max_catg_index(); i++) 
	{
		if(waren[i]) 
		{
			vector_tpl<ware_t> * warray = waren[i];
			FOR(vector_tpl<ware_t>, & j, *warray) 
			{
				j.laden_abschliessen(welt, besitzer_p);
			}
			// merge identical entries (should only happen with old games)
			ITERATE_PTR(warray,j)
			{
				if((*warray)[j].menge == 0) 
				{
					continue;
				}
				for(uint32 k = j + 1; k < warray->get_count(); k++) 
				{
					if((*warray)[k].menge > 0 && (*warray)[j].can_merge_with((*warray)[k])) 
					{
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
		if(!all_names.put(bd->get_text(),self)) {
			DBG_MESSAGE("haltestelle_t::set_name()","name %s already used!",bd->get_text());
		}
	}
	
	if(need_recheck_for_walking_distance)
	{
		check_nearby_halts();
	}
	
	recalc_status();	
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
		for (int k = MAX_MONTHS-1; k>=0; k--)
		{
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

	uint32 total_sum = 0;
	if(get_pax_enabled()) {
		const uint32 max_ware = get_capacity(0);
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
		const uint32 max_ware = get_capacity(1);
		const uint32 post = get_ware_summe(warenbauer_t::post);
		total_sum += post;
		if(post>max_ware) {
			status_bits |= post>max_ware+200 ? 2 : 1;
			overcrowded[0] |= 2;
		}
	}

	// now for all goods
	if(status_color!=COL_RED  &&  get_ware_enabled()) {
		const int count = warenbauer_t::get_waren_anzahl();
		const uint32 max_ware = get_capacity(2);
		for( int i=2; i+1<count; i++) {
			const ware_besch_t *wtyp = warenbauer_t::get_info(i+1);
			const uint32 ware_sum = get_ware_summe(wtyp);
			total_sum += ware_sum;
			if(ware_sum>max_ware) {
				status_bits |= (ware_sum>max_ware+32  ||  CROWDED) ? 2 : 1;
				overcrowded[wtyp->get_catg_index()/8] |= 1<<(wtyp->get_catg_index()%8);
			}
		}
	}

	// take the worst color for status
	if(  status_bits  ) {
		status_color = status_bits&2 ? COL_RED : COL_ORANGE;
	}
	else
	{
		status_color = (financial_history[0][HALT_WAITING]+financial_history[0][HALT_DEPARTED] == 0) ? COL_YELLOW : COL_GREEN;
	}

	if((station_type & airstop) && has_no_control_tower())
	{
		status_color = COL_PURPLE;
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
	if (tiles.is_contained(gr)) {
		return false;
	}

	koord pos=gr->get_pos().get_2d();
	gr->set_halt(self);
	tiles.append(gr);

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

	//DBG_MESSAGE("haltestelle_t::add_grund()","pos %i,%i,%i to %s added.",pos.x,pos.y,pos.z,get_name());

	FOR(vector_tpl<fabrik_t*>, const fab, fabrik_t::sind_da_welche(welt, pos - koord(cov, cov), pos + koord(cov, cov))) {
		if(!fab_list.is_contained(fab)) {
			fab_list.insert(fab);
			fab->link_halt(self);
		}
	}

	// check if we have to register line(s) and/or lineless convoy(s) which serve this halt
	vector_tpl<linehandle_t> check_line(0);
	if(  get_besitzer()==welt->get_spieler(1)  ) {
		// must iterate over all players lines ...
		for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
			if(welt->get_spieler(i)) {
				welt->get_spieler(i)->simlinemgmt.get_lines(simline_t::line, &check_line);
				FOR(vector_tpl<linehandle_t>, const j, check_line) {
					// only add unknown lines
					if (!registered_lines.is_contained(j) && j->count_convoys() > 0) {
						FOR(minivec_tpl<linieneintrag_t>, const& k, j->get_schedule()->eintrag) {
							if (get_halt(welt, k.pos, j->get_besitzer()) == self) {
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
		FOR(vector_tpl<linehandle_t>, const j, check_line) {
			// only add unknown lines
			if (!registered_lines.is_contained(j) && j->count_convoys() > 0) {
					FOR(minivec_tpl<linieneintrag_t>, const& k, j->get_schedule()->eintrag) {
					if (get_halt(welt, k.pos, get_besitzer()) == self) {
						registered_lines.append(j);
						break;
					}
				}
			}
		}
		// Knightly : iterate over all convoys
		FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
			// only check lineless convoys which have matching ownership and which are not yet registered
			if(  !cnv->get_line().is_bound()  &&  cnv->get_besitzer()==get_besitzer()  &&  !registered_convoys.is_contained(cnv)  ) {
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

	assert(welt->lookup(pos)->get_halt() == self  &&  gr->is_halt());
	init_pos = tiles.front().grund->get_pos().get_2d();
	path_explorer_t::refresh_all_categories(false);

	check_nearby_halts();

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
	if (i == tiles.begin()  &&  (*i).grund->get_name()) {
		tstrncpy(buf, get_name(), lengthof(buf));
		station_name_to_transfer = buf;
		set_name(NULL);
	}

	// now remove tile from list
	tiles.erase(i);
	path_explorer_t::refresh_all_categories(false);
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

	// factory reach may have been changed ...
	verbinde_fabriken();

	// remove lines eventually
	for(  int j=registered_lines.get_count()-1;  j>=0;  j--  ) {
		bool ok=false;
		FOR(minivec_tpl<linieneintrag_t>, const& k, registered_lines[j]->get_schedule()->eintrag) {
			if (get_halt(welt, k.pos, registered_lines[j]->get_besitzer()) == self) {
				ok = true;
				break;
			}
		}
		// need removal?
		if(!ok) {
			registered_lines.remove_at(j);
		}
	}

	// Knightly : remove registered lineless convoys as well
	for(  int j=registered_convoys.get_count()-1;  j>=0;  --j  ) {
		bool ok = false;
		FOR(minivec_tpl<linieneintrag_t>, const& k, registered_convoys[j]->get_schedule()->eintrag) {
			if (get_halt(welt, k.pos, registered_convoys[j]->get_besitzer()) == self) {
				ok = true;
				break;
			}
		}
		// need removal?
		if(  !ok  ) {
			registered_convoys.remove_at(j);
		}
	}

	check_nearby_halts();

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
			int d = shortest_distance(start, p );
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


void* haltestelle_t::connexion::operator new(size_t /*size*/)
{
	return freelist_t::gimme_node(sizeof(haltestelle_t::connexion));
}

void haltestelle_t::connexion::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(haltestelle_t::connexion),p);
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

/**
 * Get queue position for spacing calculation.
 * @author Inkelyad
 */
int haltestelle_t::get_queue_pos(convoihandle_t cnv) const
{
	linehandle_t line = cnv->get_line();
	int count = 0;
	for(  slist_tpl<convoihandle_t>::const_iterator i = loading_here.begin(), end = loading_here.end();  i != end && (*i) != cnv; ++i )
	{
		if (!(*i).is_bound() )
		{
			continue;
		}
		if ( (*i)->get_line() == line && (*i)->get_schedule()->get_aktuell() == cnv->get_schedule()->get_aktuell() ) {
			count++;
		}
	}
	return count + 1;
}

void haltestelle_t::add_halt_within_walking_distance(halthandle_t halt)
{
	if(halt != self)
	{
		// Halt should not be listed as connected to itself.
		halts_within_walking_distance.append_unique(halt);
	}
}

void haltestelle_t::remove_halt_within_walking_distance(halthandle_t halt)
{
	halts_within_walking_distance.remove(halt);
}

void haltestelle_t::check_nearby_halts()
{
	halts_within_walking_distance.clear();
	FOR(slist_tpl<tile_t>, const& iter, tiles)
	{
		planquadrat_t *plan = welt->access(iter.grund->get_pos().get_2d());
		if(plan) 
		{
			const halthandle_t *const halt_list = plan->get_haltlist();

			for (int h = plan->get_haltlist_count() - 1; h >= 0; h--) 
			{
				halthandle_t halt = halt_list[h];
				if (halt->is_enabled(warenbauer_t::passagiere)) 
				{
					add_halt_within_walking_distance(halt);
					halt->add_halt_within_walking_distance(self);
				}
			}
		}
	}
	// Must refresh here, but only passengers can walk, so only refresh passengers.
	path_explorer_t::refresh_category(0);
}

bool haltestelle_t::is_within_walking_distance_of(halthandle_t halt) const
{ 
	return welt->get_settings().get_allow_routing_on_foot() && halts_within_walking_distance.is_contained(halt); 
}

uint32 haltestelle_t::get_number_of_halts_within_walking_distance() const
{
	if(welt->get_settings().get_allow_routing_on_foot())
	{
		return halts_within_walking_distance.get_count(); 
	}
	else
	{
		return 0;
	}
} 

bool haltestelle_t::check_access(const spieler_t* sp) const
{
	return !sp || sp == besitzer_p || besitzer_p == NULL || besitzer_p->allows_access_to(sp->get_player_nr());
}

bool haltestelle_t::has_no_control_tower() const
{
	return welt->get_settings().get_allow_airports_without_control_towers() ? false : control_towers == 0;
}
