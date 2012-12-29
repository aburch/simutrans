/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
#include "../simdebug.h"
#include "fabrikbauer.h"
#include "hausbauer.h"
#include "../simworld.h"
#include "../simintr.h"
#include "../simfab.h"
#include "../simmesg.h"
#include "../simtools.h"
#include "../simcity.h"
#include "../simhalt.h"
#include "../player/simplay.h"

#include "../boden/grund.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/pakset_info.h"
#include "../dataobj/translator.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/array_tpl.h"
#include "../sucher/bauplatz_sucher.h"

#include "../utils/cbuffer_t.h"

#include "../gui/karte.h"	// to update map after construction of new industry

// radius for checking places for construction
#define DISTANCE 40


// all factories and their exclusion areas
static array_tpl<uint8> fab_map;
static sint32 fab_map_w=0;


// marks factories with exclusion region in the position map
static void add_factory_to_fab_map(karte_t const* const welt, fabrik_t const* const fab)
{
	koord3d      const& pos     = fab->get_pos();
	sint16       const  spacing = welt->get_settings().get_factory_spacing();
	haus_besch_t const& hbesch  = *fab->get_besch()->get_haus();
	sint16       const  rotate  = fab->get_rotate();
	sint16       const  start_y = max(0, pos.y - spacing);
	sint16       const  start_x = max(0, pos.x - spacing);
	sint16       const  end_y   = min(welt->get_groesse_y() - 1, pos.y + hbesch.get_h(rotate) + spacing);
	sint16       const  end_x   = min(welt->get_groesse_x() - 1, pos.x + hbesch.get_b(rotate) + spacing);
	for (sint16 y = start_y; y < end_y; ++y) {
		for (sint16 x = start_x; x < end_x; ++x) {
			fab_map[fab_map_w * y + x / 8] |= 1 << (x % 8);
		}
	}
}



// create map with all factories and exclusion area
void init_fab_map( karte_t *welt )
{
	fab_map_w = ((welt->get_groesse_x()+7)/8);
	fab_map.resize( fab_map_w*welt->get_groesse_y() );
	for( int i=0;  i<fab_map_w*welt->get_groesse_y();  i++ ) {
		fab_map[i] = 0;
	}
	FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
		add_factory_to_fab_map(welt, f);
	}
}



// true, if factory coordinate
inline bool is_factory_at( sint16 x, sint16 y)
{
	return (fab_map[(fab_map_w*y)+(x/8)]&(1<<(x%8)))!=0;
}


void fabrikbauer_t::neue_karte(karte_t *welt)
{
	init_fab_map( welt );
}



/**
 * bauplatz_mit_strasse_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class factory_bauplatz_mit_strasse_sucher_t: public bauplatz_sucher_t  {

public:
	factory_bauplatz_mit_strasse_sucher_t(karte_t *welt) : bauplatz_sucher_t(welt) {}

	bool strasse_bei(sint16 x, sint16 y) const {
		grund_t *bd = welt->lookup_kartenboden( koord(x,y) );
		return bd && bd->hat_weg(road_wt);
	}

	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const {
		if(bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)) {
			// try to built a little away from previous factory
			for(sint16 y=pos.y;  y<pos.y+h;  y++  ) {
				for(sint16 x=pos.x;  x<pos.x+b;  x++  ) {
					if( is_factory_at(x,y)  ) {
						return false;
					}
					grund_t *gr = welt->lookup_kartenboden(koord(x,y));
					if(gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1))!=NULL) {
						// something on top (monorail or powerlines)
						return false;
					}
				}
			}
			// check to not built on a road
			int i, j;
			for(j=pos.x; j<pos.x+b; j++) {
				for(i=pos.y; i<pos.y+h; i++) {
					if(strasse_bei(j,i)) {
						return false;
					}
				}
			}
			// now check for road connection
			for(i = pos.y; i < pos.y + h; i++) {
				if(strasse_bei(pos.x - 1, i) ||  strasse_bei(pos.x + b, i)) {
					return true;
				}
			}
			for(i = pos.x; i < pos.x + b; i++) {
				if(strasse_bei(i, pos.y - 1) ||  strasse_bei(i, pos.y + h)) {
					return true;
				}
			}
		}
		return false;
	}
};


stringhashtable_tpl<const fabrik_besch_t *> fabrikbauer_t::table;


/**
 * to be able to sort per name
 * necessary in get_random_consumer
 */
static bool compare_fabrik_besch(const fabrik_besch_t* a, const fabrik_besch_t* b)
{
	const int diff = strcmp( a->get_name(), b->get_name() );
	return diff < 0;
}

/* returns a random consumer
 * @author prissi
 */
