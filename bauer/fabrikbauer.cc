/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include "../simdebug.h"
#include "fabrikbauer.h"
#include "hausbauer.h"
#include "../simworld.h"
#include "../simintr.h"
#include "../simfab.h"
#include "../simmesg.h"
#include "../utils/simrandom.h"
#include "../simcity.h"
#include "../simhalt.h"
#include "../player/simplay.h"

#include "../boden/grund.h"

#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../network/pakset_info.h"
#include "../dataobj/translator.h"

#include "../tpl/vector_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "../tpl/array_tpl.h"
#include "../finder/building_placefinder.h"

#include "../utils/cbuffer_t.h"

#include "../gui/karte.h"	// to update map after construction of new industry


karte_ptr_t factory_builder_t::welt;

/// Default factory spacing
static int max_factory_spacing_general = 40;

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
	sint16       const  rotate  = fab->get_rotate();
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
	FOR(vector_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
		add_factory_to_fab_map(welt, f);
	}
	if(  welt->get_settings().get_max_factory_spacing_percent()  ) {
		max_factory_spacing_general = (welt->get_size_max() * welt->get_settings().get_max_factory_spacing_percent()) / 100l;
	}
	else {
		max_factory_spacing_general = welt->get_settings().get_max_factory_spacing();
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


void factory_builder_t::new_world()
{
	init_fab_map( welt );
}


/**
 * Searches for a suitable building site using find_place().
 * The site is chosen so that there is a street next to the building site.
 */
class factory_place_with_road_finder: public building_placefinder_t  {

public:
	factory_place_with_road_finder(karte_t* welt) : building_placefinder_t(welt) {}

	virtual bool is_area_ok(koord pos, sint16 b, sint16 h, climate_bits cl, uint16 allowed_regions) const
	{
		if(  !building_placefinder_t::is_area_ok(pos, b, h, cl, allowed_regions)  ) {
			// We need a clear space to build, first of all
			return false;
		}
		bool next_to_road = false;

		// For a 1x1, b=1 and h=1.  We want to go through a grid one larger on all sides
		// than the factory, so go from pos - 1 to pos + b, pos - 1 to pos + h.
		for (sint16 x = -1; x <= b; x++) {
			for (sint16 y = -1;  y <= h; y++) {
				koord k(pos + koord(x,y));
				grund_t *gr = welt->lookup_kartenboden(k);
				if (!gr) {
					// We want to keep the factory at least 1 away from the edge of the map.
					return false;
				}
				// Do not build in the exclusion zone of another factory.
				if(  is_factory_at(k.x, k.y)  ) {
					return false;
				}
				if (  -1 < x && x < b && -1 < y && y < h  ) {
					// Inside the target for factory.
					// Don't build under bridges, powerlines, etc.
					if(  gr->get_leitung()!=NULL  ||  welt->lookup(gr->get_pos()+koord3d(0,0,1))!=NULL  ) {
						// something on top (monorail or powerlines)
						return false;
					}
				}
				else if (  !next_to_road  &&  ((-1 < x && x < b) || (-1 < y && y < h))  ) {
					// Border (because previous clause didn't trigger) but not corner.
					// Look for a road.
					next_to_road = gr->hat_weg(road_wt);
				}
			}
		}
		return next_to_road;
	}
};


stringhashtable_tpl<const factory_desc_t *> factory_builder_t::desc_table;
stringhashtable_tpl<factory_desc_t *> factory_builder_t::modifiable_table;


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
const factory_desc_t *factory_builder_t::get_random_consumer(bool electric, climate_bits cl, uint16 allowed_regions, uint16 timeline, const goods_desc_t* input )
{
	// get a random city factory
	weighted_vector_tpl<const factory_desc_t *> consumer;

	FOR(stringhashtable_tpl<factory_desc_t const*>, const& i, desc_table)
	{
		factory_desc_t const* const current = i.value;
		// only insert end consumers, if applicable, with the requested input goods.
		if (  current->is_consumer_only()  &&
			current->get_building()->is_allowed_climate_bits(cl)  &&
			current->get_building()->is_allowed_region_bits(allowed_regions) &&
			(electric ^ !current->is_electricity_producer())  &&
			current->get_building()->is_available(timeline)  &&
			(input == NULL || current->get_accepts_these_goods(input))
			)
		{
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


const factory_desc_t *factory_builder_t::get_desc(const char *fabtype)
{
	return desc_table.get(fabtype);
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
		dbg->warning( "factory_builder_t::register_desc()", "Object %s was overlaid by addon!", desc->get_name() );
		delete old_desc;
	}
	desc_table.put(desc->get_name(), desc);
	modifiable_table.put(desc->get_name(), desc);
}


bool factory_builder_t::successfully_loaded()
{
	FOR(stringhashtable_tpl<factory_desc_t const*>, const& i, desc_table) {
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
				dbg->fatal( "factory_builder_t::build_link()", "Factory %s output %s cannot be its own input!", i.key, current->get_supplier(j)->get_input_type()->get_name() );
			}
		}
		checksum_t *chk = new checksum_t();
		current->calc_checksum(chk);
		pakset_info_t::append(current->get_name(), chk);
	}
	return true;
}


int factory_builder_t::count_producers(const goods_desc_t *ware, uint16 timeline)
{
	int anzahl=0;

	// iterate over all factories and check if they produce this good...
	FOR(stringhashtable_tpl<factory_desc_t const*>, const& t, desc_table) {
		factory_desc_t const* const tmp = t.value;
		for (uint i = 0; i < tmp->get_product_count(); i++) {
			const factory_product_desc_t *product = tmp->get_product(i);
			if(  product->get_output_type()==ware  &&  tmp->get_distribution_weight()>0  &&  tmp->get_building()->is_available(timeline)  ) {
				anzahl++;
			}
		}
	}
DBG_MESSAGE("factory_builder_t::count_producers()","%i producer for good '%s' found.", anzahl, translator::translate(ware->get_name()));
	return anzahl;
}


/*
 * Finds a random producer producing @p ware.
 * @param timeline the current time(months)
 */
void factory_builder_t::find_producer(weighted_vector_tpl<const factory_desc_t *> &producer, const goods_desc_t *ware, uint16 timeline )
{
	// find all producers
	producer.clear();
	FOR(stringhashtable_tpl<factory_desc_t const*>, const& t, desc_table) {
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


bool factory_builder_t::check_construction_site(koord pos, koord size, bool water, bool is_fabrik, climate_bits cl, uint16 regions_allowed)
{
	// check for water (no shore in sight!)
	if(water) {
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
		// check on land
		if (!welt->square_is_free(pos, size.x, size.y, NULL, cl, regions_allowed)) {
			return false;
		}
	}
	// check for existing factories
	if (is_fabrik) {
		for(int y=0;y<size.y;y++) {
			for(int x=0;x<size.x;x++) {
				if (is_factory_at(pos.x + x, pos.y + y)){
					return false;
				}
			}
		}
	}
	// Check for runways
	karte_t::runway_info ri = welt->check_nearby_runways(pos);
	if (ri.pos != koord::invalid)
	{
		return false;
	}

	return true;
}


koord3d factory_builder_t::find_random_construction_site( koord pos, const int radius, koord size, bool wasser, const building_desc_t *desc, bool ignore_climates, uint32 max_iterations )
{
	bool is_fabrik = desc->get_type() == building_desc_t::factory;
	if(wasser) {
		// to ensure at least 3x3 water around (maybe this should be the station catchment area+1?)
		size += koord(6,6);
	}

	climate_bits climates = !ignore_climates ? desc->get_allowed_climate_bits() : ALL_CLIMATES;

	uint32 diam   = 2*radius + 1;
	uint32 area = diam * diam;
	uint32 index  = simrand(area, "find_random_construction_site");
	koord k;

	max_iterations = min( area/(size.x*size.y)+1, max_iterations );
	const uint32 a = diam+1;
	const uint32 c = 37; // very unlikely to have this as a factor in somewhere ...

	// in order to stop on the first occurence, one has to iterate over all tiles in a reproducable but random enough manner
	for(  uint32 i = 0;  i<max_iterations; i++,  index = (a*index+c) % area  ) {

		// so it is guaranteed that the iteration hits all tiles and does not repeat itself
		k = koord( pos.x - radius + (index % diam), pos.y - radius + (index / diam) );

		// check place (it will actually check an grosse.x/y size rectangle, so we can iterate over less tiles)
		if(  factory_builder_t::check_construction_site(k, size, wasser, is_fabrik, climates, desc->get_allowed_region_bits())  ) {
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
	printf("Distributing %i tourist attractions ...\n",max_number);fflush(NULL);

	int retrys = max_number*4;
	while(current_number<max_number  &&  retrys-->0) {
		koord3d	pos=koord3d( koord::koord_random(welt->get_size().x,welt->get_size().y),1);
		const building_desc_t *attraction=hausbauer_t::get_random_attraction(welt->get_timeline_year_month(),true,(climate)simrand((int)arctic_climate+1, "void factory_builder_t::distribute_attractions"));

		// no attractions for that climate or too new
		if(attraction==NULL  ||  (welt->use_timeline()  &&  attraction->get_intro_year_month()>welt->get_current_month()) ) {
			continue;
		}

		int	rotation=simrand(attraction->get_all_layouts()-1, "void factory_builder_t::distribute_attractions");
		pos = find_random_construction_site(pos.get_2d(), 20, attraction->get_size(rotation),false,attraction,false,0x0FFFFFFF);	// so far -> land only
		if(welt->lookup(pos)) {
			// Platz gefunden ...
			gebaeude_t* gb = hausbauer_t::build(welt->get_public_player(), pos, rotation, attraction);
			current_number ++;
			retrys = max_number*4;
			const planquadrat_t* tile = welt->access(gb->get_pos().get_2d());
			stadt_t* city = tile ? tile->get_city() : NULL;
			if(city)
			{
				city->add_building_to_list(gb->access_first_tile());
				gb->set_stadt(city);
			}
			else
			{
				welt->add_building_to_world_list(gb->access_first_tile());
				if (welt->get_settings().get_auto_connect_industries_and_attractions_by_road())
				{
					gb->connect_by_road_to_nearest_city();
				}
			}
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
fabrik_t* factory_builder_t::build_factory(koord3d* parent, const factory_desc_t* info, sint32 initial_prod_base, int rotate, koord3d pos, player_t* owner)
{
	fabrik_t * fab = new fabrik_t(pos, owner, info, initial_prod_base);

	// now build factory
	fab->build(rotate, true /*add fields*/, initial_prod_base != -1 /* force initial prodbase ? */);
	welt->add_fab(fab);
	add_factory_to_fab_map(welt, fab);

	// TODO: Consider adding road connexions here.

	// We need to adjust the jobs/visitor demand based on the extent to which the range has altered base production.
	// The relevant ratio is fab->prodbase : fab->get_desc()->get_productivity().

	const uint32 percentage = (fab->get_base_production() * 100) / max(1, fab->get_desc()->get_productivity());
	if (percentage > 100)
	{
		fab->get_building()->set_adjusted_jobs((fab->get_building()->get_adjusted_jobs() * percentage) / 100);
		fab->get_building()->set_adjusted_visitor_demand((fab->get_building()->get_adjusted_visitor_demand() * percentage) / 100);
		fab->get_building()->set_adjusted_mail_demand((fab->get_building()->get_adjusted_mail_demand() * percentage) / 100);
	}

	// Update local roads
	fab->mark_connected_roads(false);

	// Add the factory to the world list
	fab->add_to_world_list();

	// Adjust the actual industry density
	welt->increase_actual_industry_density(100 / info->get_distribution_weight());
	if(parent) {
		fab->add_lieferziel(parent->get_2d());
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
			// Must recalc nearby halts after the halt is set up
			fab->recalc_nearby_halts();
		}
	}
	else {
		// Must recalc nearby halts after the halt is set up
		fab->recalc_nearby_halts();
	}

	// This must be done here because the building is not valid on generation, so setting the building's
	// jobs, population and mail figures based on the factory's cannot be done.
	fab->update_scaled_pax_demand();
	fab->update_scaled_mail_demand();
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
		FOR(stringhashtable_tpl<factory_desc_t const*>, const& t, desc_table) {
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

	// Industries in town needs different place search
	if (info->get_placement() == factory_desc_t::City) {

		koord size=info->get_building()->get_size(0);

		// build consumer (factory) in town
		stadt_t *city = welt->find_nearest_city(pos->get_2d());

		const climate_bits cl = ignore_climates ? ALL_CLIMATES : info->get_building()->get_allowed_climate_bits();
		const uint16 regions_allowed = info->get_building()->get_allowed_region_bits();

		/* Three variants:
		 * A:
		 * A building site, preferably close to the town hall with a street next to it.
		 * This could be a temporary problem, if a city has no such site and the search
		 * continues to the next city.
		 * Otherwise seems to me the most realistic.
		 */
		bool is_rotate=info->get_building()->get_all_layouts()>1  &&  size.x!=size.y  &&  info->get_building()->can_rotate();
		// first try with standard orientation
		koord k = factory_place_with_road_finder(welt).find_place(city->get_pos(), size.x, size.y, cl, regions_allowed);

		// second try: rotated
		koord k1 = koord::invalid;
		if (is_rotate  &&  (k == koord::invalid  ||  simrand(256, " factory_builder_t::build_link")<128)) {
			k1 = factory_place_with_road_finder(welt).find_place(city->get_pos(), size.y, size.x, cl, regions_allowed);
		}

		rotate = simrand( info->get_building()->get_all_layouts(), " factory_builder_t::build_link" );
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

	const fabrik_t *our_fab = build_factory(parent, info, initial_prod_base, rotate, *pos, player);

	INT_CHECK("fabrikbauer 596");

	// now build supply chains for all products
	for(int i=0; i<info->get_supplier_count()  &&  i<number_of_chains; i++) {
		n += build_chain_link( our_fab, info, i, player);
	}

	// everything built -> update map if needed
	if(parent==NULL) {
		DBG_MESSAGE("factory_builder_t::build_link()","update karte");

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


int factory_builder_t::build_chain_link(const fabrik_t* our_fab, const factory_desc_t* info, int supplier_nr, player_t* player, bool no_new_industries)
{
	int n = 0;	// number of additional factories
	/* first we try to connect to existing factories and will do some
	 * cross-connect (if wanted)
	 * We must take care to add capacity for cross-connected factories!
	 */
	const factory_supplier_desc_t *supplier = info->get_supplier(supplier_nr);
	const goods_desc_t *ware = supplier->get_input_type();
	const int anzahl_producer_d=count_producers( ware, welt->get_timeline_year_month() );

	if(anzahl_producer_d==0) {
		dbg->error("factory_builder_t::build_link()","No producer for %s found, chain incomplete!",ware->get_name() );
		return 0;
	}

	// how much do we need?
	sint32 consumption = our_fab->get_base_production() * (supplier ? supplier->get_consumption() : 1);

	slist_tpl<fabs_to_crossconnect_t> factories_to_correct;
	slist_tpl<fabrik_t *> new_factories;	      // since the cross-correction must be done later
	slist_tpl<fabrik_t *> crossconnected_supplier;	// also done after the construction of new chains

	int lcount = supplier->get_supplier_count();
	int lfound = 0;	// number of found producers

	DBG_MESSAGE("factory_builder_t::build_link","supplier_count %i, lcount %i (need %i of %s)",info->get_supplier_count(),lcount,consumption,ware->get_name());

	// We only observe the max_distance_to_supplier if the supplier can exist in our region.
	const uint8 our_factory_region = welt->get_region(our_fab->get_pos().get_2d());

	uint16 max_distance_to_supplier = our_fab->get_desc()->get_max_distance_to_supplier();
	if (max_distance_to_supplier < 65535)
	{
		// Check whether any types of factory that can be built at this time in this factory's region can supply this input good.
		weighted_vector_tpl<const factory_desc_t*>producer;
		find_producer(producer, ware, welt->get_timeline_year_month());
		bool local_supplier_unavailable = true;
		FOR(weighted_vector_tpl<const factory_desc_t*>, producer_type, producer)
		{
			if (producer_type->get_building()->is_allowed_region(our_factory_region))
			{
				local_supplier_unavailable = false;
				break;
			}
		}
		if (local_supplier_unavailable)
		{
			max_distance_to_supplier = 65535;
		}
	}

	// Hajo: search if there already is one or two (crossconnect everything if possible)
	FOR(vector_tpl<fabrik_t*>, const fab, welt->get_fab_list())
	{
		// Try to find matching factories for this consumption, but don't find more than two times number of factories requested.
		//if ((lcount != 0 || consumption <= 0) && lcount < lfound + 1) break;

		// For reference, our_fab is the consumer and fab is the potential producer already in the game.

		// connect to an existing one if this is a producer
		if(fab->vorrat_an(ware) > -1)
		{
			const uint32 distance = shortest_distance(fab->get_pos().get_2d(), our_fab->get_pos().get_2d());

			if(distance >= (uint32)welt->get_settings().get_min_factory_spacing() && distance <= fab->get_desc()->get_max_distance_to_consumer() && distance <= max_distance_to_supplier)
			{
				// ok, this would match
				// but can she supply enough?

				// now guess how much this factory can supply
				const factory_desc_t* const fd = fab->get_desc();
				for (uint gg = 0; gg < fd->get_product_count(); gg++) {
					if (fd->get_product(gg)->get_output_type() == ware && fab->get_lieferziele().get_count() < 10) { // does not make sense to split into more ...
						sint32 production_left = fab->get_base_production() * fd->get_product(gg)->get_factor();
						const vector_tpl <koord> & lieferziele = fab->get_lieferziele();

						// decrease remaining production by supplier demand
						FOR(vector_tpl<koord>, const& i, lieferziele) {
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
						if (production_left > 0 ||simrand(100, "factory_builder_t::build_link") < (uint32)welt->get_settings().get_crossconnect_factor()) {

							if(production_left>0) {
								consumption -= production_left;
								fab->add_lieferziel(our_fab->get_pos().get_2d());
								DBG_MESSAGE("factory_builder_t::build_link","supplier %s can supply approx %i of %s to us", fd->get_name(), production_left, ware->get_name());
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

	if(!no_new_industries)
	{
		/* try to add all types of factories until demand is satisfied
		 */
		weighted_vector_tpl<const factory_desc_t *>producer;
		find_producer( producer, ware, welt->get_timeline_year_month() );
		if(  producer.empty()  ) {
			// can happen with timeline
			if(!info->is_consumer_only()) {
				dbg->error( "factory_builder_t::build_link()", "no producer for %s!", ware->get_name() );
				return 0;
			}
			else {
				// only consumer: Will do with partly covered chains
				dbg->error( "factory_builder_t::build_link()", "no producer for %s!", ware->get_name() );
			}
		}

		bool ignore_climates = false;	// ignore climates after some retrys
		int retry=25;	// and not more than 25 (happens mostly in towns)
		while(  (lcount>lfound  ||  lcount==0)  &&  consumption>0  &&  retry>0  ) {

			const factory_desc_t *producer_d = pick_any_weighted( producer );

			int rotate = simrand(producer_d->get_building()->get_all_layouts()-1, "factory_builder_t::build_link");
			koord3d parent_pos = our_fab->get_pos();

			INT_CHECK("fabrikbauer 697");
			const int max_distance_to_consumer = producer_d->get_max_distance_to_consumer() == 0 ? max_factory_spacing_general : producer_d->get_max_distance_to_consumer();
			koord3d k = find_random_construction_site( our_fab->get_pos().get_2d(), min(max_distance_to_supplier, min(max_factory_spacing_general, max_distance_to_consumer)), producer_d->get_building()->get_size(rotate),producer_d->get_placement()==factory_desc_t::Water, producer_d->get_building(), ignore_climates, 20000 );
			if(  k == koord3d::invalid  ) {
				// this factory cannot build in the desired vincinity
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

			DBG_MESSAGE("factory_builder_t::build_link","Try to built supplier %s at (%i,%i) r=%i for %s.",producer_d->get_name(),k.x,k.y,rotate,info->get_name());
			n += build_link(&parent_pos, producer_d, -1 /*random prodbase */, rotate, &k, player, 10000, ignore_climates);
			lfound ++;

			INT_CHECK( "fabrikbauer 702" );

			// now subtract current supplier
			fabrik_t *fab = fabrik_t::get_fab(k.get_2d() );
			if(fab==NULL) {
				DBG_MESSAGE( "factory_builder_t::build_link", "Failed to build at %s", k.get_str() );
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
					DBG_MESSAGE("factory_builder_t::build_link", "new supplier %s can supply approx %i of %s to us", fd->get_name(), production, ware->get_name());
					break;
				}
			}
		}
	}

	// now we add us to all cross-connected factories
	FOR(slist_tpl<fabrik_t*>, const i, crossconnected_supplier) {
		i->add_lieferziel(our_fab->get_pos().get_2d());
	}

	if(!no_new_industries)
	{
		/* now the cross-connect part:
		 * connect also the factories we stole from before ...
		 */
		FOR(slist_tpl<fabrik_t*>, const fab, new_factories)
		{
			for(slist_tpl<fabs_to_crossconnect_t>::iterator i = factories_to_correct.begin(), end = factories_to_correct.end(); i != end;)
			{
				i->demand -= 1;
				const uint count = fab->get_desc()->get_product_count();
				bool supplies_correct_goods = false;
				for(uint x = 0; x < count; x ++)
				{
					const factory_product_desc_t* test_prod = fab->get_desc()->get_product(x);
					if(i->fab->get_produced_goods()->is_contained(test_prod->get_output_type()))
					{
						supplies_correct_goods = true;
						break;
					}
				}
				if(supplies_correct_goods)
				{
					fab->add_lieferziel(i->fab->get_pos().get_2d());
					i->fab->add_supplier(fab->get_pos().get_2d());
				}
				if (i->demand < 0)
				{
					i = factories_to_correct.erase(i);
				}
				else
				{
					++i;
				}
			}
		}
	}

	INT_CHECK( "fabrikbauer 723" );

	return n;
}


/* This method is called whenever it is time for industry growth.
 * If there is still a pending consumer, it will first complete another chain for it
 * If not, it will decide to either built a power station (if power is needed)
 * or built a new consumer near the indicated position, unless, as a 25% chance,
 * a new consumer is to be added in any event. This latter change is needed to make
 * sure that enough consumer industries get built.
 * @return: number of factories built
 */
int factory_builder_t::increase_industry_density( bool tell_me, bool do_not_add_beyond_target_density, bool power_stations_only, uint32 force_consumer )
{
	int nr = 0;

	// TODO: This is a somewhat hackish mechanism for ensuring that there are sufficient consumer industries.
	// In the future, the building of pure consumer industries other than power stations should be part of
	// the city growth system and taken out of this entirely. That would leave this system free to complete
	// industry chains as needed.
	const bool force_add_consumer = force_consumer == 2 || (force_consumer == 0 && 75 > simrand(100, "factory_builder_t::increase_industry_density()"));

	weighted_vector_tpl<const goods_desc_t*> oversupplied_goods;

	// Build a list of all industries with incomplete supply chains.
	if(!power_stations_only && !welt->get_fab_list().empty())
	{
		// A collection of all consumer industries that are not fully linked to suppliers.
		slist_tpl<fabrik_t*> unlinked_consumers;
		slist_tpl<const goods_desc_t*> missing_goods;


		FOR(vector_tpl<fabrik_t*>, fab, welt->get_fab_list())
		{
			sint32 available_for_consumption;
			sint32 consumption_level;
			for(uint16 l = 0; l < fab->get_desc()->get_supplier_count(); l ++)
			{
				// Check the list of possible suppliers for this factory type.
				const factory_supplier_desc_t* supplier_type = fab->get_desc()->get_supplier(l);
				const goods_desc_t* input_type = supplier_type->get_input_type();
				missing_goods.append_unique(input_type);
				const vector_tpl<koord> suppliers = fab->get_suppliers();

				// Check how much of this product that the current factory needs
				consumption_level = fab->get_base_production() * (supplier_type ? supplier_type->get_consumption() : 1);
				available_for_consumption = 0;

				FOR(vector_tpl<koord>, supplier_koord, suppliers)
				{
					if (available_for_consumption >= consumption_level)
					{
						break;
					}

					// Check whether the factory's actual suppliers supply any of this product.
					const fabrik_t* supplier = fabrik_t::get_fab(supplier_koord);
					if(!supplier)
					{
						continue;
					}

					for(uint p = 0; p < supplier->get_desc()->get_product_count(); p ++)
					{
						const factory_product_desc_t* consumer_type = supplier->get_desc()->get_product(p);
						const goods_desc_t* supplier_output_type = consumer_type->get_output_type();

						if(supplier_output_type == input_type)
						{
							// Check to see whether this existing supplier is able to supply *enough* of this product
							const sint32 total_output_supplier = supplier->get_base_production() * consumer_type->get_factor();
							sint32 used_output = 0;
							vector_tpl<koord> competing_consumers = supplier->get_lieferziele();
							for(uint32 n = 0; n < competing_consumers.get_count(); n ++)
							{
								const fabrik_t* competing_consumer = fabrik_t::get_fab(competing_consumers.get_element(n));
								for(int x = 0; x < competing_consumer->get_desc()->get_supplier_count(); x ++)
								{
									const goods_desc_t* consumer_output_type = consumer_type->get_output_type();
									if(consumer_output_type == supplier_output_type)
									{
										const factory_supplier_desc_t* alternative_supplier_to_consumer = competing_consumer->get_desc()->get_supplier(x);
										used_output += competing_consumer->get_base_production() * (alternative_supplier_to_consumer ? alternative_supplier_to_consumer->get_consumption() : 1);
									}
								}
								const sint32 remaining_output = total_output_supplier - used_output;
								if(remaining_output > 0)
								{
									available_for_consumption += remaining_output;
								}
							}

							if(available_for_consumption >= consumption_level)
							{
								// If the suppliers between them do supply enough of the product, do not list it as missing.
								missing_goods.remove(input_type);

								if (oversupplied_goods.is_contained(input_type))
								{
									// Avoid duplication
									oversupplied_goods.remove(input_type);
								}
								oversupplied_goods.append(input_type, available_for_consumption - consumption_level);
							}
						}
					}
				} // Actual suppliers
			} // Possible suppliers

			if(!missing_goods.empty())
			{
				unlinked_consumers.append_unique(fab);
			}
			missing_goods.clear();
		} // All industries

		int missing_goods_index = 0;

		// ok, found consumer
		if(!force_add_consumer && !unlinked_consumers.empty())
		{
			FOR(slist_tpl<fabrik_t*>, unlinked_consumer, unlinked_consumers)
			{
				for(int i=0;  i < unlinked_consumer->get_desc()->get_supplier_count();  i++)
				{
					goods_desc_t const* const w = unlinked_consumer->get_desc()->get_supplier(i)->get_input_type();
					for(uint32 j = 0; j < unlinked_consumer->get_suppliers().get_count(); j++)
					{
						fabrik_t *sup = fabrik_t::get_fab(unlinked_consumer->get_suppliers()[j]);
						const factory_desc_t* const fd = sup->get_desc();
						for (uint32 k = 0; k < fd->get_product_count(); k++)
						{
							if (fd->get_product(k)->get_output_type() == w)
							{
								missing_goods_index = i + 1;
								goto next_ware_check;
							}
						}
					}
next_ware_check:
					// ok, found something, text next
					;
				}

				// first: do we have to continue unfinished factory chains?
				if(missing_goods_index < unlinked_consumer->get_desc()->get_supplier_count())
				{
					int org_rotation = -1;
					// rotate until we can save it, if one of the factory is non-rotateable ...
					if(welt->cannot_save()  &&  !can_factory_tree_rotate(unlinked_consumer->get_desc()) )
					{
						org_rotation = welt->get_settings().get_rotation();
						for(  int i=0;  i<3  &&  welt->cannot_save();  i++  )
						{
							welt->rotate90();
						}
						assert( !welt->cannot_save() );
					}

					const uint32 last_suppliers = unlinked_consumer->get_suppliers().get_count();
					do
					{
						nr += build_chain_link(unlinked_consumer, unlinked_consumer->get_desc(), missing_goods_index, welt->get_public_player(), do_not_add_beyond_target_density && welt->get_actual_industry_density() >= welt->get_target_industry_density());
						missing_goods_index ++;
					} while(missing_goods_index < unlinked_consumer->get_desc()->get_supplier_count() && unlinked_consumer->get_suppliers().get_count()==last_suppliers);

					// must rotate back?
					if(org_rotation>=0) {
						for (int i = 0; i < 4 && welt->get_settings().get_rotation() != org_rotation; ++i) {
							welt->rotate90();
						}
						welt->update_map();
					}

					// only return, if successful
					if(unlinked_consumer->get_suppliers().get_count() > last_suppliers)
					{
						DBG_MESSAGE( "factory_builder_t::increase_industry_density()", "added ware %i to factory %s", missing_goods_index, unlinked_consumer->get_name() );
						// tell the player
						if(tell_me) {
							stadt_t *s = welt->find_nearest_city( unlinked_consumer->get_pos().get_2d() );
							const char *stadt_name = s ? s->get_name() : "simcity";
							cbuffer_t buf;
							buf.printf( translator::translate("Factory chain extended\nfor %s near\n%s built with\n%i factories."), translator::translate(unlinked_consumer->get_name()), stadt_name, nr );
							welt->get_message()->add_message(buf, unlinked_consumer->get_pos().get_2d(), message_t::industry, CITY_KI, unlinked_consumer->get_desc()->get_building()->get_tile(0)->get_background(0, 0, 0));
						}
						reliefkarte_t::get_karte()->calc_map();
						return nr;
					}
				}
			}
		}
	}

	// ok, no chains to finish, thus we must start anew

	// first decide whether a new powerplant is needed or not
	uint32 total_electric_demand = 1;
	uint32 electric_productivity = 0;

	FOR(vector_tpl<fabrik_t*>, const fab, welt->get_fab_list())
	{
		if(fab->get_desc()->is_electricity_producer())
		{
			electric_productivity += fab->get_scaled_electric_demand();
		}
		else
		{
			total_electric_demand += fab->get_scaled_electric_demand();
		}
	}

	const weighted_vector_tpl<stadt_t*>& staedte = welt->get_cities();

	for(weighted_vector_tpl<stadt_t*>::const_iterator i = staedte.begin(), end = staedte.end(); i != end; ++i)
	{
		total_electric_demand += (*i)->get_power_demand();
	}

	// now decide producer of electricity or normal ...
	const sint64 promille = ((sint64)electric_productivity * 4000l) / total_electric_demand;
	const sint64 target_promille = (sint64)welt->get_settings().get_electric_promille();
	int no_electric = force_add_consumer || (promille >= target_promille) ? 1 : 0;
	DBG_MESSAGE( "factory_builder_t::increase_industry_density()", "production of electricity/total electrical demand is %i/%i (%i o/oo)", electric_productivity, total_electric_demand, promille );

	// Determine whether to fill in oversupplied goods with a consumer industry, or generate one entirely randomly
	const goods_desc_t* input_for_consumer = NULL;
	if (!oversupplied_goods.empty() && simrand(100,"factory_builder_t::increase_industry_density()") < 20)
	{
		const uint32 pick = simrand(oversupplied_goods.get_sum_weight(), "factory_builder_t::increase_industry_density() 2");
		input_for_consumer = oversupplied_goods.at_weight(pick);
	}

	while(no_electric < 2)
	{
		bool ignore_climates = false;

		for(int retries = 20; retries > 0; retries--)
		{
			if (retries < 5)
			{
				// Give up trying to find the right consumer after trying too many times.
				input_for_consumer = NULL;
			}
			const factory_desc_t *consumer = get_random_consumer(no_electric==0, ALL_CLIMATES, 65535, welt->get_timeline_year_month(), input_for_consumer);
			if(consumer)
			{
				if(do_not_add_beyond_target_density && !consumer->is_electricity_producer())
				{
					// Make sure that industries are not added beyond target density.
					if(100U / consumer->get_distribution_weight() > (welt->get_target_industry_density() - welt->get_actual_industry_density()))
					{
						continue;
					}
				}
				const bool in_city = consumer->get_placement() == factory_desc_t::City;
				if (in_city && welt->get_cities().empty())
				{
					// we cannot build this factory here
					continue;
				}
				koord testpos = in_city ? pick_any_weighted(welt->get_cities())->get_pos() : koord::koord_random(welt->get_size().x, welt->get_size().y);
				koord3d pos = welt->lookup_kartenboden( testpos )->get_pos();
				int rotation = simrand(consumer->get_building()->get_all_layouts()-1, "factory_builder_t::increase_industry_density()");
				if(!in_city)
				{
					// find somewhere on the map
					pos = find_random_construction_site( koord(welt->get_size().x/2,welt->get_size().y/2), welt->get_size_max()/2, consumer->get_building()->get_size(rotation), consumer->get_placement()==factory_desc_t::Water, consumer->get_building(),ignore_climates,10000);
				}
				else
				{
					// or within the city limit
					const stadt_t *city = pick_any_weighted(welt->get_cities());
					koord diff = city->get_rechtsunten()-city->get_linksoben();
					pos = find_random_construction_site( city->get_center(), max(diff.x,diff.y)/2, consumer->get_building()->get_size(rotation), consumer->get_placement()==factory_desc_t::Water, consumer->get_building(), ignore_climates, 1000);
				}

				if(welt->lookup(pos))
				{
					// Space found...
					nr += build_link(NULL, consumer, -1 /* random prodbase */, rotation, &pos, welt->get_public_player(), 1, ignore_climates);
					if(nr > 0)
					{
						fabrik_t *our_fab = fabrik_t::get_fab( pos.get_2d() );
						reliefkarte_t::get_karte()->calc_map_size();
						// tell the player
						if(tell_me)
						{
							stadt_t *s = welt->find_nearest_city(pos.get_2d());
							const char *stadt_name = s ? s->get_name() : translator::translate("nowhere");
							cbuffer_t buf;
							buf.printf( translator::translate("New factory chain\nfor %s near\n%s built with\n%i factories."), translator::translate(our_fab->get_name()), stadt_name, nr );
							welt->get_message()->add_message(buf, pos.get_2d(), message_t::industry, CITY_KI, our_fab->get_desc()->get_building()->get_tile(0)->get_background(0, 0, 0));
						}
						return nr;
					}
				}
				else if(retries == 1 && !ignore_climates)
				{
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

bool factory_builder_t::power_stations_available()
{
	weighted_vector_tpl<const factory_desc_t*> power_stations;

	FOR(stringhashtable_tpl<const factory_desc_t *>, const& iter, desc_table)
	{
		const factory_desc_t* current = iter.value;
		if(!current->is_electricity_producer()
			|| (welt->use_timeline()
				&& (current->get_building()->get_intro_year_month() > welt->get_timeline_year_month()
					|| current->get_building()->get_retire_year_month() < welt->get_timeline_year_month()))
			)
		{
			continue;
		}
		return true;
	}
	return false;
}
