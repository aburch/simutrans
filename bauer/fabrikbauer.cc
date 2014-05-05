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

#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../network/pakset_info.h"
#include "../dataobj/translator.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/array_tpl.h"
#include "../sucher/bauplatz_sucher.h"

#include "../utils/cbuffer_t.h"

#include "../gui/karte.h"	// to update map after construction of new industry


karte_ptr_t fabrikbauer_t::welt;

/// Default factory spacing
static int DISTANCE = 40;

/// all factories and their exclusion areas
static array_tpl<uint8> fab_map;

/// width of the factory map
static sint32 fab_map_w=0;


/**
 * Marks factories and their exclusion region in the position map.
 */
static void add_factory_to_fab_map(karte_t const* const welt, fabrik_t const* const fab)
{
	koord3d      const& pos     = fab->get_pos();
	sint16       const  spacing = welt->get_settings().get_min_factory_spacing();
	haus_besch_t const& hbesch  = *fab->get_besch()->get_haus();
	sint16       const  rotate  = fab->get_rotate();
	sint16       const  start_y = max(0, pos.y - spacing);
	sint16       const  start_x = max(0, pos.x - spacing);
	sint16       const  end_y   = min(welt->get_size().y - 1, pos.y + hbesch.get_h(rotate) + spacing);
	sint16       const  end_x   = min(welt->get_size().x - 1, pos.x + hbesch.get_b(rotate) + spacing);
	for (sint16 y = start_y; y < end_y; ++y) {
		for (sint16 x = start_x; x < end_x; ++x) {
			fab_map[fab_map_w * y + x / 8] |= 1 << (x % 8);
		}
	}
}


/**
 * Creates map with all factories and exclusion area.
 */
void init_fab_map( karte_t *welt )
{
	fab_map_w = ((welt->get_size().x+7)/8);
	fab_map.resize( fab_map_w*welt->get_size().y );
	for( int i=0;  i<fab_map_w*welt->get_size().y;  i++ ) {
		fab_map[i] = 0;
	}
	FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
		add_factory_to_fab_map(welt, f);
	}
	if(  welt->get_settings().get_max_factory_spacing_percent()  ) {
		DISTANCE = (welt->get_size_max() * welt->get_settings().get_max_factory_spacing_percent()) / 100l;
	}
	else {
		DISTANCE = welt->get_settings().get_max_factory_spacing();
	}
}


/**
 * @param x,y world position
 * @returns true, if factory coordinate
 */
inline bool is_factory_at( sint16 x, sint16 y)
{
	return (fab_map[(fab_map_w*y)+(x/8)]&(1<<(x%8)))!=0;
}


void fabrikbauer_t::neue_karte()
{
	init_fab_map( welt );
}


/**
 * Searches for a suitable building site using suche_platz().
 * The site is chosen so that there is a street next to the building site.
 */
class factory_bauplatz_mit_strasse_sucher_t: public bauplatz_sucher_t  {

public:
	factory_bauplatz_mit_strasse_sucher_t(karte_t* welt) : bauplatz_sucher_t(welt) {}

	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const
	{
		if(  !bauplatz_sucher_t::ist_platz_ok(pos, b, h, cl)  ) {
			return false;
		}
		bool next_to_road = false;

		for (sint16 y = -1;  y < h; y++) {
			for (sint16 x = -1; x < b; x++) {
				koord k(pos + koord(x,y));
				grund_t *gr = welt->lookup_kartenboden(k);
				if (!gr) {
					return false;
				}
				if (	0 <= x  &&  x < b-1  &&  0 <= y  &&  y < h-1) {
					// not at border: is there something top like elevated monorails?
					if(  gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1)  )!=NULL) {
						// something on top (monorail or power lines)
						return false;
					}

				}
				else {
					// border but not corner: near a road if possible
					if (!next_to_road  &&  (0 <= x  ||  x < b-1)  &&  (0 <= y  ||  y < h-1)) {
						next_to_road = gr->hat_weg(road_wt);
					}
					// do not build on an existing factory
					if( is_factory_at(k.x, k.y)  ) {
						return false;
					}
				}
			}
		}
		return next_to_road;
	}
};


stringhashtable_tpl<const fabrik_besch_t *> fabrikbauer_t::table;