const fabrik_besch_t *fabrikbauer_t::get_random_consumer(bool electric, climate_bits cl, uint16 timeline )
{
	// get a random city factory
	weighted_vector_tpl<const fabrik_besch_t *> consumer;

	FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& i, table) {
		fabrik_besch_t const* const current = i.value;
		// nur endverbraucher eintragen
		if (current->is_consumer_only()                    &&
			current->get_haus()->is_allowed_climate_bits(cl) &&
			(electric ^ !current->is_electricity_producer())  &&
			(timeline==0  ||  (current->get_haus()->get_intro_year_month() <= timeline  &&  current->get_haus()->get_retire_year_month() > timeline))  ) {
			consumer.insert_unique_ordered(current, current->get_gewichtung(), compare_fabrik_besch);
		}
	}
	// no consumer installed?
	if (consumer.empty()) {
DBG_MESSAGE("fabrikbauer_t::get_random_consumer()","No suitable consumer found");
		return NULL;
	}
	// now find a random one
	fabrik_besch_t const* const fb = pick_any_weighted(consumer);
	DBG_MESSAGE("fabrikbauer_t::get_random_consumer()", "consumer %s found.", fb->get_name());
	return fb;
}



const fabrik_besch_t *fabrikbauer_t::get_fabesch(const char *fabtype)
{
	return table.get(fabtype);
}



void fabrikbauer_t::register_besch(fabrik_besch_t *besch)
{
	uint16 p=besch->get_produktivitaet();
	if(p&0x8000) {
		koord k=besch->get_haus()->get_groesse();
		// to be compatible with old factories, since new code only steps once per factory, not per tile
		besch->set_produktivitaet( (p&0x7FFF)*k.x*k.y );
DBG_DEBUG("fabrikbauer_t::register_besch()","Correction for old factory: Increase poduction from %i by %i",p&0x7FFF,k.x*k.y);
	}
	if(  table.remove(besch->get_name())  ) {
		dbg->warning( "fabrikbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	table.put(besch->get_name(), besch);
}


bool fabrikbauer_t::alles_geladen()
{
	FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& i, table) {
		fabrik_besch_t const* const current = i.value;
		if(  field_group_besch_t * fg = const_cast<field_group_besch_t *>(current->get_field_group())  ) {
			// initialize weighted vector for the field class indices
			fg->init_field_class_indices();
		}
		checksum_t *chk = new checksum_t();
		current->calc_checksum(chk);
		pakset_info_t::append(current->get_name(), chk);
	}
	return true;
}


int fabrikbauer_t::finde_anzahl_hersteller(const ware_besch_t *ware, uint16 timeline)
{
	int anzahl=0;

	FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& t, table) {
		fabrik_besch_t const* const tmp = t.value;
		for (uint i = 0; i < tmp->get_produkte(); i++) {
			const fabrik_produkt_besch_t *produkt = tmp->get_produkt(i);
			if(produkt->get_ware()==ware  &&  tmp->get_gewichtung()>0  &&  (timeline==0  ||  (tmp->get_haus()->get_intro_year_month() <= timeline  &&  tmp->get_haus()->get_retire_year_month() > timeline))  ) {
				anzahl ++;
			}
		}
	}
DBG_MESSAGE("fabrikbauer_t::finde_anzahl_hersteller()","%i producer for good '%s' fount.", anzahl, translator::translate(ware->get_name()));
	return anzahl;
}



/* finds a producer;
 * also water only producer are allowed
 */
const fabrik_besch_t *fabrikbauer_t::finde_hersteller(const ware_besch_t *ware, uint16 timeline )
{
	weighted_vector_tpl<const fabrik_besch_t *> producer;

	FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& t, table) {
		fabrik_besch_t const* const tmp = t.value;
		if (tmp->get_gewichtung()>0  &&  (timeline==0  ||  (tmp->get_haus()->get_intro_year_month() <= timeline  &&  tmp->get_haus()->get_retire_year_month() > timeline))) {
			for (uint i = 0; i < tmp->get_produkte(); i++) {
				const fabrik_produkt_besch_t *produkt = tmp->get_produkt(i);
				if(produkt->get_ware()==ware) {
					producer.insert_unique_ordered(tmp, tmp->get_gewichtung(), compare_fabrik_besch);
					break;
				}
			}
		}
	}

	// no producer installed?
	if (producer.empty()) {
		dbg->error("fabrikbauer_t::finde_hersteller()","no producer for good '%s' was found", translator::translate(ware->get_name()));
		return NULL;
	}
	// now find a random one
	// now find a random one
	fabrik_besch_t const* const besch = pick_any_weighted(producer);
	DBG_MESSAGE("fabrikbauer_t::finde_hersteller()","producer for good '%s' was found %s", translator::translate(ware->get_name()),besch->get_name());
	return besch;
}


