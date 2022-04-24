/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include "../simdebug.h"
#include "fabrikbauer.h"
#include "hausbauer.h"
#include "../world/simworld.h"
#include "../simintr.h"
#include "../simfab.h"
#include "../simmesg.h"
#include "../utils/simrandom.h"
#include "../world/simcity.h"
#include "../simhalt.h"
#include "../player/simplay.h"

#include "../ground/grund.h"

#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../network/pakset_info.h"
#include "../dataobj/translator.h"
#include "../dataobj/pakset_manager.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/array_tpl.h"
#include "../world/building_placefinder.h"

#include "../utils/cbuffer.h"

#include "../descriptor/objversion.h"

#include "../gui/minimap.h" // to update map after construction of new industry


karte_ptr_t factory_builder_t::welt;

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
	building_desc_t const& bdsc  = *fab->get_desc()->get_building();
	uint8        const  rotate  = fab->get_rotate();
	sint16       const  start_y = max(0, pos.y - spacing);
	sint16       const  start_x = max(0, pos.x - spacing);
	sint16       const  end_y   = min(welt->get_size().y - 1, pos.y + bdsc.get_y(rotate) + spacing);
	sint16       const  end_x   = min(welt->get_size().x - 1, pos.x + bdsc.get_x(rotate) + spacing);
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
	for(fabrik_t* const f : welt->get_fab_list()) {
		add_factory_to_fab_map(welt, f);
	}
	if(  welt->get_settings().get_max_factory_spacing_percent() > 0  ) {
		DISTANCE = (welt->get_size_max() * welt->get_settings().get_max_factory_spacing_percent()) / 100l;
	}
	else {
		DISTANCE = welt->get_settings().get_max_factory_spacing();
	}
}


/**
 * @param x,y world position, needs to be valid coordinates
 * @returns true, if factory coordinate
 */
inline bool is_factory_at(sint16 x, sint16 y)
{
	uint32 idx = (fab_map_w*y)+(x/8);
	return idx < fab_map.get_count()  &&  (fab_map[idx]&(1<<(x%8)))!=0;
}


void factory_builder_t::new_world()
{
	init_fab_map( welt );
}


/**
 * Searches for a suitable building site using suche_platz().
 * The site is chosen so that there is a street next to the building site.
 */
class factory_site_searcher_t: public building_placefinder_t  {

	factory_desc_t::site_t site;
public:
	factory_site_searcher_t(karte_t* welt, factory_desc_t::site_t site_) : building_placefinder_t(welt), site(site_) {}

	bool is_area_ok(koord pos, sint16 w, sint16 h, climate_bits cl) const OVERRIDE
	{
		if(  !building_placefinder_t::is_area_ok(pos, w, h, cl)  ) {
			return false;
		}
		uint16 mincond = 0;
		uint16 condmet = 0;
		switch(site) {
			case factory_desc_t::forest:
				mincond = w*h+w+h; // at least w*h+w+h trees, i.e. few tiles with more than one tree
				break;

			case factory_desc_t::shore:
			case factory_desc_t::river:
			case factory_desc_t::City:
			default:
				mincond = 1;
		}

		// needs to run one tile wider than the factory on all sides
		for (sint16 y = -1;  y <= h; y++) {
			for (sint16 x = -1; x <= w; x++) {
				koord k(pos + koord(x,y));
				grund_t *gr = welt->lookup_kartenboden(k);
				if (!gr) {
					return false;
				}
				// do not build on an existing factory
				if( is_factory_at(k.x, k.y)  ) {
					return false;
				}
				if(  0 <= x  &&  x < w  &&  0 <= y  &&  y < h  ) {
					// actual factorz tile: is there something top like elevated monorails?
					if(  gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1)  )!=NULL) {
						// something on top (monorail or power lines)
						return false;
					}
					// check for trees unless the map was generated without trees
					if(  site==factory_desc_t::forest  &&  welt->get_settings().get_tree_distribution()!=settings_t::TREE_DIST_NONE  &&  condmet < mincond  ) {
						for(uint8 i=0; i< gr->get_top(); i++) {
							if (gr->obj_bei(i)->get_typ() == obj_t::baum) {
								condmet++;
							}
						}
					}
				}
				else {
					// border tile: check for road, shore, river
					if(  condmet < mincond  &&  (-1==x  ||  x==w)  &&  (-1==y  ||  y==h)  ) {
						switch (site) {
							case factory_desc_t::City:
								condmet += gr->hat_weg(road_wt);
								break;
							case factory_desc_t::shore:
								condmet += welt->get_climate(k)==water_climate;
								break;
							case factory_desc_t::river:
							if(  welt->get_settings().get_river_number() >0  ) {
								weg_t* river = gr->get_weg(water_wt);
								if (river  &&  river->get_desc()->get_styp()==type_river) {
									condmet++;
									DBG_DEBUG("factory_site_searcher_t::is_area_ok()", "Found river near %s", pos.get_str());
								}
								break;
							}
							else {
								// always succeeds on maps without river ...
								condmet++;
								break;
							}
							default: ;
						}
					}
				}
			}
		}
		if(  site==factory_desc_t::forest  &&  condmet>=3  ) {
			dbg->message("", "found %d at %s", condmet, pos.get_str() );
		}
		return condmet >= mincond;
	}
};