/**
 * Compares factory descriptors by their name.
 * Used in get_random_consumer().
 * @return true, if @p a < @p b, false otherwise.
 */
static bool compare_fabrik_besch(const fabrik_besch_t* a, const fabrik_besch_t* b)
{
	const int diff = strcmp( a->get_name(), b->get_name() );
	return diff < 0;
}


// returns a random consumer
const fabrik_besch_t *fabrikbauer_t::get_random_consumer(bool electric, climate_bits cl, uint16 timeline )
{
	// get a random city factory
	weighted_vector_tpl<const fabrik_besch_t *> consumer;

	FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& i, table) {
		fabrik_besch_t const* const current = i.value;
		// only insert end consumers
		if (  current->is_consumer_only()  &&
			current->get_haus()->is_allowed_climate_bits(cl)  &&
			(electric ^ !current->is_electricity_producer())  &&
			current->get_haus()->is_available(timeline)  ) {
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
DBG_DEBUG("fabrikbauer_t::register_besch()","Correction for old factory: Increase production from %i by %i",p&0x7FFF,k.x*k.y);
	}
	if(  const fabrik_besch_t *old_besch = table.remove(besch->get_name())  ) {
		dbg->warning( "fabrikbauer_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		delete old_besch;
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
		// check for crossconnects
		for(  int j=0;  j < current->get_lieferanten();  j++  ) {
			weighted_vector_tpl<const fabrik_besch_t *>producer;
			finde_hersteller( producer, current->get_lieferant(j)->get_ware(), 0 );
			if(  producer.is_contained(current)  ) {
				// must not happen else
				dbg->fatal( "fabrikbauer_t::baue_hierarchie()", "Factory %s output %s cannot be its own input!", i.key, current->get_lieferant(j)->get_ware()->get_name() );
			}
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

	// iterate over all factories and check if they produce this good...
	FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& t, table) {
		fabrik_besch_t const* const tmp = t.value;
		for (uint i = 0; i < tmp->get_produkte(); i++) {
			const fabrik_produkt_besch_t *produkt = tmp->get_produkt(i);
			if(  produkt->get_ware()==ware  &&  tmp->get_gewichtung()>0  &&  tmp->get_haus()->is_available(timeline)  ) {
				anzahl++;
			}
		}
	}
DBG_MESSAGE("fabrikbauer_t::finde_anzahl_hersteller()","%i producer for good '%s' found.", anzahl, translator::translate(ware->get_name()));
	return anzahl;
}


/*
 * Finds a random producer producing @p ware.
 * @param timeline the current time(months)
 */
void fabrikbauer_t::finde_hersteller(weighted_vector_tpl<const fabrik_besch_t *> &producer, const ware_besch_t *ware, uint16 timeline )
{
	// find all producers
	producer.clear();
	FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& t, table) {
		fabrik_besch_t const* const tmp = t.value;
		if (  tmp->get_gewichtung()>0  &&  tmp->get_haus()->is_available(timeline)  ) {
			for(  uint i=0; i<tmp->get_produkte();  i++  ) {
				const fabrik_produkt_besch_t *produkt = tmp->get_produkt(i);
				if(  produkt->get_ware()==ware  ) {
					producer.insert_unique_ordered(tmp, tmp->get_gewichtung(), compare_fabrik_besch);
					break;
				}
			}
		}
	}

	// no producer installed?
	if(  producer.empty()  ) {
		dbg->error("fabrikbauer_t::finde_hersteller()", "no producer for good '%s' was found.", translator::translate(ware->get_name()));
	}
	DBG_MESSAGE("fabrikbauer_t::finde_hersteller()", "%i producer for good '%s' found.", producer.get_count(), translator::translate(ware->get_name()) );
}


bool fabrikbauer_t::ist_bauplatz(koord pos, koord groesse, bool water, bool is_fabrik, climate_bits cl)
{
	// check for water (no shore in sight!)
	if(water) {
		for(int y=0;y<groesse.y;y++) {
			for(int x=0;x<groesse.x;x++) {
				const grund_t *gr=welt->lookup_kartenboden(pos+koord(x,y));
				if(gr==NULL  ||  !gr->ist_wasser()  ||  gr->get_grund_hang()!=hang_t::flach) {
					return false;
				}
			}
		}
	}
	else {
		// check on land
		if (!welt->square_is_free(pos, groesse.x, groesse.y, NULL, cl)) {
			return false;
		}
	}
	// check for existing factories
	if (is_fabrik) {
		for(int y=0;y<groesse.y;y++) {
			for(int x=0;x<groesse.x;x++) {
				if (is_factory_at(pos.x + x, pos.y + y)){
					return false;
				}
			}
		}
	}
	return true;
}


koord3d fabrikbauer_t::finde_zufallsbauplatz( koord pos, const int radius, koord groesse, bool wasser, const haus_besch_t *besch, bool ignore_climates, uint32 max_iterations )
{
	bool is_fabrik = besch->get_utyp()==haus_besch_t::fabrik;
	if(wasser) {
		// to ensure at least 3x3 water around (maybe this should be the station catchment area+1?)
		groesse += koord(6,6);
	}

	climate_bits climates = !ignore_climates ? besch->get_allowed_climate_bits() : ALL_CLIMATES;

	uint32 diam   = 2*radius + 1;
	uint32 size   = diam * diam;
	uint32 index  = simrand(size);
	koord k;

	max_iterations = min( size/(groesse.x*groesse.y)+1, max_iterations );
	const uint32 a = diam+1;
	const uint32 c = 37; // very unlikely to have this as a factor in somewhere ...

	// in order to stop on the first occurence, one has to iterate over all tiles in a reproducable but random enough manner
	for(  uint32 i = 0;  i<max_iterations; i++,  index = (a*index+c) % size  ) {

		// so it is guaranteed that the iteration hits all tiles and does not repeat itself
		k = koord( pos.x - radius + (index % diam), pos.y - radius + (index / diam) );

		// check place (it will actually check an grosse.x/y size rectangle, so we can iterate over less tiles)
		if(  fabrikbauer_t::ist_bauplatz(k, groesse, wasser, is_fabrik, climates)  ) {
			// then accept first hit
			goto finish;
		}
	}
	// nothing found
	return koord3d::invalid;

finish:
	if(wasser) {
		// take care of offset
		return welt->lookup_kartenboden(k+koord(3, 3))->get_pos();
	}
	return welt->lookup_kartenboden(k)->get_pos();
}


// Create a certain number of tourist attractions
void fabrikbauer_t::verteile_tourist(int max_number)
{
	// current number of tourist attractions constructed
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
		koord3d	pos=koord3d( koord::koord_random(welt->get_size().x,welt->get_size().y),1);
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),true,(climate)simrand((int)arctic_climate+1));

		// no attractions for that climate or too new
		if(attraction==NULL  ||  (welt->use_timeline()  &&  attraction->get_intro_year_month()>welt->get_current_month()) ) {
			continue;
		}

		int	rotation=simrand(attraction->get_all_layouts()-1);
		pos = finde_zufallsbauplatz(pos.get_2d(), 20, attraction->get_groesse(rotation),false,attraction,false,0x0FFFFFFF);	// so far -> land only
		if(welt->lookup(pos)) {
			// space found, build attraction
			hausbauer_t::baue(welt->get_spieler(1), pos, rotation, attraction);
			current_number ++;
			retrys = max_number*4;
		}

	}
	// update an open map
	reliefkarte_t::get_karte()->calc_map_size();
}