koord3d fabrikbauer_t::finde_zufallsbauplatz(karte_t *welt, const koord3d pos, const int radius, koord groesse, bool wasser, const haus_besch_t *besch, bool ignore_climates)
{
	bool is_fabrik = besch->get_utyp()==haus_besch_t::fabrik;

	if(wasser) {
		groesse += koord(6,6);
	}

	climate_bits climates = !ignore_climates ? besch->get_allowed_climate_bits() : ALL_CLIMATES;

	uint32 diam   = 2*radius + 1;
	uint32 size   = diam * diam;
	uint32 offset = diam*groesse.x*groesse.y + 1;
	uint32 index  = simrand(size);
	koord k;
	for(uint32 i = 0; i<size; i++, index+=offset) {
		// as offset % size == 1, we are guaranteed that the iteration hits all tiles and does not repeat itself
		k = koord( pos.x-radius + (index / diam), pos.y-radius + (index % diam));

		if (!welt->ist_in_kartengrenzen(k)) {
			continue;
		}
		// to close to existing factory
		if(  is_fabrik  &&  is_factory_at(k.x,k.y)  ) {
			continue;
		}
		// climate check
		if(  fabrik_t::ist_bauplatz(welt, k, groesse,wasser,climates)  ) {
			// we accept first hit
			goto finish;
		}
		// next search will be groesse.x rows down, groesse.y+1 columns left
	}
	// nothing found
	return koord3d(-1, -1, -1);

finish:
	koord3d p = welt->lookup_kartenboden(k)->get_pos();
	if(wasser) {
		// take care of offset
		p += koord3d(3, 3, 0);
	}
	return p;
}


/* Create a certain numer of tourist attractions
 * @author prissi
 */
void fabrikbauer_t::verteile_tourist(karte_t* welt, int max_number)
{
	// current count
	int current_number=0;

	// select without timeline disappearing dates
	if(hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),true,temperate_climate)==NULL) {
		// nothing at all?
		return;
	}

	// very fast, so we do not bother updating progress bar
	printf("Distributing %i tourist attractions ...\n",max_number);fflush(NULL);

	int retrys = max_number*4;
	while(current_number<max_number  &&  retrys-->0) {
		koord3d	pos=koord3d( koord::koord_random(welt->get_groesse_x(),welt->get_groesse_y()),1);
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),true,(climate)simrand((int)arctic_climate+1));

		// no attractions for that climate or too new
		if(attraction==NULL  ||  (welt->use_timeline()  &&  attraction->get_intro_year_month()>welt->get_current_month()) ) {
			continue;
		}

		int	rotation=simrand(attraction->get_all_layouts()-1);
		pos = finde_zufallsbauplatz(welt, pos, 20, attraction->get_groesse(rotation),false,attraction,false);	// so far -> land only
		if(welt->lookup(pos)) {
			// Platz gefunden ...
			hausbauer_t::baue(welt, welt->get_spieler(1), pos, rotation, attraction);
			current_number ++;
			retrys = max_number*4;
		}

	}
	// update an open map
	reliefkarte_t::get_karte()->calc_map_groesse();
}

class RelativeDistanceOrdering
{
private:
	const koord m_origin;
public:
	RelativeDistanceOrdering(const koord& origin)
		: m_origin(origin)
	{ /* nothing */ }

	/**
	* Returns true if `a' is closer to the origin than `b', otherwise false.
	*/
	bool operator()(const stadt_t *a, const stadt_t *b) const
	{
		return koord_distance(m_origin, a->get_pos()) < koord_distance(m_origin, b->get_pos());
	}
};

/**
 * Build factory according to instructions in 'info'
 * @author Hj.Malthaner
 */