stringhashtable_tpl<const factory_desc_t *> factory_builder_t::desc_table;


/**
 * Compares factory descriptors by their name.
 * Used in get_random_consumer().
 * @return true, if @p a < @p b, false otherwise.
 */
static bool compare_fabrik_desc(const factory_desc_t* a, const factory_desc_t* b)
{
	const int diff = strcmp( a->get_name(), b->get_name() );
	return diff < 0;
}


// returns a random consumer
const factory_desc_t *factory_builder_t::get_random_consumer(bool electric, climate_bits cl, uint16 timeline )
{
	// get a random city factory
	weighted_vector_tpl<const factory_desc_t *> consumer;

	for(auto const& i : desc_table) {
		factory_desc_t const* const current = i.value;
		// only insert end consumers
		if (  current->is_consumer_only()  &&
			current->get_building()->is_allowed_climate_bits(cl)  &&
			(electric ^ !current->is_electricity_producer())  &&
			current->get_building()->is_available(timeline)  ) {
				consumer.insert_unique_ordered(current, current->get_distribution_weight(), compare_fabrik_desc);
		}
	}
	// no consumer installed?
	if (consumer.empty()) {
DBG_MESSAGE("factory_builder_t::get_random_consumer()","No suitable consumer found");
		return NULL;
	}
	// now find a random one
	factory_desc_t const* const fd = pick_any_weighted(consumer);
	DBG_MESSAGE("factory_builder_t::get_random_consumer()", "consumer %s found.", fd->get_name());
	return fd;
}


const factory_desc_t *factory_builder_t::get_desc(const char *factory_name)
{
	return desc_table.get(factory_name);
}


void factory_builder_t::register_desc(factory_desc_t *desc)
{
	uint16 p=desc->get_productivity();
	if(p&0x8000) {
		koord k=desc->get_building()->get_size();

		// to be compatible with old factories, since new code only steps once per factory, not per tile
		desc->set_productivity( (p&0x7FFF)*k.x*k.y );
DBG_DEBUG("factory_builder_t::register_desc()","Correction for old factory: Increase production from %i by %i",p&0x7FFF,k.x*k.y);
	}
	if(  const factory_desc_t *old_desc = desc_table.remove(desc->get_name())  ) {
		pakset_manager_t::doubled( "factory", desc->get_name() );
		delete old_desc;
	}
	desc_table.put(desc->get_name(), desc);
}


bool factory_builder_t::successfully_loaded()
{
	for(auto const& i : desc_table) {
		factory_desc_t const* const current = i.value;
		if(  field_group_desc_t * fg = const_cast<field_group_desc_t *>(current->get_field_group())  ) {
			// initialize weighted vector for the field class indices
			fg->init_field_class_indices();
		}
		// check for crossconnects
		for(  int j=0;  j < current->get_supplier_count();  j++  ) {
			weighted_vector_tpl<const factory_desc_t *>producer;
			find_producer( producer, current->get_supplier(j)->get_input_type(), 0 );
			if(  producer.is_contained(current)  ) {
				// must not happen else
				dbg->fatal( "factory_builder_t::successfully_loaded()", "Factory %s output %s cannot be its own input!", i.key, current->get_supplier(j)->get_input_type()->get_name() );
			}
		}
		checksum_t *chk = new checksum_t();
		current->calc_checksum(chk);
		pakset_info_t::append(current->get_name(), obj_factory, chk);
	}
	return true;
}


int factory_builder_t::count_producers(const goods_desc_t *ware, uint16 timeline)
{
	int count=0;

	// iterate over all factories and check if they produce this good...
	for(auto const& t : desc_table) {
		factory_desc_t const* const tmp = t.value;
		for (uint i = 0; i < tmp->get_product_count(); i++) {
			const factory_product_desc_t *product = tmp->get_product(i);
			if(  product->get_output_type()==ware  &&  tmp->get_distribution_weight()>0  &&  tmp->get_building()->is_available(timeline)  ) {
				count++;
			}
		}
	}
DBG_MESSAGE("factory_builder_t::count_producers()","%i producer for good '%s' found.", count, translator::translate(ware->get_name()));
	return count;
}


/*
 * Finds a random producer producing @p ware.
 * @param timeline the current time(months)
 */