/**
 * Compares cities by their distance to an origin.
 */
class RelativeDistanceOrdering
{
private:
	const koord m_origin;

public:
	RelativeDistanceOrdering(const koord& origin)
		: m_origin(origin)
	{ /* nothing */ }

	/// @returns true if @p a is closer to the origin than @p b, otherwise false.
	bool operator()(const stadt_t *a, const stadt_t *b) const
	{
		return koord_distance(m_origin, a->get_pos()) < koord_distance(m_origin, b->get_pos());
	}
};


/*
 * Builds a single new factory.
 */
fabrik_t* fabrikbauer_t::baue_fabrik(koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d pos, spieler_t* spieler)
{
	fabrik_t * fab = new fabrik_t(pos, spieler, info, initial_prod_base);

	// now build factory
	fab->baue(rotate, true /*add fields*/, initial_prod_base != -1 /* force initial prodbase ? */);
	welt->add_fab(fab);
	add_factory_to_fab_map(welt, fab);

	if(parent) {
		fab->add_lieferziel(parent->get_2d());
	}

	// make all water station
	if(info->get_platzierung() == fabrik_besch_t::Wasser) {
		const haus_besch_t *besch = info->get_haus();
		koord dim = besch->get_groesse(rotate);

		// create water halt
		halthandle_t halt = haltestelle_t::create(pos.get_2d(), welt->get_spieler(1));
		if(halt.is_bound()) {

			// add all other tiles of the factory to the halt
			for(  int x=pos.x;  x<pos.x+dim.x;  x++  ) {
				for(  int y=pos.y;  y<pos.y+dim.y;  y++  ) {
					if(welt->is_within_limits(x,y)) {
						grund_t *gr = welt->lookup_kartenboden(x,y);
						// build the halt on factory tiles on open water only,
						// since the halt can't be removed otherwise
						if(gr->ist_wasser()  &&  !(gr->hat_weg(water_wt))  &&  gr->find<gebaeude_t>()) {
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
		// connect factory to stations
		// search for nearby stations and connect factory to them
		koord dim = info->get_haus()->get_groesse(rotate);

		for(  int x=pos.x;  x<pos.x+dim.x;  x++  ) {
			for(  int y=pos.y;  y<pos.y+dim.y;  y++  ) {
				const planquadrat_t *plan = welt->access(x,y);
				const halthandle_t *halt_list = plan->get_haltlist();
				for(  unsigned h=0;  h<plan->get_haltlist_count();  h++  ) {
					halt_list[h]->verbinde_fabriken();
				}
			}
		}
	}

	// add passenger to pax>0, (so no suicide diver at the fish swarm)
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


// Check if we have to rotate the factories before building this tree
bool fabrikbauer_t::can_factory_tree_rotate( const fabrik_besch_t *besch )
{
	// we are finished: we cannot rotate
	if(!besch->get_haus()->can_rotate()) {
		return false;
	}

	// now check all suppliers if they can rotate
	for(  int i=0;  i<besch->get_lieferanten();  i++   ) {

		const ware_besch_t *ware = besch->get_lieferant(i)->get_ware();

		// unfortunately, for every for iteration we have to check all factories ...
		FOR(stringhashtable_tpl<fabrik_besch_t const*>, const& t, table) {
			fabrik_besch_t const* const tmp = t.value;
			// now check if we produce this good...
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


/*
 * Builds a new full chain of factories. Precondition before calling this function:
 * @p pos is suitable for factory construction and number of chains
 * is the maximum number of good types for which suppliers chains are built
 * (meaning there are no unfinished factory chains).
 */
int fabrikbauer_t::baue_hierarchie(koord3d* parent, const fabrik_besch_t* info, sint32 initial_prod_base, int rotate, koord3d* pos, spieler_t* sp, int number_of_chains )
{
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

	// if a factory is not rotate-able, rotate the world until we can save it
	if(welt->cannot_save()  &&  parent==NULL  &&  !can_factory_tree_rotate(info)  ) {
		org_rotation = welt->get_settings().get_rotation();
		for(  int i=0;  i<3  &&  welt->cannot_save();  i++  ) {
			pos->rotate90( welt->get_size().y-info->get_haus()->get_h(rotate) );
			welt->rotate90();
		}
		assert( !welt->cannot_save() );
	}

	// in town we need a different place search
	if (info->get_platzierung() == fabrik_besch_t::Stadt) {

		koord size=info->get_haus()->get_groesse(0);

		// build consumer (factory) in town
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
#endif

		/* C:
		 * A building site, as near as possible to the city hall.
		 *  If several final factories land in one city, they are often
		 * often hidden behind a row of houses, cut off from roads.
		 */
#if 0
		k = bauplatz_sucher_t(welt).suche_platz(city->get_pos(), land_bau.dim.x, land_bau.dim.y, info->get_haus()->get_allowed_climate_bits(), &is_rotate);
#endif

		if(k != koord::invalid) {
			*pos = welt->lookup_kartenboden(k)->get_pos();
		}
		else {
			return 0;
		}
	}

	DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Construction of %s at (%i,%i).",info->get_name(),pos->x,pos->y);
	INT_CHECK("fabrikbauer 594");

	const fabrik_t *our_fab=baue_fabrik(parent, info, initial_prod_base, rotate, *pos, sp);

	INT_CHECK("fabrikbauer 596");

	// now build supply chains for all products
	for(int i=0; i<info->get_lieferanten()  &&  i<number_of_chains; i++) {
		n += baue_link_hierarchie( our_fab, info, i, sp);
	}

	// everything built -> update map if needed
	if(parent==NULL) {
		DBG_MESSAGE("fabrikbauer_t::baue_hierarchie()","update karte");

		// update the map if needed
		reliefkarte_t::get_karte()->calc_map_size();

		INT_CHECK( "fabrikbauer 730" );

		// must rotate back?
		if(org_rotation>=0) {
			for (int i = 0; i < 4 && welt->get_settings().get_rotation() != org_rotation; ++i) {
				pos->rotate90( welt->get_size().y-1 );
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
	/* first we try to connect to existing factories and will do some
	 * cross-connect (if wanted)
	 * We must take care to add capacity for cross-connected factories!
	 */
	const fabrik_lieferant_besch_t *lieferant = info->get_lieferant(lieferant_nr);
	const ware_besch_t *ware = lieferant->get_ware();
	const int anzahl_hersteller=finde_anzahl_hersteller( ware, welt->get_timeline_year_month() );

	if(anzahl_hersteller==0) {
		dbg->error("fabrikbauer_t::baue_hierarchie()","No producer for %s found, chain incomplete!",ware->get_name() );
		return 0;
	}

	// how much do we need?
	sint32 verbrauch = our_fab->get_base_production()*lieferant->get_verbrauch();

	slist_tpl<fabs_to_crossconnect_t> factories_to_correct;
	slist_tpl<fabrik_t *> new_factories;	      // since the cross-correction must be done later
	slist_tpl<fabrik_t *> crossconnected_supplier;	// also done after the construction of new chains

	int lcount = lieferant->get_anzahl();
	int lfound = 0;	// number of found producers

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","lieferanten %i, lcount %i (need %i of %s)",info->get_lieferanten(),lcount,verbrauch,ware->get_name());

	// Hajo: search if there already is one or two (cross-connect everything if possible)
	FOR(slist_tpl<fabrik_t*>, const fab, welt->get_fab_list()) {
		// Try to find matching factories for this consumption, but don't find more than two times number of factories requested.
		if ((lcount != 0 || verbrauch <= 0) && lcount < lfound + 1) break;

		// connect to an existing one if this is a producer
		if(fab->vorrat_an(ware) > -1) {

			// for sources (oil fields, forests ... ) prefer those with a smaller distance
			const unsigned distance = koord_distance(fab->get_pos(),our_fab->get_pos());

			if(distance>6) {//  &&  distance < simrand(welt->get_size().x+welt->get_size().y)) {
				// ok, this would match
				// but can she supply enough?

				// now guess how much this factory can supply
				const fabrik_besch_t* const fb = fab->get_besch();
				for (uint gg = 0; gg < fb->get_produkte(); gg++) {
					if (fb->get_produkt(gg)->get_ware() == ware && fab->get_lieferziele().get_count() < 10) { // does not make sense to split into more ...
						sint32 production_left = fab->get_base_production() * fb->get_produkt(gg)->get_faktor();
						const vector_tpl <koord> & lieferziele = fab->get_lieferziele();

						// decrease remaining production by supplier demand
						FOR(vector_tpl<koord>, const& i, lieferziele) {
							if (production_left <= 0) break;
							fabrik_t* const zfab = fabrik_t::get_fab(i);
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
									fabrik_t* zfab = fabrik_t::get_fab(t);
									slist_tpl<fabs_to_crossconnect_t>::iterator i = std::find(factories_to_correct.begin(), factories_to_correct.end(), fabs_to_crossconnect_t(zfab, 0));
									if (i == factories_to_correct.end()) {
										factories_to_correct.append(fabs_to_crossconnect_t(zfab, 1));
									}
									else {
										i->demand += 1;
									}
								}
								// the needed production to be built does not change!
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
	 */
	weighted_vector_tpl<const fabrik_besch_t *>producer;
	finde_hersteller( producer, ware, welt->get_timeline_year_month() );
	if(  producer.empty()  ) {
		// can happen with timeline
		if(!info->is_consumer_only()) {
			dbg->error( "fabrikbauer_t::baue_hierarchie()", "no producer for %s!", ware->get_name() );
			return 0;
		}
		else {
			// only consumer: Will do with partly covered chains
			dbg->error( "fabrikbauer_t::baue_hierarchie()", "no producer for %s!", ware->get_name() );
		}
	}

	bool ignore_climates = false;	// ignore climates after some retrys
	int retry=25;	// and not more than 25 (happens mostly in towns)
	while(  (lcount>lfound  ||  lcount==0)  &&  verbrauch>0  &&  retry>0  ) {

		const fabrik_besch_t *hersteller = pick_any_weighted( producer );

		int rotate = simrand(hersteller->get_haus()->get_all_layouts()-1);
		koord3d parent_pos = our_fab->get_pos();
		// ignore climates after 40 tries

		INT_CHECK("fabrikbauer 697");

		koord3d k = finde_zufallsbauplatz( our_fab->get_pos().get_2d(), DISTANCE, hersteller->get_haus()->get_groesse(rotate),hersteller->get_platzierung()==fabrik_besch_t::Wasser, hersteller->get_haus(), ignore_climates, 20000 );
		if(  k == koord3d::invalid  ) {
			// this factory cannot buuild in the desired vincinity
			producer.remove( hersteller );
			if(  producer.empty()  ) {
				if(  ignore_climates  ) {
					// absolutely no place to construct something here
					break;
				}
				finde_hersteller( producer, ware, welt->get_timeline_year_month() );
				ignore_climates = true;
			}
			continue;
		}

		INT_CHECK("fabrikbauer 697");

DBG_MESSAGE("fabrikbauer_t::baue_hierarchie","Try to built lieferant %s at (%i,%i) r=%i for %s.",hersteller->get_name(),k.x,k.y,rotate,info->get_name());
		n += baue_hierarchie(&parent_pos, hersteller, -1 /*random prodbase */, rotate, &k, sp, 10000 );
		lfound ++;

		INT_CHECK( "fabrikbauer 702" );

		// now subtract current supplier
		fabrik_t *fab = fabrik_t::get_fab(k.get_2d() );
		if(fab==NULL) {
DBG_MESSAGE( "fabrikbauer_t::baue_hierarchie", "Failed to build at %s", k.get_str() );
			retry --;
			continue;
		}
		new_factories.append(fab);

		// connect new supplier to us
		const fabrik_besch_t* const fb = fab->get_besch();
		for (uint gg = 0; gg < fab->get_besch()->get_produkte(); gg++) {
			if (fb->get_produkt(gg)->get_ware() == ware) {
				sint32 produktion = fab->get_base_production() * fb->get_produkt(gg)->get_faktor();
				// the take care of how much this factory could supply
				verbrauch -= produktion;
				DBG_MESSAGE("fabrikbauer_t::baue_hierarchie", "new supplier %s can supply approx %i of %s to us", fb->get_name(), produktion, ware->get_name());
				break;
			}
		}
	}

	// now we add us to all cross-connected factories
	FOR(slist_tpl<fabrik_t*>, const i, crossconnected_supplier) {
		i->add_lieferziel(our_fab->get_pos().get_2d());
	}

	/* now the cross-connect part:
	 * connect also the factories we stole from before ...
	 */
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


/*
 * This function is called whenever it is time for industry growth.
 * If there is still a pending consumer, this method will first complete another chain for it.
 * If not, it will decide to either build a power station (if power is needed)
 * or build a new consumer near the indicated position.
 * @return number of factories built
 */
int fabrikbauer_t::increase_industry_density( bool tell_me )
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
					fabrik_besch_t const* const fb = fabrik_t::get_fab(j)->get_besch();
					for (uint32 k = 0; k < fb->get_produkte(); k++) {
						if (fb->get_produkt(k)->get_ware() == w) {
							last_built_consumer_ware = i+1;
							goto next_ware_check;
						}
					}
				}
next_ware_check:
				// ok, found something, test next
				;
			}
		}
	}

	// first: do we have to continue unfinished factory chains?
	if(last_built_consumer  &&  last_built_consumer_ware < last_built_consumer->get_besch()->get_lieferanten()) {

		int org_rotation = -1;
		// rotate until we can save it if one of the factories is non-rotate-able ...
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

		// only return if successful
		if(  last_built_consumer->get_suppliers().get_count() > last_suppliers  ) {
			DBG_MESSAGE( "fabrikbauer_t::increase_industry_density()", "added ware %i to factory %s", last_built_consumer_ware, last_built_consumer->get_name() );
			// tell the player
			if(tell_me) {
				stadt_t *s = welt->suche_naechste_stadt( last_built_consumer->get_pos().get_2d() );
				const char *stadt_name = s ? s->get_name() : translator::translate("nowhere");
				cbuffer_t buf;
				buf.printf( translator::translate("Factory chain extended\nfor %s near\n%s built with\n%i factories."), translator::translate(last_built_consumer->get_name()), stadt_name, nr );
				welt->get_message()->add_message(buf, last_built_consumer->get_pos().get_2d(), message_t::industry, CITY_KI, last_built_consumer->get_besch()->get_haus()->get_tile(0)->get_hintergrund(0, 0, 0));
			}
			reliefkarte_t::get_karte()->calc_map();
			return nr;
		}
	}

	// ok, no chains to finish, thus we must start anew

	// first determine electric supply and demand
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
	int    no_electric = promille > welt->get_settings().get_electric_promille(); // =1 -> build normal factories only
	DBG_MESSAGE( "fabrikbauer_t::increase_industry_density()", "production of electricity/total production is %i/%i (%i o/oo)", electric_supply, electric_demand, promille );

	while(  no_electric<2  ) {
		bool ignore_climates = false;

		for(int retries=20;  retries>0;  retries--  ) {
			const fabrik_besch_t *fab=get_random_consumer( no_electric==0, ALL_CLIMATES, welt->get_timeline_year_month() );
			if(fab) {
				const bool in_city = fab->get_platzierung() == fabrik_besch_t::Stadt;
				if (in_city && welt->get_staedte().empty()) {
					// we cannot build this factory here
					continue;
				}
				koord3d pos;
				int rotation = simrand( fab->get_haus()->get_all_layouts() );
				if(!in_city) {
					// find somewhere on the map
					pos = finde_zufallsbauplatz( koord(welt->get_size().x/2,welt->get_size().y/2), welt->get_size_max()/2, fab->get_haus()->get_groesse(rotation),fab->get_platzierung()==fabrik_besch_t::Wasser,fab->get_haus(),ignore_climates,10000);
				}
				else {
					// or within the city limit
					const stadt_t *city = pick_any_weighted(welt->get_staedte());
					koord diff = city->get_rechtsunten()-city->get_linksoben();
					pos = finde_zufallsbauplatz( city->get_center(), max(diff.x,diff.y)/2, fab->get_haus()->get_groesse(rotation),fab->get_platzierung()==fabrik_besch_t::Wasser,fab->get_haus(),ignore_climates, 1000);
				}
				if(welt->lookup(pos)) {
					// Space found...
					nr += baue_hierarchie(NULL, fab, -1 /* random prodbase */, rotation, &pos, welt->get_spieler(1), 1 );
					if(nr>0) {
						fabrik_t *our_fab = fabrik_t::get_fab( pos.get_2d() );
						reliefkarte_t::get_karte()->calc_map_size();
						// tell the player
						if(tell_me) {
							stadt_t *s = our_fab->get_target_cities()[0];
							const char *stadt_name = s ? s->get_name() : translator::translate("nowhere");
							cbuffer_t buf;
							buf.printf( translator::translate("New factory chain\nfor %s near\n%s built with\n%i factories."), translator::translate(our_fab->get_name()), stadt_name, nr );
							welt->get_message()->add_message(buf, pos.get_2d(), message_t::industry, CITY_KI, our_fab->get_besch()->get_haus()->get_tile(0)->get_hintergrund(0, 0, 0));
						}
						return nr;
					}
				}
				else if(  retries==1  &&  !ignore_climates  ) {
					// from now on, we will ignore climates to avoid broken chains
					ignore_climates = true;
					retries = 20;
				}
			}
		}
		// try building normal factories if building power plants was not successful
		no_electric++;
	}

	// we should not reach here, because it means neither land nor city industries exist ...
	dbg->warning( "fabrikbauer_t::increase_industry_density()", "No suitable city industry found => pak missing something?" );
	return 0;
}