fabrik_t* fabrikbauer_t::baue_fabrik(karte_t* welt, koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d pos, spieler_t* spieler)
{
	fabrik_t * fab = new fabrik_t(pos, spieler, info, initial_prod_base);

	if(parent) {
		fab->add_lieferziel(parent->get_2d());
	}

	// now build factory
	fab->baue(rotate);
	welt->add_fab(fab);
	add_factory_to_fab_map(welt, fab);

	// make all water station
	if(info->get_platzierung() == fabrik_besch_t::Wasser) {
		const haus_besch_t *besch = info->get_haus();
		koord dim = besch->get_groesse(rotate);

		koord k;
		halthandle_t halt = haltestelle_t::create(welt, pos.get_2d(), welt->get_spieler(1));
		if(halt.is_bound()) {

			for(k.x=pos.x; k.x<pos.x+dim.x; k.x++) {
				for(k.y=pos.y; k.y<pos.y+dim.y; k.y++) {
					if(welt->ist_in_kartengrenzen(k)) {
						// add all water to station
						grund_t *gr = welt->lookup_kartenboden(k);
						// build only on gb, otherwise can't remove it
						// also savegame restore only halt on gb
						// this needs for bad fish swarm
						if(gr->ist_wasser()  &&  gr->hat_weg(water_wt) == 0  &&  gr->find<gebaeude_t>()) {
							halt->add_grund( gr );
						}
					}
				}
			}
			halt->set_name( translator::translate(info->get_name()) );
			halt->recalc_station_type();
		}
	}
	else {
		// connenct factory to stations
		// search for near stations and connect factory to them
		koord dim = info->get_haus()->get_groesse(rotate);
		koord k;

		for(  k.x=pos.x;  k.x<=pos.x+dim.x;  k.x++  ) {
			for(  k.y=pos.y;  k.y<=pos.y+dim.y;  k.y++  ) {
				const planquadrat_t *plan = welt->lookup(k);
				const halthandle_t *halt_list = plan->get_haltlist();
				for(  unsigned h=0;  h<plan->get_haltlist_count();  h++  ) {
					halt_list[h]->verbinde_fabriken();
				}
			}
		}
	}

	// add passenger to pax>0, (so no sucide diver at the fish swarm)
	if(info->get_pax_level()>0) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		vector_tpl<stadt_t *>distance_stadt( staedte.get_count() );

		FOR(weighted_vector_tpl<stadt_t*>, const i, staedte) {
			distance_stadt.insert_ordered(i, RelativeDistanceOrdering(fab->get_pos().get_2d()));
		}
		settings_t const& s = welt->get_settings();
		FOR(vector_tpl<stadt_t*>, const i, distance_stadt) {
			uint32 const ntgt = fab->get_target_cities().get_count();
			if (ntgt >= s.get_factory_worker_maximum_towns()) break;
			if (ntgt < s.get_factory_worker_minimum_towns() ||
					koord_distance(fab->get_pos(), i->get_pos()) < s.get_factory_worker_radius()) {
				fab->add_target_city(i);
			}
		}
	}
	return fab;
}


// check, if we have to rotate the factories before building this tree
bool fabrikbauer_t::can_factory_tree_rotate( const fabrik_besch_t *besch )
{
	// we are finished: we cannont rotate
	if(!besch->get_haus()->can_rotate()) {
		return false;
	}

	// now check for all products (should be changed later for the root)
	for(  int i=0;  i<besch->get_lieferanten();  i++   ) {

		const ware_besch_t *ware = besch->get_lieferant(i)->get_ware();

		// infortunately, for every for iteration, we have to check all factories ...
		FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& t, table) {
			fabrik_besch_t const* const tmp = t.value;
			// now check, if we produce this ...
			for (uint i = 0; i < tmp->get_produkte(); i++) {
				if(tmp->get_produkt(i)->get_ware()==ware  &&  tmp->get_gewichtung()>0) {

					if(!can_factory_tree_rotate( tmp )) {
						return false;
					}
					// check next factory
					break;
				}
			}
		}
	}
	// all good -> true;
	return true;
}


/**
 * Build a full chain of factories
 * Precondition before calling this function: pos is suitable for factory construction
 */