void factory_builder_t::find_producer(weighted_vector_tpl<const factory_desc_t *> &producer, const goods_desc_t *ware, uint16 timeline )
{
	// find all producers
	producer.clear();
	for(auto const& t : desc_table) {
		factory_desc_t const* const tmp = t.value;
		if (  tmp->get_distribution_weight()>0  &&  tmp->get_building()->is_available(timeline)  ) {
			for(  uint i=0; i<tmp->get_product_count();  i++  ) {
				const factory_product_desc_t *product = tmp->get_product(i);
				if(  product->get_output_type()==ware  ) {
					producer.insert_unique_ordered(tmp, tmp->get_distribution_weight(), compare_fabrik_desc);
					break;
				}
			}
		}
	}

	// no producer installed?
	if(  producer.empty()  ) {
		dbg->error("factory_builder_t::find_producer()", "no producer for good '%s' was found.", translator::translate(ware->get_name()));
	}
	DBG_MESSAGE("factory_builder_t::find_producer()", "%i producer for good '%s' found.", producer.get_count(), translator::translate(ware->get_name()) );
}


bool factory_builder_t::check_construction_site(koord pos, koord size, factory_desc_t::site_t site, bool is_factory, climate_bits cl)
{
	// check for water (no shore in sight!)
	if(site==factory_desc_t::Water) {
		for(int y=0;y<size.y;y++) {
			for(int x=0;x<size.x;x++) {
				const grund_t *gr=welt->lookup_kartenboden(pos+koord(x,y));
				if(gr==NULL  ||  !gr->is_water()  ||  gr->get_grund_hang()!=slope_t::flat) {
					return false;
				}
			}
		}
	}
	else {
		if (is_factory  &&  site!=factory_desc_t::Land) {
			return factory_site_searcher_t(welt, site).is_area_ok(pos, size.x, size.y, cl);
		}
		else {
			// check on land
			// do not build too close or on an existing factory
			if( !welt->is_within_limits(pos)  || is_factory_at(pos.x, pos.y)  ) {
				return false;
			}
			return welt->square_is_free(pos, size.x, size.y, NULL, cl);
		}
	}
	return true;
}


koord3d factory_builder_t::find_random_construction_site(koord pos, int radius, koord size, factory_desc_t::site_t site, const building_desc_t *desc, bool ignore_climates, uint32 max_iterations)
{
	bool is_factory = desc->get_type()==building_desc_t::factory;
	bool wasser = site == factory_desc_t::Water;

	if(wasser) {
		// to ensure at least 3x3 water around (maybe this should be the station catchment area+1?)
		size += koord(6,6);
	}

	climate_bits climates = !ignore_climates ? desc->get_allowed_climate_bits() : ALL_CLIMATES;
	if (ignore_climates  &&  site != factory_desc_t::Water) {
		//site = factory_desc_t::Land;
	}

	uint32 diam   = 2*radius + 1;
	uint32 area   = diam * diam;
	uint32 index  = simrand(area);
	koord k;

	max_iterations = min( area/(size.x*size.y)+1, max_iterations );
	const uint32 a = diam+1;
	const uint32 c = 37; // very unlikely to have this as a factor in somewhere ...

	// in order to stop on the first occurence, one has to iterate over all tiles in a reproducable but random enough manner
	for(  uint32 i = 0;  i<max_iterations; i++,  index = (a*index+c) % area  ) {

		// so it is guaranteed that the iteration hits all tiles and does not repeat itself
		k = koord( pos.x - radius + (index % diam), pos.y - radius + (index / diam) );

		// check place (it will actually check an grosse.x/y size rectangle, so we can iterate over less tiles)
		if(  factory_builder_t::check_construction_site(k, size, site, is_factory, climates)  ) {
			// then accept first hit
			if (site != factory_desc_t::Water && site != factory_desc_t::Land) {
				DBG_MESSAGE("factory_builder_t::find_random_construction_site","Found spot for %d at %s / %d\n", site, k.get_str(), max_iterations);
			}
			// we accept first hit
			goto finish;
		}
	}
	// nothing found
	if (site != factory_desc_t::Water  &&  site != factory_desc_t::Land) {
		DBG_MESSAGE("factory_builder_t::find_random_construction_site","No spot found for location %d of %s near %s / %d\n", site, desc->get_name(), pos.get_str(), max_iterations);
	}
	return koord3d::invalid;

finish:
	if(wasser) {
		// take care of offset
		return welt->lookup_kartenboden(k+koord(3, 3))->get_pos();
	}
	return welt->lookup_kartenboden(k)->get_pos();
}