int fabrikbauer_t::baue_hierarchie(koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d* pos, spieler_t* sp, int number_of_chains )
{
	karte_t* welt = sp->get_welt();
	int n = 1;
	int org_rotation = -1;

	if(info==NULL) {
		// no industry found
		return 0;
	}

	// no cities at all?
	if (info->get_platzierung() == fabrik_besch_t::Stadt  &&  welt->get_staedte().empty()) {
		return 0;
	}

	// rotate until we can save it, if one of the factory is non-rotateable ...
	if(welt->cannot_save()  &&  parent==NULL  &&  !can_factory_tree_rotate(info)  ) {
		org_rotation = welt->get_settings().get_rotation();
		for(  int i=0;  i<3  &&  welt->cannot_save();  i++  ) {
			pos->rotate90( welt->get_groesse_y()-info->get_haus()->get_h(rotate) );
			welt->rotate90();
		}
		assert( !welt->cannot_save() );
	}

	// intown needs different place search
	if (info->get_platzierung() == fabrik_besch_t::Stadt) {

		koord size=info->get_haus()->get_groesse(0);

		// built consumer (factory) intown
		stadt_t *city = welt->suche_naechste_stadt(pos->get_2d());

		/* Three variants:
		 * A:
		 * A building site, preferably close to the town hall with a street next to it.
		 * This could be a temporary problem, if a city has no such site and the search
		 * continues to the next city.
		 * Otherwise seems to me the most realistic.
		 */
		bool is_rotate=info->get_haus()->get_all_layouts()>1  &&  size.x!=size.y  &&  info->get_haus()->can_rotate();
		// first try with standard orientation
		koord k = factory_bauplatz_mit_strasse_sucher_t(welt).suche_platz(city->get_pos(), size.x, size.y, info->get_haus()->get_allowed_climate_bits());

		// second try: rotated
		koord k1 = koord::invalid;
		if (is_rotate  &&  (k == koord::invalid  ||  simrand(256)<128)) {
			k1 = factory_bauplatz_mit_strasse_sucher_t(welt).suche_platz(city->get_pos(), size.y, size.x, info->get_haus()->get_allowed_climate_bits());
		}

		rotate = simrand( info->get_haus()->get_all_layouts() );
		if (k1 == koord::invalid) {
			if (size.x != size.y) {
				rotate &= 2; // rotation must be even number
			}
		}
		else {
			k = k1;
			rotate |= 1; // rotation must be odd number
		}
		if (!info->get_haus()->can_rotate()) {
			rotate = 0;
		}

		INT_CHECK( "fabrikbauer 588" );

		/* B:
		 * Also good.  The final factories stand possibly somewhat outside of the city however not far away.
		 * (does not obey climates though!)
		 */
#if 0
		k = finde_zufallsbauplatz(welt, welt->lookup(city->get_pos())->get_boden()->get_pos(), 3, land_bau.dim).get_2d();
#endif /* 0 */

		/* C:
		 * A building site, as near as possible to the city hall.
		 *  If several final factories land in one city, they are often
		 * often hidden behind a row of houses, cut off from roads.
		 */
#if 0
		k = bauplatz_sucher_t(welt).suche_platz(city->get_pos(), land_bau.dim.x, land_bau.dim.y, info->get_haus()->get_allowed_climate_bits(), &is_rotate);
#endif /* 0 */

		if(k != koord::invalid) {
			*pos = welt->lookup_kartenboden(k)->get_pos();
		}
		else {
			return 0;
		}
	}

	DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Construction of %s at (%i,%i).",info->get_name(),pos->x,pos->y);
	INT_CHECK("fabrikbauer 594");

	const fabrik_t *our_fab=baue_fabrik(welt, parent, info, initial_prod_base, rotate, *pos, sp);

	INT_CHECK("fabrikbauer 596");

	// now built supply chains for all products
	for(int i=0; i<info->get_lieferanten()  &&  i<number_of_chains; i++) {
		n += baue_link_hierarchie( our_fab, info, i, sp);
	}

	// finally
	if(parent==NULL) {
		DBG_MESSAGE("fabrikbauer_t::baue_hierarchie()","update karte");

		// update the map if needed
		reliefkarte_t::get_karte()->calc_map_groesse();

		INT_CHECK( "fabrikbauer 730" );

		// must rotate back?
		if(org_rotation>=0) {
			for (int i = 0; i < 4 && welt->get_settings().get_rotation() != org_rotation; ++i) {
				pos->rotate90( welt->get_groesse_y()-1 );
				welt->rotate90();
			}
			welt->update_map();
		}
	}

	return n;
}



int fabrikbauer_t::baue_link_hierarchie(const fabrik_t* our_fab, const fabrik_besch_t* info, int lieferant_nr, spieler_t* sp)
{
	int n = 0;	// number of additional factories
	karte_t* welt = sp->get_welt();
	/* first we try to connect to existing factories and will do some
	 * crossconnect (if wanted)
	 * We must take care, to add capacity for the crossconnected ones!
	 */
	const fabrik_lieferant_besch_t *lieferant = info->get_lieferant(lieferant_nr);
	const ware_besch_t *ware = lieferant->get_ware();
	const int anzahl_hersteller=finde_anzahl_hersteller( ware, welt->get_timeline_year_month() );

	if(anzahl_hersteller==0) {
		dbg->error("fabrikbauer_t::baue_hierarchie()","No producer for %s found, chain uncomplete!",ware->get_name() );
		return 0;
	}

	// how much we need?
	sint32 verbrauch = our_fab->get_base_production()*lieferant->get_verbrauch();

	slist_tpl<fabs_to_crossconnect_t> factories_to_correct;
	slist_tpl<fabrik_t *> new_factories;	// since the crosscorrection must be done later
	slist_tpl<fabrik_t *> crossconnected_supplier;	// also done after the construction of new chains

	int lcount = lieferant->get_anzahl();
	int lfound = 0;	// number of found producers

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","lieferanten %i, lcount %i (need %i of %s)",info->get_lieferanten(),lcount,verbrauch,ware->get_name());

	// Hajo: search if there already is one or two (crossconnect everything if possible)
	FOR(slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()) {
		// Try to find matching factories for this consumption, but don't find more than two times number of factories requested.
		if ((lcount != 0 || verbrauch <= 0) && lcount < lfound + 1) break;

		// connect to an existing one, if this is an producer
		if(fab->vorrat_an(ware) > -1) {

			// for sources (oil fields, forests ... ) prefer thoses with a smaller distance
			const unsigned distance = koord_distance(fab->get_pos(),our_fab->get_pos());

			if(distance>6) {//  &&  distance < simrand(welt->get_groesse_x()+welt->get_groesse_y())) {
				// ok, this would match
				// but can she supply enough?

				// now guess, how much this factory can supply
				const fabrik_besch_t* const fb = fab->get_besch();
				for (uint gg = 0; gg < fb->get_produkte(); gg++) {
					if (fb->get_produkt(gg)->get_ware() == ware && fab->get_lieferziele().get_count() < 10) { // does not make sense to split into more ...
						sint32 production_left = fab->get_base_production() * fb->get_produkt(gg)->get_faktor();
						const vector_tpl <koord> & lieferziele = fab->get_lieferziele();
						FOR(vector_tpl<koord>, const& i, lieferziele) {
							if (production_left <= 0) break;
							fabrik_t* const zfab = fabrik_t::get_fab(welt, i);
							for(int zz=0;  zz<zfab->get_besch()->get_lieferanten();  zz++) {
								if(zfab->get_besch()->get_lieferant(zz)->get_ware()==ware) {
									production_left -= zfab->get_base_production()*zfab->get_besch()->get_lieferant(zz)->get_verbrauch();
									break;
								}
							}
						}
						// here is actually capacity left (or sometimes just connect anyway)!
						if (production_left > 0 || simrand(100) < (uint32)welt->get_settings().get_crossconnect_factor()) {
							if(production_left>0) {
								verbrauch -= production_left;
								fab->add_lieferziel(our_fab->get_pos().get_2d());
								DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","supplier %s can supply approx %i of %s to us", fb->get_name(), production_left, ware->get_name());
							}
							else {
								/* we steal something; however the total capacity
								 * needed is the same. Therefore, we just keep book
								 * from whose factories from how many we stole */
								crossconnected_supplier.append(fab);
								FOR(vector_tpl<koord>, const& t, lieferziele) {
									fabrik_t* zfab = fabrik_t::get_fab(welt, t);
									slist_tpl<fabs_to_crossconnect_t>::iterator i = std::find(factories_to_correct.begin(), factories_to_correct.end(), fabs_to_crossconnect_t(zfab, 0));
									if (i == factories_to_correct.end()) {
										factories_to_correct.append(fabs_to_crossconnect_t(zfab, 1));
									}
									else {
										i->demand += 1;
									}
								}
								// the needed produktion to be built does not change!
							}
							lfound ++;
						}
						break; // since we found the product ...
					}
				}
			}
		}
	}

	INT_CHECK( "fabrikbauer 670" );

	/* try to add all types of factories until demand is satisfied
	 * or give up after 50 tries
	 */
	int retry = 0;
	const fabrik_besch_t *hersteller = NULL;
	static koord retry_koord[1+8+16]={
		koord(0,0), // center
		koord(-1,-1), koord(0,-1), koord(1,-1), koord(1,0), koord(1,1), koord(0,1), koord(-1,1), koord(-1,0), // nearest neighbour
		koord(-2,-2), koord(-1,-2), koord(0,-2), koord(1,-2), koord(2,-2), koord(2,-1), koord(2,0), koord(2,1), koord(2,2), koord(1,2), koord(0,2), koord(-1,2), koord(-2,2), koord(-2,1), koord(-2,0), koord(-2,1) // second nearest neighbour
	};

	for(int j=0;  j<50  &&  (lcount>lfound  ||  lcount==0)  &&  verbrauch>0;  j++  ) {

		if(retry==0  ||  retry>25) {
			hersteller = finde_hersteller( ware, welt->get_timeline_year_month() );
			// no one at all
			if(hersteller==NULL) {
				if(welt->use_timeline()) {
					// can happen with timeline
					if (!info->is_consumer_only()) {
						dbg->error( "fabrikbauer_t::baue_hierarchie()", "no produder for %s yet!", ware->get_name() );
						return 0;
					}
					else {
						// only consumer: Will do with partly covered chains
						dbg->warning( "fabrikbauer_t::baue_hierarchie()", "no produder for %s yet!", ware->get_name() );
						break;
					}
				}
				else {
					// must not happen else
					dbg->fatal( "fabrikbauer_t::baue_hierarchie()", "no produder for %s yet!", ware->get_name() );
				}
			}
			if(info==hersteller) {
				// loop: we must stop here!
				dbg->fatal("fabrikbauer_t::baue_hierarchie()","found myself! (pak corrupted?)");
			}
			retry = 0;
		}

		int rotate = simrand(hersteller->get_haus()->get_all_layouts()-1);
		koord3d parent_pos = our_fab->get_pos();
		// ignore climates, when already 40 retrys occurred ...
		koord3d k = finde_zufallsbauplatz(welt, our_fab->get_pos()+(retry_koord[retry%25]*DISTANCE*2), DISTANCE, hersteller->get_haus()->get_groesse(rotate),hersteller->get_platzierung()==fabrik_besch_t::Wasser,hersteller->get_haus(),j>40);

		INT_CHECK("fabrikbauer 697");

		if(welt->lookup(k)) {
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Try to built lieferant %s at (%i,%i) r=%i for %s.",hersteller->get_name(),k.x,k.y,rotate,info->get_name());
			n += baue_hierarchie(&parent_pos, hersteller, -1 /*random prodbase */, rotate, &k, sp, 10000 );
			lfound ++;

			INT_CHECK( "fabrikbauer 702" );

			// now substract current supplier
			fabrik_t *fab = fabrik_t::get_fab(welt, k.get_2d() );
			if(fab==NULL) {
				continue;
			}
			new_factories.append(fab);

			// connect new supplier to us
			const fabrik_besch_t* const fb = fab->get_besch();
			for (uint gg = 0; gg < fab->get_besch()->get_produkte(); gg++) {
				if (fb->get_produkt(gg)->get_ware() == ware) {
					sint32 produktion = fab->get_base_production() * fb->get_produkt(gg)->get_faktor();
					// the take care of how much this factorycould supply
					verbrauch -= produktion;
					DBG_MESSAGE("fabrikbauer_t::baue_hierarchie", "new supplier %s can supply approx %i of %s to us", fb->get_name(), produktion, ware->get_name());
					break;
				}
			}
			retry = 0;
		}
		else {
			k = our_fab->get_pos()+(retry_koord[retry%25]*DISTANCE*2);
DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","failed to built lieferant %s around (%i,%i) r=%i for %s.",hersteller->get_name(),k.x,k.y,rotate,info->get_name());
			retry ++;
		}
	}

	/* now we add us to all crossconnected factories */
	FOR(slist_tpl<fabrik_t*>, const i, crossconnected_supplier) {
		i->add_lieferziel(our_fab->get_pos().get_2d());
	}

	/* now the crossconnect part:
	 * connect also the factories we stole from before ... */
	FOR(slist_tpl<fabrik_t*>, const fab, new_factories) {
		for (slist_tpl<fabs_to_crossconnect_t>::iterator i = factories_to_correct.begin(), end = factories_to_correct.end(); i != end;) {
			i->demand -= 1;
			fab->add_lieferziel(i->fab->get_pos().get_2d());
			i->fab->add_supplier(fab->get_pos().get_2d());
			if (i->demand < 0) {
				i = factories_to_correct.erase(i);
			}
			else {
				++i;
			}
		}
	}

	INT_CHECK( "fabrikbauer 723" );

	return n;
}