// Create a certain number of tourist attractions
void factory_builder_t::distribute_attractions(int max_number)
{
	// current number of tourist attractions constructed
	int current_number=0;

	// select without timeline disappearing dates
	if(hausbauer_t::get_random_attraction(welt->get_timeline_year_month(),true,temperate_climate)==NULL) {
		// nothing at all?
		return;
	}

	// very fast, so we do not bother updating progress bar
	dbg->message("factory_builder_t::distribute_attractions()", "Distributing %i tourist attractions", max_number);

	int retrys = max_number*4;
	while(current_number<max_number  &&  retrys-->0) {
		koord3d pos=koord3d( koord::koord_random(welt->get_size().x,welt->get_size().y),1);
		const building_desc_t *attraction=hausbauer_t::get_random_attraction(welt->get_timeline_year_month(),true,(climate)simrand((int)arctic_climate+1));

		// no attractions for that climate or too new
		if(attraction==NULL  ||  (welt->use_timeline()  &&  attraction->get_intro_year_month()>welt->get_current_month()) ) {
			continue;
		}

		int rotation=simrand(attraction->get_all_layouts()-1);
		pos = find_random_construction_site(pos.get_2d(), 20, attraction->get_size(rotation), factory_desc_t::Land, attraction, false, 0x0FFFFFFF); // so far -> land only
		if(welt->lookup(pos)) {
			// space found, build attraction
			hausbauer_t::build(welt->get_public_player(), pos.get_2d(), rotation, attraction);
			current_number ++;
			retrys = max_number*4;
		}

	}
	// update an open map
	minimap_t::get_instance()->calc_map_size();
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
fabrik_t* factory_builder_t::build_factory(koord3d* parent, const factory_desc_t* info, sint32 initial_prod_base, int rotate, koord3d pos, player_t* owner)
{
	fabrik_t * fab = new fabrik_t(pos, owner, info, initial_prod_base);

	// now build factory
	fab->build(rotate, true /*add fields*/, initial_prod_base != -1 /* force initial prodbase ? */);
	welt->add_fab(fab);
	add_factory_to_fab_map(welt, fab);

	if(parent) {
		fab->add_consumer(parent->get_2d());
	}

	// make all water station
	if(info->get_placement() == factory_desc_t::Water) {
		const building_desc_t *desc = info->get_building();
		koord dim = desc->get_size(rotate);

		// create water halt
		halthandle_t halt = haltestelle_t::create(pos.get_2d(), welt->get_public_player());
		if(halt.is_bound()) {

			// add all other tiles of the factory to the halt
			for(  int x=pos.x;  x<pos.x+dim.x;  x++  ) {
				for(  int y=pos.y;  y<pos.y+dim.y;  y++  ) {
					if(welt->is_within_limits(x,y)) {
						grund_t *gr = welt->lookup_kartenboden(x,y);
						// build the halt on factory tiles on open water only,
						// since the halt can't be removed otherwise
						if(gr->is_water()  &&  !(gr->hat_weg(water_wt))  &&  gr->find<gebaeude_t>()) {
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
		koord dim = info->get_building()->get_size(rotate);

		for(  int x=pos.x;  x<pos.x+dim.x;  x++  ) {
			for(  int y=pos.y;  y<pos.y+dim.y;  y++  ) {
				const planquadrat_t *plan = welt->access(x,y);
				const halthandle_t *halt_list = plan->get_haltlist();

				for(  unsigned h=0;  h<plan->get_haltlist_count();  h++  ) {
					if (halt_list[h]->connect_factory(fab)) {
						if (x != pos.x  ||  y != pos.y) {
							// factory halt list is already correct at pos
							fab->link_halt(halt_list[h]);
						}
					}
					// cannot call halt_list[h]->verbinde_fabriken() here,
					// as it modifies halt_list in its calls to fabrik_t::unlink_halt and fabrik_t::link_halt
				}
			}
		}
	}

	// add passenger to pax>0, (so no suicide diver at the fish swarm)
	if(info->get_pax_level()>0) {
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_cities();
		vector_tpl<stadt_t *>distance_stadt( staedte.get_count() );

		for(stadt_t* const i : staedte) {
			distance_stadt.insert_ordered(i, RelativeDistanceOrdering(fab->get_pos().get_2d()));
		}
		settings_t const& s = welt->get_settings();
		for(stadt_t* const i : distance_stadt) {
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
bool factory_builder_t::can_factory_tree_rotate( const factory_desc_t *desc )
{
	// we are finished: we cannot rotate
	if(!desc->get_building()->can_rotate()) {
		return false;
	}

	// now check all suppliers if they can rotate
	for(  int i=0;  i<desc->get_supplier_count();  i++   ) {

		const goods_desc_t *ware = desc->get_supplier(i)->get_input_type();

		// unfortunately, for every for iteration we have to check all factories ...
		for(auto const& t : desc_table) {
			factory_desc_t const* const tmp = t.value;
			// now check if we produce this good...
			for (uint i = 0; i < tmp->get_product_count(); i++) {
				if(tmp->get_product(i)->get_output_type()==ware  &&  tmp->get_distribution_weight()>0) {

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
int factory_builder_t::build_link(koord3d* parent, const factory_desc_t* info, sint32 initial_prod_base, int rotate, koord3d* pos, player_t* player, int number_of_chains, bool ignore_climates)
{
	int n = 1;
	int org_rotation = -1;

	if(info==NULL) {
		// no industry found
		return 0;
	}

	// no cities at all?
	if (info->get_placement() == factory_desc_t::City  &&  welt->get_cities().empty()) {
		return 0;
	}

	// if a factory is not rotate-able, rotate the world until we can save it
	if(welt->cannot_save()  &&  parent==NULL  &&  !can_factory_tree_rotate(info)  ) {
		org_rotation = welt->get_settings().get_rotation();
		for(  int i=0;  i<3  &&  welt->cannot_save();  i++  ) {
			pos->rotate90( welt->get_size().y-info->get_building()->get_y(rotate) );
			welt->rotate90();
		}
		assert( !welt->cannot_save() );
	}

	// in town we need a different place search
	if (info->get_placement() == factory_desc_t::City) {

		koord size=info->get_building()->get_size(0);

		// build consumer (factory) in town
		stadt_t *city = welt->find_nearest_city(pos->get_2d());

		climate_bits cl = ignore_climates ? ALL_CLIMATES : info->get_building()->get_allowed_climate_bits();

		/* Three variants:
		 * A:
		 * A building site, preferably close to the town hall with a street next to it.
		 * This could be a temporary problem, if a city has no such site and the search
		 * continues to the next city.
		 * Otherwise seems to me the most realistic.
		 */
		bool is_rotate=info->get_building()->get_all_layouts()>1  &&  size.x!=size.y  &&  info->get_building()->can_rotate();
		// first try with standard orientation
		koord k = factory_site_searcher_t(welt, factory_desc_t::City).find_place(city->get_pos(), size.x, size.y, cl );

		// second try: rotated
		koord k1 = koord::invalid;
		if (is_rotate  &&  (k == koord::invalid  ||  simrand(256)<128)) {
			k1 = factory_site_searcher_t(welt, factory_desc_t::City).find_place(city->get_pos(), size.y, size.x, cl );
		}

		rotate = simrand( info->get_building()->get_all_layouts() );
		if (k1 == koord::invalid) {
			if (size.x != size.y) {
				rotate &= 2; // rotation must be even number
			}
		}
		else {
			k = k1;
			rotate |= 1; // rotation must be odd number
		}
		if (!info->get_building()->can_rotate()) {
			rotate = 0;
		}

		INT_CHECK( "fabrikbauer 588" );

		/* B:
		 * Also good.  The final factories stand possibly somewhat outside of the city however not far away.
		 * (does not obey climates though!)
		 */
#if 0
		k = find_random_construction_site(welt, welt->lookup(city->get_pos())->get_boden()->get_pos(), 3, land_bau.dim).get_2d();
#endif

		/* C:
		 * A building site, as near as possible to the city hall.
		 *  If several final factories land in one city, they are often
		 * often hidden behind a row of houses, cut off from roads.
		 */
#if 0
		k = building_placefinder_t(welt).find_place(city->get_pos(), land_bau.dim.x, land_bau.dim.y, info->get_building()->get_allowed_climate_bits(), &is_rotate);
#endif

		if(k != koord::invalid) {
			*pos = welt->lookup_kartenboden(k)->get_pos();
		}
		else {
			return 0;
		}
	}

	DBG_MESSAGE("factory_builder_t::build_link","Construction of %s at (%i,%i).",info->get_name(),pos->x,pos->y);
	INT_CHECK("fabrikbauer 594");

	const fabrik_t *our_fab=build_factory(parent, info, initial_prod_base, rotate, *pos, player);

	INT_CHECK("fabrikbauer 596");

	// now build supply chains for all products
	for(int i=0; i<info->get_supplier_count()  &&  i<number_of_chains; i++) {
		n += build_chain_link( our_fab, info, i, player);
	}

	// everything built -> update map if needed
	if(parent==NULL) {
		DBG_MESSAGE("factory_builder_t::build_link()","update karte");

		// update the map if needed
		minimap_t::get_instance()->calc_map_size();

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


int factory_builder_t::build_chain_link(const fabrik_t* our_fab, const factory_desc_t* info, int supplier_nr, player_t* player)
{
	int n = 0; // number of additional factories
	/* first we try to connect to existing factories and will do some
	 * cross-connect (if wanted)
	 * We must take care to add capacity for cross-connected factories!
	 */
	const factory_supplier_desc_t *supplier = info->get_supplier(supplier_nr);
	const goods_desc_t *ware = supplier->get_input_type();
	const int producer_count=count_producers( ware, welt->get_timeline_year_month() );

	if(producer_count==0) {
		dbg->error("factory_builder_t::build_chain_link()","No producer for %s found, chain incomplete!",ware->get_name() );
		return 0;
	}

	// how much do we need?
	sint32 consumption = our_fab->get_base_production()*supplier->get_consumption();

	slist_tpl<factories_to_crossconnect_t> factories_to_correct;
	slist_tpl<fabrik_t *> new_factories;           // since the cross-correction must be done later
	slist_tpl<fabrik_t *> crossconnected_supplier; // also done after the construction of new chains

	int lcount = supplier->get_supplier_count();
	int lfound = 0; // number of found producers

	DBG_MESSAGE("factory_builder_t::build_chain_link","supplier_count %i, lcount %i (need %i of %s)",info->get_supplier_count(),lcount,consumption,ware->get_name());

	// search if there already is one or two (cross-connect everything if possible)
	for(fabrik_t* const fab : welt->get_fab_list()) {

		// Try to find matching factories for this consumption, but don't find more than two times number of factories requested.
		if (  (lcount != 0  ||  consumption <= 0)  &&  lcount < lfound + 1  )
			break;

		// connect to an existing one if this is a producer
		if(  fab->get_output_stock(ware) > -1  ) {

			const int distance = koord_distance(fab->get_pos(),our_fab->get_pos());
			if(  distance > welt->get_settings().get_min_factory_spacing()  &&  distance <= DISTANCE  ) {
				// ok, this would match but can she supply enough?

				// now guess how much this factory can supply
				const factory_desc_t* const fd = fab->get_desc();
				for (uint gg = 0; gg < fd->get_product_count(); gg++) {
					if (fd->get_product(gg)->get_output_type() == ware && fab->get_consumer().get_count() < 10) { // does not make sense to split into more ...
						sint32 production_left = fab->get_base_production() * fd->get_product(gg)->get_factor();
						const vector_tpl <koord> & consumer = fab->get_consumer();

						// decrease remaining production by supplier demand
						for(koord const& i : consumer) {
							if (production_left <= 0) break;
							fabrik_t* const zfab = fabrik_t::get_fab(i);
							for(int zz=0;  zz<zfab->get_desc()->get_supplier_count();  zz++) {
								if(zfab->get_desc()->get_supplier(zz)->get_input_type()==ware) {
									production_left -= zfab->get_base_production()*zfab->get_desc()->get_supplier(zz)->get_consumption();
									break;
								}
							}
						}

						// here is actually capacity left (or sometimes just connect anyway)!
						if (production_left > 0 || simrand(100) < (uint32)welt->get_settings().get_crossconnect_factor()) {
							if(production_left>0) {
								consumption -= production_left;
								fab->add_consumer(our_fab->get_pos().get_2d());
								DBG_MESSAGE("factory_builder_t::build_chain_link","supplier %s can supply approx %i of %s to us", fd->get_name(), production_left, ware->get_name());
							}
							else {
								/* we steal something; however the total capacity
								 * needed is the same. Therefore, we just keep book
								 * from whose factories from how many we stole */
								crossconnected_supplier.append(fab);
								for(koord const& t : consumer) {
									fabrik_t* zfab = fabrik_t::get_fab(t);
									for(int zz=0;  zz<zfab->get_desc()->get_supplier_count();  zz++) {
										if(zfab->get_desc()->get_supplier(zz)->get_input_type()==ware) {

											slist_tpl<factories_to_crossconnect_t>::iterator i = std::find(factories_to_correct.begin(), factories_to_correct.end(), factories_to_crossconnect_t(zfab, 0));
											if (i == factories_to_correct.end()) {
												factories_to_correct.append(factories_to_crossconnect_t(zfab, 1));
											}
											else {
												i->demand += 1;
											}

										}
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
	weighted_vector_tpl<const factory_desc_t *>producer;
	find_producer( producer, ware, welt->get_timeline_year_month() );
	if(  producer.empty()  ) {
		// can happen with timeline
		if(!info->is_consumer_only()) {
			dbg->error( "factory_builder_t::build_chain_link()", "no producer for %s!", ware->get_name() );
			return 0;
		}
		else {
			// only consumer: Will do with partly covered chains
			dbg->error( "factory_builder_t::build_chain_link()", "no producer for %s!", ware->get_name() );
		}
	}

	bool ignore_climates = false; // ignore climates after some retrys
	int retry=25; // and not more than 25 (happens mostly in towns)

	while(  (lcount>lfound  ||  lcount==0)  &&  consumption>0  &&  retry>0  ) {
		const factory_desc_t *producer_d = pick_any_weighted( producer );
		int rotate = simrand(producer_d->get_building()->get_all_layouts()-1);
		koord3d parent_pos = our_fab->get_pos();
		// ignore climates after 40 tries

		// if climates are ignored, then placement as well ...
		factory_desc_t::site_t placement = ignore_climates ? (producer_d->get_placement() == factory_desc_t::City ? factory_desc_t::City : factory_desc_t::Land) : producer_d->get_placement();

		INT_CHECK("fabrikbauer 697");

		koord3d k = find_random_construction_site( our_fab->get_pos().get_2d(), DISTANCE, producer_d->get_building()->get_size(rotate), placement, producer_d->get_building(), ignore_climates, 20000 );
		if(  k == koord3d::invalid  ) {
			// this factory cannot buuild in the desired vincinity
			producer.remove( producer_d );
			if(  producer.empty()  ) {
				if(  ignore_climates  ) {
					// absolutely no place to construct something here
					break;
				}
				find_producer( producer, ware, welt->get_timeline_year_month() );
				ignore_climates = true;
			}
			continue;
		}

		INT_CHECK("fabrikbauer 697");

		DBG_MESSAGE("factory_builder_t::build_chain_link","Try to built supplier %s at (%i,%i) r=%i for %s.",producer_d->get_name(),k.x,k.y,rotate,info->get_name());
		n += build_link(&parent_pos, producer_d, -1 /*random prodbase */, rotate, &k, player, 10000, ignore_climates);
		lfound ++;

		INT_CHECK( "fabrikbauer 702" );

		// now subtract current supplier
		fabrik_t *fab = fabrik_t::get_fab(k.get_2d() );
		if(fab==NULL) {
			DBG_MESSAGE( "factory_builder_t::build_chain_link", "Failed to build at %s", k.get_str() );
			retry --;
			continue;
		}
		new_factories.append(fab);

		// connect new supplier to us
		const factory_desc_t* const fd = fab->get_desc();
		for (uint gg = 0; gg < fab->get_desc()->get_product_count(); gg++) {
			if (fd->get_product(gg)->get_output_type() == ware) {
				sint32 production = fab->get_base_production() * fd->get_product(gg)->get_factor();
				// the take care of how much this factory could supply
				consumption -= production;
				DBG_MESSAGE("factory_builder_t::build_chain_link", "new supplier %s can supply approx %i of %s to us", fd->get_name(), production, ware->get_name());
				break;
			}
		}
	}

	// now we add us to all cross-connected factories
	for(fabrik_t* const i : crossconnected_supplier) {
		i->add_consumer(our_fab->get_pos().get_2d());
	}

	/* now the cross-connect part:
	 * connect also the factories we stole from before ...
	 */
	for(fabrik_t* const fab : new_factories) {
		for (slist_tpl<factories_to_crossconnect_t>::iterator i = factories_to_correct.begin(), end = factories_to_correct.end(); i != end;) {
			i->demand -= 1;
			fab->add_consumer(i->fab->get_pos().get_2d());
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
int factory_builder_t::increase_industry_density( bool tell_me )
{
	int nr = 0;
	fabrik_t *last_built_consumer = NULL;
#if MSG_LEVEL >= 3
	const goods_desc_t *last_built_consumer_ware = NULL;
#endif

	// find last consumer
	minivec_tpl<const goods_desc_t *>ware_needed;
	if(!welt->get_fab_list().empty()) {
		for(fabrik_t* const fab : welt->get_fab_list()) {
			if (fab->get_desc()->is_consumer_only()) {
				last_built_consumer = fab;
				break;
			}
		}

		// ok, found consumer
		if(  last_built_consumer  ) {
			ware_needed.clear();
			ware_needed.resize( last_built_consumer->get_desc()->get_supplier_count() );
			for(  int i=0;  i < last_built_consumer->get_desc()->get_supplier_count();  i++  ) {
				ware_needed.append_unique( last_built_consumer->get_desc()->get_supplier(i)->get_input_type() );
			}

			for(koord const& j : last_built_consumer->get_suppliers()) {
				factory_desc_t const* const fd = fabrik_t::get_fab(j)->get_desc();
				for (uint32 k = 0; k < fd->get_product_count(); k++) {
					ware_needed.remove( fd->get_product(k)->get_output_type() );
				}
			}
		}
	}

	// first: do we have to continue unfinished factory chains?
	if(  !ware_needed.empty()  && last_built_consumer  ) {

		int org_rotation = -1;
		// rotate until we can save it if one of the factories is non-rotate-able ...
		if(welt->cannot_save()  &&  !can_factory_tree_rotate(last_built_consumer->get_desc()) ) {
			org_rotation = welt->get_settings().get_rotation();
			for(  int i=0;  i<3  &&  welt->cannot_save();  i++  ) {
				welt->rotate90();
			}
			assert( !welt->cannot_save() );
		}


		uint32 last_suppliers = last_built_consumer->get_suppliers().get_count();
		do {
			// we have the ware but not the supplier for it ...
			int last_built_consumer_missing_supplier = 99999;
			for(  int i=0;  i < last_built_consumer->get_desc()->get_supplier_count();  i++  ) {
				if(  last_built_consumer->get_desc()->get_supplier(i)->get_input_type() == ware_needed[0]  ) {
					last_built_consumer_missing_supplier = i;
					break;
				}
			}

			nr += build_chain_link( last_built_consumer, last_built_consumer->get_desc(), last_built_consumer_missing_supplier, welt->get_public_player() );
#if MSG_LEVEL >=3
			last_built_consumer_ware = ware_needed[ 0 ];
#endif
			ware_needed.remove_at( 0 );

		} while(  !ware_needed.empty()  &&  last_built_consumer->get_suppliers().get_count()==last_suppliers  );

		// must rotate back?
		if(org_rotation>=0) {
			for (int i = 0; i < 4 && welt->get_settings().get_rotation() != org_rotation; ++i) {
				welt->rotate90();
			}
			welt->update_map();
		}

		// only return if successful
		if(  last_built_consumer->get_suppliers().get_count() > last_suppliers  ) {
			DBG_MESSAGE( "factory_builder_t::increase_industry_density()", "added ware %s to factory %s", last_built_consumer_ware->get_name(), last_built_consumer->get_name() );
			// tell the player
			if(tell_me) {
				stadt_t *s = welt->find_nearest_city( last_built_consumer->get_pos().get_2d() );
				const char *stadt_name = s ? s->get_name() : translator::translate("nowhere");
				cbuffer_t buf;
				buf.printf( translator::translate("Factory chain extended\nfor %s near\n%s built with\n%i factories."), translator::translate(last_built_consumer->get_name()), stadt_name, nr );
				welt->get_message()->add_message(buf, last_built_consumer->get_pos().get_2d(), message_t::industry, CITY_KI, last_built_consumer->get_desc()->get_building()->get_tile(0)->get_background(0, 0, 0));
			}
			minimap_t::get_instance()->calc_map();
			return nr;
		}
		else {
			DBG_MESSAGE( "factory_builder_t::increase_industry_density()", "failed to add ware %s to factory %s", last_built_consumer_ware->get_name(), last_built_consumer->get_name() );
		}
	}

	// ok, no chains to finish, thus we must start anew

	// first determine electric supply and demand
	uint32 electric_supply = 0;
	uint32 electric_demand = 1;

	for(fabrik_t* const fab : welt->get_fab_list()) {
		if(  fab->get_desc()->is_electricity_producer()  ) {
			electric_supply += fab->get_scaled_electric_demand() / PRODUCTION_DELTA_T;
		}
		else {
			electric_demand += fab->get_scaled_electric_demand() / PRODUCTION_DELTA_T;
		}
	}

	// now decide producer of electricity or normal ...
	sint32 promille = (electric_supply*1000l)/electric_demand;
	int    no_electric = promille > welt->get_settings().get_electric_promille(); // =1 -> build normal factories only
	DBG_MESSAGE( "factory_builder_t::increase_industry_density()", "production of electricity/total production is %i/%i (%i o/oo)", electric_supply, electric_demand, promille );

	while(  no_electric<2  ) {
		bool ignore_climates = false;

		for(int retries=20;  retries>0;  retries--  ) {
			const factory_desc_t *fab=get_random_consumer( no_electric==0, ALL_CLIMATES, welt->get_timeline_year_month() );
			if(fab) {
				const bool in_city = fab->get_placement() == factory_desc_t::City;
				if (in_city && welt->get_cities().empty()) {
					// we cannot build this factory here
					continue;
				}
				koord3d pos;
				int rotation = simrand( fab->get_building()->get_all_layouts() );
				if(!in_city) {
					// find somewhere on the map
					pos = find_random_construction_site( koord(welt->get_size().x/2,welt->get_size().y/2), welt->get_size_max()/2, fab->get_building()->get_size(rotation),fab->get_placement(),fab->get_building(),ignore_climates,10000);
				}
				else {
					// or within the city limit
					const stadt_t *city = pick_any_weighted(welt->get_cities());
					koord diff = city->get_rechtsunten()-city->get_linksoben();
					pos = find_random_construction_site( city->get_center(), max(diff.x,diff.y)/2, fab->get_building()->get_size(rotation),fab->get_placement(),fab->get_building(),ignore_climates, 1000);
				}
				if(welt->lookup(pos)) {
					// Space found...
					nr += build_link(NULL, fab, -1 /* random prodbase */, rotation, &pos, welt->get_public_player(), 1, ignore_climates);
					if(nr>0) {
						fabrik_t *our_fab = fabrik_t::get_fab( pos.get_2d() );
						minimap_t::get_instance()->calc_map_size();
						// tell the player
						if(tell_me) {
							const char *stadt_name = translator::translate("nowhere");
							if (!our_fab->get_target_cities().empty()) {
								stadt_name = our_fab->get_target_cities()[0]->get_name();
							}
							cbuffer_t buf;
							buf.printf( translator::translate("New factory chain\nfor %s near\n%s built with\n%i factories."), translator::translate(our_fab->get_name()), stadt_name, nr );
							welt->get_message()->add_message(buf, pos.get_2d(), message_t::industry, CITY_KI, our_fab->get_desc()->get_building()->get_tile(0)->get_background(0, 0, 0));
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
	dbg->warning( "factory_builder_t::increase_industry_density()", "No suitable city industry found => pak missing something?" );
	return 0;
}