/* this function is called, whenever it is time for industry growth
 * If there is still a pending consumer, it will first complete another chain for it
 * If not, it will decide to either built a power station (if power is needed)
 * or built a new consumer near the indicated position
 * @return: number of factories built
 */
int fabrikbauer_t::increase_industry_density( karte_t *welt, bool tell_me )
{
	int nr = 0;
	fabrik_t *last_built_consumer = NULL;
	int last_built_consumer_ware = 0;

	// find last consumer
	if(!welt->get_fab_list().empty()) {
		FOR(slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()) {
			if (fab->get_besch()->is_consumer_only()) {
				last_built_consumer = fab;
				break;
			}
		}
		// ok, found consumer
		if(  last_built_consumer  ) {
			for(  int i=0;  i < last_built_consumer->get_besch()->get_lieferanten();  i++  ) {
				ware_besch_t const* const w = last_built_consumer->get_besch()->get_lieferant(i)->get_ware();
				FOR(vector_tpl<koord>, const& j, last_built_consumer->get_suppliers()) {
					fabrik_besch_t const* const fb = fabrik_t::get_fab(welt, j)->get_besch();
					for (uint32 k = 0; k < fb->get_produkte(); k++) {
						if (fb->get_produkt(k)->get_ware() == w) {
							last_built_consumer_ware = i+1;
							goto next_ware_check;
						}
					}
				}
next_ware_check:
				// ok, found something, text next
				;
			}
		}
	}

	// first: do we have to continue unfinished buissness?
	if(last_built_consumer  &&  last_built_consumer_ware < last_built_consumer->get_besch()->get_lieferanten()) {
		int org_rotation = -1;
		// rotate until we can save it, if one of the factory is non-rotateable ...
		if(welt->cannot_save()  &&  !can_factory_tree_rotate(last_built_consumer->get_besch()) ) {
			org_rotation = welt->get_settings().get_rotation();
			for(  int i=0;  i<3  &&  welt->cannot_save();  i++  ) {
				welt->rotate90();
			}
			assert( !welt->cannot_save() );
		}

		uint32 last_suppliers = last_built_consumer->get_suppliers().get_count();
		do {
			nr += baue_link_hierarchie( last_built_consumer, last_built_consumer->get_besch(), last_built_consumer_ware, welt->get_spieler(1) );
			last_built_consumer_ware ++;
		} while(  last_built_consumer_ware < last_built_consumer->get_besch()->get_lieferanten()  &&  last_built_consumer->get_suppliers().get_count()==last_suppliers  );

		// must rotate back?
		if(org_rotation>=0) {
			for (int i = 0; i < 4 && welt->get_settings().get_rotation() != org_rotation; ++i) {
				welt->rotate90();
			}
			welt->update_map();
		}

		// only return, if successfull
		if(  last_built_consumer->get_suppliers().get_count() > last_suppliers  ) {
			DBG_MESSAGE( "fabrikbauer_t::increase_industry_density()", "added ware %i to factory %s", last_built_consumer_ware, last_built_consumer->get_name() );
			// tell the player
			if(tell_me) {
				stadt_t *s = welt->suche_naechste_stadt( last_built_consumer->get_pos().get_2d() );
				const char *stadt_name = s ? s->get_name() : "simcity";
				cbuffer_t buf;
				buf.printf( translator::translate("Factory chain extended\nfor %s near\n%s built with\n%i factories."), translator::translate(last_built_consumer->get_name()), stadt_name, nr );
				welt->get_message()->add_message(buf, last_built_consumer->get_pos().get_2d(), message_t::industry, CITY_KI, last_built_consumer->get_besch()->get_haus()->get_tile(0)->get_hintergrund(0, 0, 0));
			}
			reliefkarte_t::get_karte()->calc_map();
			return nr;
		}
	}

	// ok, nothing to built, thus we must start new

	// first decide, whether a new powerplant is needed or not
	uint32 electric_supply = 0;
	uint32 electric_demand = 1;

	FOR(slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()) {
		if(  fab->get_besch()->is_electricity_producer()  ) {
			electric_supply += fab->get_scaled_electric_amount() / PRODUCTION_DELTA_T;
		}
		else {
			electric_demand += fab->get_scaled_electric_amount() / PRODUCTION_DELTA_T;
		}
	}

	// now decide producer of electricity or normal ...
	sint32 promille = (electric_supply*1000l)/electric_demand;
	int    no_electric = promille > welt->get_settings().get_electric_promille();
	DBG_MESSAGE( "fabrikbauer_t::increase_industry_density()", "production of electricity/total production is %i/%i (%i o/oo)", electric_supply, electric_demand, promille );

	bool not_yet_too_desperate_to_ignore_climates = false;
	while(  no_electric<2  ) {
		for(int retrys=20;  retrys>0;  retrys--  ) {
			const fabrik_besch_t *fab=get_random_consumer( no_electric==0, ALL_CLIMATES, welt->get_timeline_year_month() );
			if(fab) {
				const bool in_city = fab->get_platzierung() == fabrik_besch_t::Stadt;
				if (in_city && welt->get_staedte().empty()) {
					// we cannot built this factory here
					continue;
				}
				koord   testpos = in_city ? pick_any_weighted(welt->get_staedte())->get_pos() : koord::koord_random(welt->get_groesse_x(), welt->get_groesse_y());
				koord3d pos =  welt->lookup_kartenboden( testpos )->get_pos();
				int     rotation=simrand(fab->get_haus()->get_all_layouts()-1);
				if(!in_city) {
					pos = finde_zufallsbauplatz(welt, pos, 20, fab->get_haus()->get_groesse(rotation),fab->get_platzierung()==fabrik_besch_t::Wasser,fab->get_haus(),not_yet_too_desperate_to_ignore_climates);
				}
				if(welt->lookup(pos)) {
					// Platz gefunden ...
					nr += baue_hierarchie(NULL, fab, -1 /*random prodbase */, rotation, &pos, welt->get_spieler(1), 1 );
					if(nr>0) {
						fabrik_t *our_fab = fabrik_t::get_fab( welt, pos.get_2d() );
						reliefkarte_t::get_karte()->calc_map_groesse();
						// tell the player
						if(tell_me) {
							stadt_t *s = welt->suche_naechste_stadt( pos.get_2d() );
							const char *stadt_name = s ? s->get_name() : "simcity";
							cbuffer_t buf;
							buf.printf( translator::translate("New factory chain\nfor %s near\n%s built with\n%i factories."), translator::translate(our_fab->get_name()), stadt_name, nr );
							welt->get_message()->add_message(buf, pos.get_2d(), message_t::industry, CITY_KI, our_fab->get_besch()->get_haus()->get_tile(0)->get_hintergrund(0, 0, 0));
						}
						return nr;
					}
				}
				else if(  retrys==1  &&  not_yet_too_desperate_to_ignore_climates  ) {
					// from now one, we will ignore climates to avoid broken chains
					not_yet_too_desperate_to_ignore_climates = true;
					retrys = 20;
				}
			}
		}
		// if electricity not sucess => try next
		no_electric ++;
	}

	// we should not reach here, because that means neither land nor city industries exist ...
	dbg->warning( "fabrikbauer_t::increase_industry_density()", "No suitable city industry found => pak missing something?" );
	return 0;
}
