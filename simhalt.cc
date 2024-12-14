/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>

#include "freight_list_sorter.h"

#include "simcity.h"
#include "simcolor.h"
#include "simconvoi.h"
#include "simdebug.h"
#include "simfab.h"
#include "simhalt.h"
#include "simintr.h"
#include "simline.h"
#include "simmem.h"
#include "simmesg.h"
#include "simplan.h"
#include "player/simplay.h"
#include "gui/simwin.h"
#include "simworld.h"
#include "simware.h"

#include "bauer/hausbauer.h"
#include "bauer/goods_manager.h"

#include "descriptor/goods_desc.h"
#include "descriptor/tunnel_desc.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/strasse.h"

#include "dataobj/settings.h"
#include "dataobj/schedule.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/environment.h"

#include "obj/gebaeude.h"
#include "obj/label.h"
#include "obj/tunnel.h"
#include "obj/wayobj.h"

#include "gui/halt_info.h"
#include "gui/minimap.h"

#include "utils/simrandom.h"
#include "utils/simstring.h"

#include "tpl/binary_heap_tpl.h"

#include "vehicle/simvehicle.h"
#include "vehicle/pedestrian.h"

karte_ptr_t haltestelle_t::welt;

vector_tpl<halthandle_t> haltestelle_t::alle_haltestellen;

stringhashtable_tpl<halthandle_t> haltestelle_t::all_names;

// hash table only used during loading
inthashtable_tpl<sint32,halthandle_t> *haltestelle_t::all_koords = NULL;
// since size_x*size_y < 0x1000000, we have just to shift the high bits
#define get_halt_key(k,width) ( ((k).x*(width)+(k).y) /*+ ((k).z << 25)*/ )

uint8 haltestelle_t::status_step = 0;
uint8 haltestelle_t::reconnect_counter = 0;
uint8 haltestelle_t::connection_update_counter = 0;


static vector_tpl<convoihandle_t>stale_convois;
static vector_tpl<linehandle_t>stale_lines;

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295u // (1<<32)-1
#endif

// The goods info which is waiting at the halt
struct halt_waiting_goods_t {
    ware_t goods;

    // The time when the cargo arrived at this stop.
    // When this is INVALID_CARGO_ARRIVED_TIME, arrived_time is not available.
    uint32 arrived_time;

	halt_waiting_goods_t(const ware_t& w, uint32 t) : goods(w), arrived_time(t) {}
    halt_waiting_goods_t() : arrived_time(INVALID_CARGO_ARRIVED_TIME) {}
};

void haltestelle_t::reset_routing()
{
	reconnect_counter = welt->get_schedule_counter()-1;
}


#define WEIGHT_UPDATE_INTERVAL_TICKS 5000

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

	static vector_tpl<halthandle_t>::iterator iter( alle_haltestellen.begin() );
	if (alle_haltestellen.empty()) {
		return;
	}
	static uint32 last_reconnection_started_ticks = 0;
	const uint8 schedule_counter = welt->get_schedule_counter();
	if (reconnect_counter != schedule_counter) {
		// always start with reconnection, re-routing will happen after complete reconnection
		status_step = RECONNECTING;
		reconnect_counter = schedule_counter;
		iter = alle_haltestellen.begin();
		last_reconnection_started_ticks = welt->get_ticks();
	}

	const bool is_tgbr_enabled = welt->get_settings().get_goods_routing_policy()==GRP_FIFO_ET;
	if(  status_step==0  &&  is_tgbr_enabled  &&  welt->get_ticks() > last_reconnection_started_ticks + WEIGHT_UPDATE_INTERVAL_TICKS  ) {
		status_step = WEIGHT_UPDATE;
		last_reconnection_started_ticks = welt->get_ticks();
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

	if (status_step == RECONNECTING || status_step == WEIGHT_UPDATE) {
		// reconnecting finished, compute connected components in one sweep
		rebuild_connected_components();
	}

	if (status_step == RECONNECTING) {
		// reroute in next call
		status_step = REROUTING;
	}
	else if (status_step == REROUTING  ||  status_step == WEIGHT_UPDATE) {
		status_step = 0;
	}
	iter = alle_haltestellen.begin();
}


void haltestelle_t::start_load_game()
{
	all_koords = new inthashtable_tpl<sint32,halthandle_t>;
}


void haltestelle_t::end_load_game()
{
	delete all_koords;
	all_koords = NULL;
}

/**
 * return an index to a halt; it is only used for old games
 * by default create a new halt if none found
 */
halthandle_t haltestelle_t::get_halt_koord_index(koord k)
{
	if(!welt->is_within_limits(k)) {
		return halthandle_t();
	}
	// check in hashtable
	halthandle_t h;
	const sint32 n = get_halt_key(koord3d(k,-128), welt->get_size().y);
	assert(all_koords);
	h = all_koords->get( n );

	if(  !h.is_bound()  ) {
		// No halts found => create one
		h = haltestelle_t::create(k, NULL );
		all_koords->set( n,  h );
	}
	return h;
}


/* we allow only for a single stop per grund
 * this will only return something if this stop belongs to same player or is public, or is a dock (when on water)
 */
halthandle_t haltestelle_t::get_halt(const koord3d pos, const player_t *player )
{
	const grund_t *gr = welt->lookup(pos);
	if(gr) {
		if(gr->get_halt().is_bound()  &&  player_t::check_owner(player,gr->get_halt()->get_owner())  ) {
			return gr->get_halt();
		}
		// no halt? => we do the water check
		if(gr->is_water()) {
			// may catch bus stops close to water ...
			const planquadrat_t *plan = welt->access(pos.get_2d());
			const uint8 cnt = plan->get_haltlist_count();
			// first check for own stop
			for(  uint8 i=0;  i<cnt;  i++  ) {
				halthandle_t halt = plan->get_haltlist()[i];
				if(  halt->get_owner()==player  &&  halt->get_station_type()&dock  ) {
					return halt;
				}
			}
			// then for public stop
			for(  uint8 i=0;  i<cnt;  i++  ) {
				halthandle_t halt = plan->get_haltlist()[i];
				if(  halt->get_owner()==welt->get_public_player()  &&  halt->get_station_type()&dock  ) {
					return halt;
				}
			}
			// so: nothing found
		}
	}
	return halthandle_t();
}


halthandle_t haltestelle_t::get_stoppable_halt(const koord3d pos, const player_t *player )
{
	const grund_t *gr = welt->lookup(pos);
	if(  !gr  ) { return halthandle_t(); }
	const halthandle_t halt = gr->get_halt();
	if(  halt.is_bound()  ) {
		const bool accepts_other_player = halt->is_other_player_connection_allowed();
		if(  player_t::check_owner(player, halt->get_owner())  ||  accepts_other_player  ) {
			return halt;
		}
	}
	if(  !gr->is_water()  ) { return halthandle_t(); }
	// no halt? => we do the water check
	// may catch bus stops close to water ...
	const planquadrat_t *plan = welt->access(pos.get_2d());
	const uint8 cnt = plan->get_haltlist_count();
	// first check for own stop
	for(  uint8 i=0;  i<cnt;  i++  ) {
		halthandle_t halt = plan->get_haltlist()[i];
		if(  halt->get_owner()==player  &&  halt->get_station_type()&dock  ) {
			return halt;
		}
	}
	// then for public stop 
	for(  uint8 i=0;  i<cnt;  i++  ) {
		halthandle_t halt = plan->get_haltlist()[i];
		const bool accepts_other_player = halt->is_other_player_connection_allowed();
		if(  (halt->get_owner()==welt->get_public_player()  ||  accepts_other_player)  &&  halt->get_station_type()&dock  ) {
			return halt;
		}
	}
	// so: nothing found
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
	//assert(tiles.front().grund->get_pos().get_2d() == init_pos);
	return tiles.front().grund->get_pos();
}

// returns tile closest to this coordinate
grund_t *haltestelle_t::get_ground_closest_to( const koord here ) const
{
	uint32 distance = 0x7FFFFFFFu;
	grund_t *closest = NULL;
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		uint32 dist = shortest_distance( i.grund->get_pos().get_2d(), here );
		if(  dist < distance  ) {
			distance = dist;
			closest = i.grund;
			if(  distance == 0  ) {
				break;
			}
		}
	}

	return closest;
}



/**
 * return the closest square that belongs to this halt
 */
koord haltestelle_t::get_next_pos( koord start ) const
{
	if(  grund_t *gr=get_ground_closest_to(start)  ) {
		return gr->get_pos().get_2d();
	}
	return koord::invalid;
}



/* Calculate and set basis position of this station
 * It is the avarage of all tiles' coordinate weighed by level of the building */
void haltestelle_t::recalc_basis_pos()
{
	sint64 cent_x, cent_y;
	cent_x = cent_y = 0;
	uint64 level_sum;
	level_sum = 0;
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if(  gebaeude_t* const gb = i.grund->find<gebaeude_t>()  ) {
			uint32 lv;
			lv = gb->get_tile()->get_desc()->get_level() + 1;
			cent_x += gb->get_pos().get_2d().x * lv;
			cent_y += gb->get_pos().get_2d().y * lv;
			level_sum += lv;
		}
	}
	koord cent;
	cent = koord((sint16)(cent_x/level_sum),(sint16)(cent_y/level_sum));

	// save old name
	plainstring name = get_name();
	// clear name at old place (and the book-keeping)
	set_name(NULL);

	if ( level_sum > 0 ) {
		grund_t *new_center = get_ground_closest_to( cent );
		if(  new_center != tiles.front().grund  &&  new_center->get_text()==NULL  ) {
			// move to new center, if there is not yet a name on it
			tiles.remove( new_center );
			tiles.insert( new_center );
			init_pos = new_center->get_pos().get_2d();
		}
	}
	// .. and set name again (and do all the book-keeping)
	set_name(name);

	return;
}

/**
 * Station factory method. Returns handles instead of pointers.
 */
halthandle_t haltestelle_t::create(koord pos, player_t *player)
{
	haltestelle_t * p = new haltestelle_t(pos, player);
	return p->self;
}


/**
 * removes a ground tile from a station
 */
bool haltestelle_t::remove(player_t *player, koord3d pos)
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
			hausbauer_t::remove( player, gb );
			bd = NULL; // no need to recalc image
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
	else {
		halt->recalc_basis_pos();
	}

	// if building was removed this is false!
	if(bd) {
		bd->calc_image();
		minimap_t::get_instance()->calc_map_pixel(pos.get_2d());
	}
	return true;
}



/**
 * Station factory method. Returns handles instead of pointers.
 */
halthandle_t haltestelle_t::create(loadsave_t *file)
{
	haltestelle_t *p = new haltestelle_t(file);
	return p->self;
}


/**
 * Station destruction method.
 */
void haltestelle_t::destroy(halthandle_t const halt)
{
	delete halt.get_rep();
}


/**
 * Station destruction method.
 * Da destroy() alle_haltestellen modifiziert kann kein Iterator benutzt
 * werden!
 */
void haltestelle_t::destroy_all()
{
	while (!alle_haltestellen.empty()) {
		halthandle_t halt = alle_haltestellen.back();
		destroy(halt);
	}
	delete all_koords;
	all_koords = NULL;
	status_step = 0;
}


haltestelle_t::haltestelle_t(loadsave_t* file)
{
	last_loading_step = welt->get_steps();

	cargo = (slist_tpl<cargo_item_t> **)calloc( goods_manager_t::get_max_catg_index(), sizeof(slist_tpl<cargo_item_t> *) );
	fresh_cargo.resize( goods_manager_t::get_max_catg_index() );
	all_links = new link_t[ goods_manager_t::get_max_catg_index() ];
	staged_all_links = NULL;

	status_color = SYSCOL_TEXT_UNUSED;
	last_status_color = color_idx_to_rgb(COL_PURPLE);
	last_bar_count = 0;

	reconnect_counter = welt->get_schedule_counter()-1;

	enables = NOT_ENABLED;
	flags = 0;

	sortierung = freight_list_sorter_t::by_name;
	resort_freight_info = true;

	rdwr(file);

	markers[ self.get_id() ] = current_marker;

	alle_haltestellen.append(self);
}


haltestelle_t::haltestelle_t(koord k, player_t* player)
{
	self = halthandle_t(this);
	assert( !alle_haltestellen.is_contained(self) );
	alle_haltestellen.append(self);

	markers[ self.get_id() ] = current_marker;

	last_loading_step = welt->get_steps();

	this->init_pos = k;
	owner = player;

	enables = NOT_ENABLED;
	flags = 0;
	// force total re-routing
	reconnect_counter = welt->get_schedule_counter()-1;
	last_catg_index = 255;

	cargo = (slist_tpl<cargo_item_t> **)calloc( goods_manager_t::get_max_catg_index(), sizeof(slist_tpl<cargo_item_t> *) );
	fresh_cargo.resize( goods_manager_t::get_max_catg_index() );
	all_links = new link_t[ goods_manager_t::get_max_catg_index() ];
	staged_all_links = NULL;

	status_color = SYSCOL_TEXT_UNUSED;
	last_status_color = color_idx_to_rgb(COL_PURPLE);
	last_bar_count = 0;

	sortierung = freight_list_sorter_t::by_name;
	init_financial_history();
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

	// free name
	set_name(NULL);

	// remove from ground and planquadrat (tile) haltlists
	koord ul(32767,32767);
	koord lr(0,0);
	while(  !tiles.empty()  ) {
		grund_t *gr = tiles.remove_first().grund;
		koord pos = gr->get_pos().get_2d();
		gr->set_halt( halthandle_t() );
		// bounding box for adjustments
		ul.clip_max(pos);
		lr.clip_min(pos);
	}

	// remove from all haltlists
	uint16 const cov = welt->get_settings().get_station_coverage();
	ul.x = max(0, ul.x - cov);
	ul.y = max(0, ul.y - cov);
	lr.x = min(welt->get_size().x, lr.x + 1 + cov);
	lr.y = min(welt->get_size().y, lr.y + 1 + cov);
	for(  int y=ul.y;  y<lr.y;  y++  ) {
		for(  int x=ul.x;  x<lr.x;  x++  ) {
			planquadrat_t *plan = welt->access(x,y);
			if(plan->get_haltlist_count()>0) {
				plan->remove_from_haltlist(self);
			}
		}
	}

	destroy_win( magic_halt_info + self.get_id() );

	// finally detach handle
	// before it is needed for clearing up the planqudrat and tiles
	self.detach();

	for(unsigned i=0; i<goods_manager_t::get_max_catg_index(); i++) {
		if (cargo[i]) {
			FOR(slist_tpl<cargo_item_t>, const &w, *cargo[i]) {
				fabrik_t::update_transit(&w->goods, false);
			}
			delete cargo[i];
			cargo[i] = NULL;
		}
	}
	free( cargo );
	delete[] all_links;
	if(  staged_all_links  ) {
		delete[] staged_all_links;
	}

	// routes may have changed without this station ...
	verbinde_fabriken();
}


void haltestelle_t::rotate90( const sint16 y_size )
{
	init_pos.rotate90( y_size );

	// rotate cargo (good) destinations
	// iterate over all different categories
	for(unsigned i=0; i<goods_manager_t::get_max_catg_index(); i++) {
		if(cargo[i]) {
			slist_tpl<cargo_item_t>& warray = *cargo[i];
			for (  slist_tpl<cargo_item_t>::iterator ware = warray.begin();  ware!=warray.end();  ) {
				if(  ware->get()->goods.menge>0  ) {
					ware->get()->goods.rotate90(y_size);
					ware ++;
				} else {
					// empty => remove
					ware = warray.erase(ware);
				}
			}
		}
	}

	// re-linking factories
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
	stadt_t *stadt = welt->find_nearest_city(k);
	const char *stop = translator::translate(typ,lang);
	cbuffer_t buf;

	// this fails only, if there are no towns at all!
	if(stadt==NULL) {
		for(  uint32 i=1;  i<65536;  i++  ) {
			// get a default name
			buf.printf( translator::translate("land stop %i %s",lang), i, stop );
			if(  !all_names.get(buf).is_bound()  ) {
				return strdup(buf);
			}
			buf.clear();
		}
		return strdup("Unnamed");
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
			koord( 0, -1), // north
			koord( 1,  0), // east
			koord( 0,  1), // south
			koord(-1,  0), // west
			koord( 1, -1), // northeast
			koord( 1,  1), // southeast
			koord(-1,  1), // southwest
			koord(-1, -1), // northwest
			koord( 0, -2), // double nswo
			koord( 2,  0),
			koord( 0,  2),
			koord(-2,  0),
			koord( 1, -2), // all the remaining 3s
			koord( 2, -1),
			koord( 2,  1),
			koord( 1,  2),
			koord(-1,  2),
			koord(-2,  1),
			koord(-2, -1),
			koord(-1, -2),
			koord( 2, -2), // and now all buildings with distance 4
			koord( 2,  2),
			koord(-2,  2),
			koord(-2, -2)
		};

		// standard names:
		// order: factory, attraction, direction, normal name
		// first we try a factory name

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
					fabrik_t *fab = fabrik_t::get_fab(k+next_building[test]);
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
			// check for other special building (townhall, monument, tourist attraction)
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
				else if (gb->is_townhall() ||
					gb->get_tile()->get_desc()->get_type() == building_desc_t::attraction_land || // land attraction
					gb->get_tile()->get_desc()->get_type() == building_desc_t::attraction_city) { // town attraction
					building_name = make_single_line_string(translator::translate(gb->get_tile()->get_desc()->get_name(),lang), 2);
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
			const vector_tpl<char*>& street_names( translator::get_street_name_list() );
			// make sure we do only ONE random call regardless of how many names are available (to avoid desyncs in network games)
			if(  const uint32 count = street_names.get_count()  ) {
				uint32 idx = simrand( count );
				static const uint32 some_primes[] = { 19, 31, 109, 199, 409, 571, 631, 829, 1489, 1999, 2341, 2971, 3529, 4621, 4789, 7039, 7669, 8779, 9721 };
				// find prime that does not divide count
				uint32 offset = 1;
				for(uint8 i=0; i<lengthof(some_primes); i++) {
					if (count % some_primes[i]!=0) {
						offset = some_primes[i];
						break;
					}
				}
				// as count % offset != 0 we are guaranteed to test all street names
				for(uint32 i=0; i<count; i++) {
					buf.clear();
					if (cbuffer_t::check_format_strings("%s %s", street_names[idx])) {
						buf.printf( street_names[idx], city_name, stop );
						if(  !all_names.get(buf).is_bound()  ) {
							return strdup(buf);
						}
					}
					idx = (idx+offset) % count;
				}
				buf.clear();
			}
			else {
				/* the one random call to avoid desyncs */
				simrand(5);
			}
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
	if(  !loading_here.is_contained( cnv )  ) {
		loading_here.append( cnv );
	}
	if(  last_loading_step != welt->get_steps()  ) {
		last_loading_step = welt->get_steps();
		// now iterate over all convois
		for(  vector_tpl<convoihandle_t>::iterator i = loading_here.begin(); i != loading_here.end();  ) {
			convoihandle_t const c = *i;
			if (c.is_bound() && c->is_loading()) {
				// now we load into convoi
				const uint32 halt_length_for_convoy = c->calc_available_halt_length_in_vehicle_steps(c->front()->get_pos(), c->front()->get_direction());
				c->hat_gehalten(self, halt_length_for_convoy);
			}
			if (c.is_bound() && c->is_loading()) {
				++i;
			}
			else {
				i = loading_here.erase( i );
			}
		}
	}
}



bool haltestelle_t::has_available_network(const player_t* player, uint8 catg_index) const
{
	if(!player_t::check_owner( player, owner )) {
		return false;
	}
	if(  catg_index != goods_manager_t::INDEX_NONE  &&  !is_enabled(catg_index)  ) {
		return false;
	}
	// Check if there is a player's line
	FOR(vector_tpl<linehandle_t>, const l, registered_lines) {
		if(  l->get_owner() == player  &&  l->count_convoys()>0  ) {
			if(  catg_index == goods_manager_t::INDEX_NONE  ) {
				return true;
			}
			else if(  l->get_goods_catg_index().is_contained(catg_index)) {
				return true;
			}
		}
	}
	// Check lineless convoys
	FOR( vector_tpl<convoihandle_t>, cnv, registered_convoys ) {
		if(  cnv->get_owner() == player  &&  cnv->get_state() != convoi_t::INITIAL  ) {
			if(  catg_index == goods_manager_t::INDEX_NONE  ) {
				return true;
			}
			else if(  cnv->get_goods_catg_index().is_contained(catg_index)  ) {
				return true;
			}
		}
	};
	return false;
}




bool haltestelle_t::step(uint8 what, sint16 &units_remaining)
{
	switch(what) {
		case RECONNECTING:
		case WEIGHT_UPDATE:
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
 */
void haltestelle_t::new_month()
{
	if(  welt->get_active_player()==owner  &&  status_color==color_idx_to_rgb(COL_RED)  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("%s\nis crowded."), get_name() );
		welt->get_message()->add_message(buf, get_basis_pos(),message_t::full|message_t::expire_after_one_month_flag, PLAYER_FLAG|owner->get_player_nr(), IMG_EMPTY );
		enables &= (PAX|POST|WARE);
	}

	// roll financial history
	for (int j = 0; j<MAX_HALT_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	// number of waiting should be constant ...
	financial_history[0][HALT_WAITING] = financial_history[1][HALT_WAITING];
}



/**
 * Called after schedule calculation of all stations is finished
 * will distribute the goods to changed routes (if there are any)
 * returns true upon completion
 */
bool haltestelle_t::reroute_goods(sint16 &units_remaining)
{
	if(  last_catg_index==255  ) {
		last_catg_index = 0;
	}

	for(  ; last_catg_index<goods_manager_t::get_max_catg_index(); last_catg_index++) {

		if(  units_remaining<=0  ) {
			return false;
		}
		if(  !cargo[last_catg_index]  ) {
			continue;
		}

		// first: clean out the array
		slist_tpl<cargo_item_t> * warray = cargo[last_catg_index];
		for (  slist_tpl<cargo_item_t>::iterator ware = warray->begin(); ware!=warray->end();  ) {
			const ware_t& goods = ware->get()->goods;
			if(  goods.menge==0  ) {
				ware = warray->erase(ware);
				continue;
			}
			// since also the factory halt list is added to the ground, we can use just this ...
			else if(  welt->access(goods.get_zielpos())->is_connected(self)  ) {
				// we are already there!
				if(  goods.to_factory  ) {
					liefere_an_fabrik(goods);
				}
				ware = warray->erase(ware);
				continue;
			}
			ware ++;
		}

		// delete, if nothing connects here
		if(  warray->empty()  &&  all_links[last_catg_index].connections.empty()  ) {
			// no connections from here => delete
			delete cargo[last_catg_index];
			cargo[last_catg_index] = NULL;
			continue;
		}

		// if something left
		// re-route goods to adapt to changes in world layout,
		// remove all goods whose destination was removed from the map
		for(  slist_tpl<cargo_item_t>::iterator ware = warray->begin();  ware!=warray->end();  ) {
			ware_t& goods = ware->get()->goods;
			if(  !is_route_search_needed(goods)  ) {
				ware++;
				continue;
			}
			search_route_resumable(goods);
			if(  goods.get_ziel()==halthandle_t()  ) {
				// remove invalid destinations
				fabrik_t::update_transit( &goods, false);
				ware = warray->erase(ware);
			}
			else {
				ware++;
			}
		}

	}
	// likely the display must be updated after this
	resort_freight_info = true;
	last_catg_index = 255; // all categories are rerouted
	return true; // all updated ...
}



/*
 * connects a factory to a halt
 */
bool haltestelle_t::connect_factory(fabrik_t *fab)
{
	if(!fab_list.is_contained(fab)) {
		// water factories can only connect to docks
		if(  fab->get_desc()->get_placement() != factory_desc_t::Water  ||  (station_type & dock) > 0  ) {
			// do no link to oil rigs via stations ...
			fab_list.insert(fab);
			return true;
		}
	}
	return false;
}


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

		uint16 const cov = welt->get_settings().get_station_coverage();
		FOR(vector_tpl<fabrik_t*>, const fab, fabrik_t::sind_da_welche(p - koord(cov, cov), p + koord(cov, cov))) {
			if (connect_factory(fab)) {
				// also connect the factory to this halt
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


// A helper method of rebuild_connections() when the time based goods routing is enabled.
// Returns the estimated waiting time for the given schedule, for the given index halt.
// The base waiting ticks is added as a waiting time.
uint32 estimated_waiting_ticks(const schedule_t* schedule, uint8 index) {
	const schedule_entry_t schedule_entry = schedule->entries[index];
	const uint32 average_waiting_time = schedule_entry.get_average_waiting_time();
	const uint32 base_waiting_ticks = world()->get_settings().get_base_waiting_ticks(schedule->get_waytype());
	const uint32 additional_ticks_by_schedule = (uint64)schedule->get_additional_base_waiting_time() * world()->ticks_per_world_month / world()->get_settings().get_spacing_shift_divisor();
	return average_waiting_time + base_waiting_ticks + additional_ticks_by_schedule;
}


/**
 * Rebuilds the list of connections to directly reachable halts
 * Returns the number of stops considered
 */
#define WEIGHT_WAIT welt->get_settings().get_routecost_wait()
#define WEIGHT_HALT welt->get_settings().get_routecost_halt()
// the minimum weight of a connection from a transfer halt
#define WEIGHT_MIN (WEIGHT_WAIT+WEIGHT_HALT)
sint32 haltestelle_t::rebuild_connections()
{
	// These variables are for calculating is_transfer.
	// halts which either immediately precede or succeed self halt in serving schedules
	vector_tpl<halthandle_t> consecutive_halts[256];
	// halts which either immediately precede or succeed self halt in currently processed schedule
	vector_tpl<halthandle_t> consecutive_halts_schedule[256];
	// remember max number of consecutive halts for one schedule
	uint8 max_consecutive_halts_schedule[256];
	MEMZERON(max_consecutive_halts_schedule, goods_manager_t::get_max_catg_index());
	// previous halt supporting the ware categories of the serving line
	halthandle_t previous_halt[256];

	// true if the estimated time based goods routing is enabled. false if we use route cost calculation.
	const bool is_tbgr_enabled = welt->get_settings().get_goods_routing_policy() == goods_routing_policy_t::GRP_FIFO_ET;

	// first, remove all old entries
	if(  staged_all_links  ) {
		// rebuild_connections() is called before the staged links are commited.
		delete[] staged_all_links;
	}
	staged_all_links = new link_t[ goods_manager_t::get_max_catg_index() ];
	resort_freight_info = true; // might result in error in routing

	last_catg_index = 255; // must reroute everything
	sint32 connections_searched = 0;

// DBG_MESSAGE("haltestelle_t::rebuild_destinations()", "Adding new table entries");

	const player_t *owner;
	schedule_t *schedule;
	const minivec_tpl<uint8> *goods_catg_index;
	sint32 speedbonus_kmh;
	traveler_t traveler;

	minivec_tpl<uint8> supported_catg_index(32);

	/*
	 * In the first loops:
	 * lines==true => search for lines
	 * After this:
	 * lines==false => search for single convoys without lines
	 */
	bool lines = true;
	uint32 current_index = 0;
	// Is unload_all applied for this halt?
	// HACK: When unload_all, no_load or no_unload is applied, is_transfer is true regardless of connections.
	bool force_transfer_search = false;
	while(  lines  ||  current_index < registered_convoys.get_count()  ) {

		// Now, collect the "schedule", "owner" and "add_catg_index" from line resp. convoy.
		if(  lines  ) {
			if(  current_index >= registered_lines.get_count()  ) {
				// We have looped over all lines.
				lines = false;
				current_index = 0; // start over for registered lineless convoys
				continue;
			}

			const linehandle_t line = registered_lines[current_index];
			++current_index;

			owner = line->get_owner();
			schedule = line->get_schedule();
			goods_catg_index = &line->get_goods_catg_index();
			speedbonus_kmh = max(line->get_finance_history(0, LINE_MAXSPEED), 10); // To avoid zero division
			traveler = line;
		}
		else {
			const convoihandle_t cnv = registered_convoys[current_index];
			++current_index;

			owner = cnv->get_owner();
			schedule = cnv->get_schedule();
			goods_catg_index = &cnv->get_goods_catg_index();
			speedbonus_kmh = max(cnv->get_speedbonus_kmh(), 10); // To avoid zero division
			traveler = cnv;
		}

		if(  schedule->is_temporary()  ) {
			// this schedule does not affect connection calculation.
			continue;
		}

		// find the index from which to start processing
		uint8 start_index = 0;
		while(  start_index < schedule->get_count()  &&  get_stoppable_halt( schedule->entries[start_index].pos, owner ) != self  ) {
			++start_index;
		}
		if(  start_index==schedule->get_count()  ) {
			// self halt entry was not found.
			dbg->error("haltestelle_t::rebuild_connections()", "The self halt was not found for the schedule at %s. Type: %d, number of halts: %d", get_name(), schedule->get_type(), schedule->get_count());
			continue;
		}
		++start_index;	// the next index after self halt; it's okay to be out-of-range

		// determine goods category indices supported by this halt
		supported_catg_index.clear();
		FOR(minivec_tpl<uint8>, const catg_index, *goods_catg_index) {
			if(  is_enabled(catg_index)  ) {
				supported_catg_index.append(catg_index);
				previous_halt[catg_index] = self;
				consecutive_halts_schedule[catg_index].clear();
			}
		}

		if(  supported_catg_index.empty()  ) {
			// this halt does not support the goods categories handled by the line/lineless convoy
			continue;
		}

		INT_CHECK("simhalt.cc 612");

		// now we add the schedule to the connection array
		const schedule_entry_t start_entry = schedule->entries[start_index-1];

		// aggregate_weight: When TBGR is enabled, (average goods waiting time) + (median journey time)
		// When TBGR is disabled, it is route cost. e.g. WEIGHT_HALT + WEIGHT_WAIT * (stops count)
		sint32 aggregate_weight;
		if(  is_tbgr_enabled  ) {
			// the journey time of the first entry contains the stopping time at the starting point, which should be excluded.
			aggregate_weight = estimated_waiting_ticks(schedule, start_index-1) - start_entry.get_median_convoy_stopping_time();
		}
		else {
			aggregate_weight = WEIGHT_WAIT;
		}

		bool no_load_section = start_entry.is_no_load();
		force_transfer_search |= (start_entry.is_unload_all()  ||  start_entry.is_no_load()  ||  start_entry.is_no_unload());
		uint8 interval = 0;
		for(  uint8 j=0;  j<schedule->get_count();  ++j  ) {
			const uint8 current_entry_index = (start_index+j)%schedule->get_count();
			const schedule_entry_t current_entry = schedule->entries[current_entry_index];
			halthandle_t current_halt = get_stoppable_halt(current_entry.pos, owner );
			if(  !current_halt.is_bound()  ) {
				// ignore way points.
				// just count the journey time
				if(  is_tbgr_enabled  ) {
					aggregate_weight += schedule->get_median_journey_time(current_entry_index, speedbonus_kmh);
				}
				continue;
			}
			if(  current_halt == self  ) {
				// check for consecutive halts which precede self halt
				FOR(minivec_tpl<uint8>, const catg_index, supported_catg_index) {
					if(  previous_halt[catg_index]!=self  ) {
						consecutive_halts[catg_index].append_unique(previous_halt[catg_index]);
						consecutive_halts_schedule[catg_index].append_unique(previous_halt[catg_index]);
						previous_halt[catg_index] = self;
					}
				}
				// reset aggregate weight and no_load_section
				if(  is_tbgr_enabled  ) {
					aggregate_weight = estimated_waiting_ticks(schedule, current_entry_index) - current_entry.get_median_convoy_stopping_time();
				} else {
					aggregate_weight = WEIGHT_WAIT;
				}
			 	force_transfer_search |= (current_entry.is_unload_all()  ||  current_entry.is_no_load()  ||  current_entry.is_no_unload());
				no_load_section = current_entry.is_no_load();
				interval = 0;
				continue;
			}

			// Add weight
			if(  is_tbgr_enabled  ) {
				aggregate_weight += schedule->get_median_journey_time(current_entry_index, (uint32)speedbonus_kmh);
			} else {
				aggregate_weight += WEIGHT_HALT;
			}

			if(  current_entry.is_no_unload()  ||  no_load_section  ) {
				// do not add connection if this halt is set no_unload or if the previous self stop is set no_load.
				continue;
			}

			no_load_section |= current_entry.is_unload_all();
			
			if(current_entry.is_transfer_interval()){
				if(interval == 1){
					no_load_section |= current_entry.is_transfer_interval();
				}
				++interval;
			}

			FOR(minivec_tpl<uint8>, const catg_index, supported_catg_index) {
				if(  current_halt->is_enabled(catg_index)  ) {
					// check for consecutive halts which succeed self halt
					if(  previous_halt[catg_index] == self  ) {
						consecutive_halts[catg_index].append_unique(current_halt);
						consecutive_halts_schedule[catg_index].append_unique(current_halt);
					}
					previous_halt[catg_index] = current_halt;

					// either add a new connection or update the weight of an existing connection where necessary
					connection_t *const existing_connection = staged_all_links[catg_index].connections.insert_unique_ordered( connection_t( current_halt, aggregate_weight, traveler ), connection_t::compare );
					if(  existing_connection  &&  aggregate_weight<existing_connection->weight  ) {
						existing_connection->weight = aggregate_weight;
						existing_connection->best_weight_traveler = traveler;
					}
				}
			}
		}

		FOR(minivec_tpl<uint8>, const catg_index, supported_catg_index) {
			if(  consecutive_halts_schedule[catg_index].get_count() > max_consecutive_halts_schedule[catg_index]  ) {
				max_consecutive_halts_schedule[catg_index] = consecutive_halts_schedule[catg_index].get_count();
			}
		}
		connections_searched += schedule->get_count();
	}
	for(  uint8 i=0;  i<goods_manager_t::get_max_catg_index();  i++  ){
		// one schedule reaches all consecutive halts -> this is not transfer halt
		staged_all_links[i].is_transfer = !consecutive_halts[i].empty()  &&
			(consecutive_halts[i].get_count() != max_consecutive_halts_schedule[i]  ||
			force_transfer_search);
	}
	return connections_searched;
}


void haltestelle_t::rebuild_linked_connections()
{
	vector_tpl<halthandle_t> all; // all halts connected to this halt
	for(  uint8 i=0;  i<goods_manager_t::get_max_catg_index();  i++  ){
		vector_tpl<connection_t>& connections = all_links[i].connections;

		FOR(vector_tpl<connection_t>, &c, connections) {
			all.append_unique( c.halt );
		}
	}
	FOR(vector_tpl<halthandle_t>, h, all) {
		h->rebuild_connections();
	}
}


void haltestelle_t::fill_connected_component(uint8 catg_idx, uint16 comp)
{
	if (all_links[catg_idx].catg_connected_component != UNDECIDED_CONNECTED_COMPONENT) {
		// already connected
		return;
	}
	all_links[catg_idx].catg_connected_component = comp;

	FOR(vector_tpl<connection_t>, &c, all_links[catg_idx].connections) {
		c.halt->fill_connected_component(catg_idx, comp);
		// cache the is_transfer value
		c.is_transfer = c.halt->is_transfer(catg_idx);
	}
}


void haltestelle_t::rebuild_connected_components()
{
	// commit staged_all_links of all halts
	FOR(vector_tpl<halthandle_t>, halt, alle_haltestellen) {
		if(  halt->staged_all_links  ) {
			delete[] halt->all_links;
			halt->all_links = halt->staged_all_links;
			halt->staged_all_links = NULL;
		}
	}
	// calculate catg_connected_component
	for(uint8 catg_idx = 0; catg_idx<goods_manager_t::get_max_catg_index(); catg_idx++) {
		FOR(vector_tpl<halthandle_t>, halt, alle_haltestellen) {
			if (halt->all_links[catg_idx].catg_connected_component == UNDECIDED_CONNECTED_COMPONENT) {
				// start recursion
				halt->fill_connected_component(catg_idx, halt.get_id());
			}
		}
	}
	connection_update_counter += 1; // notify that the connections have been updated
}


sint8 haltestelle_t::is_connected(halthandle_t halt, uint8 catg_index) const
{
	if (!halt.is_bound()) {
		return 0; // not connected
	}
	const link_t& linka =       all_links[catg_index];
	const link_t& linkb = halt->all_links[catg_index];
	if (linka.connections.empty()  ||  linkb.connections.empty()) {
		return 0; // empty connections -> not connected
	}
	if (linka.catg_connected_component == UNDECIDED_CONNECTED_COMPONENT  ||  linkb.catg_connected_component == UNDECIDED_CONNECTED_COMPONENT) {
		return -1; // undecided - try later
	}
	// now check whether both halts are in the same component
	return linka.catg_connected_component == linkb.catg_connected_component ? 1 : 0;
}


/**
 * Helper class that combines a vector_tpl
 * with WEIGHT_HEAP elements and a binary_heap_tpl.
 * If inserted node has weight less than WEIGHT_HEAP,
 * then node is inserted in one of the vectors.
 * If weight is larger, then node goes into the heap.
 *
 * WEIGHT_HEAP = 200 should be safe for even the largest games.
 */
template<class T> class bucket_heap_tpl
{
#define WEIGHT_HEAP (200)
	vector_tpl<T> *buckets;  ///< array of vectors
	binary_heap_tpl<T> heap; ///< the heap

	uint16 min_weight; ///< current min_weight of nodes in the buckets
	uint32 node_count; ///< total count of nodes
public:
	bucket_heap_tpl() : heap(128)
	{
		min_weight = WEIGHT_HEAP;
		node_count = 0;
		buckets = new vector_tpl<T> [WEIGHT_HEAP];
	}

	~bucket_heap_tpl()
	{
		delete [] buckets;
	}

	void insert(const T item)
	{
		node_count++;
		uint16 weight = *item;

		if (weight < WEIGHT_HEAP) {
			if (weight < min_weight) {
				min_weight = weight;
			}
			buckets[weight].append(item);
		}
		else {
			heap.insert(item);
		}
	}

	T pop()
	{
		assert(!empty());
		node_count--;

		if (min_weight < WEIGHT_HEAP) {
			T ret = buckets[min_weight].pop_back();

			while(min_weight < WEIGHT_HEAP  &&  buckets[min_weight].empty()) {
				min_weight++;
			}
			return ret;
		}
		else {
			return heap.pop();
		}
	}

	void clear()
	{
		for(uint16 i=min_weight; i<WEIGHT_HEAP; i++) {
			buckets[i].clear();
		}
		min_weight = WEIGHT_HEAP;
		node_count = 0;

		heap.clear();
	}

	uint32 get_count() const
	{
		return node_count;
	}

	const T& front()
	{
		assert(!empty());
		if (min_weight < WEIGHT_HEAP) {
			return buckets[min_weight].back();
		}
		else {
			return heap.front();
		}
	}

	bool empty() const { return node_count == 0; }
};

/**
 * Data for route searching
 */
haltestelle_t::halt_data_t haltestelle_t::halt_data[65536];
bucket_heap_tpl<haltestelle_t::route_node_t> haltestelle_t::open_list;
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
 *  - during re-routing
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
 */
int haltestelle_t::search_route( const halthandle_t *const start_halts, const uint16 start_halt_count, const bool no_routing_over_overcrowding, ware_t &ware, ware_t *const return_ware )
{
	const uint8 ware_catg_idx = ware.get_desc()->get_catg_index();
	const uint8 ware_idx = ware.get_desc()->get_index();

	// since also the factory halt list is added to the ground, we can use just this ...
	const planquadrat_t *const plan = welt->access( ware.get_zielpos() );
	const halthandle_t *const halt_list = plan->get_haltlist();
	// but we can only use a subset of these
	static vector_tpl<halthandle_t> end_halts(16);
	end_halts.clear();
	// target halts are in these connected components
	// we start from halts only in the same components
	static vector_tpl<uint16> end_conn_comp(16);
	end_conn_comp.clear();
	// if one target halt is undefined, we have to start search from all halts
	bool end_conn_comp_undefined = false;

	for( uint32 h=0;  h<plan->get_haltlist_count();  ++h ) {
		halthandle_t halt = halt_list[h];
		if(  halt.is_bound()  &&  halt->is_enabled(ware_catg_idx)  ) {
			// check if this is present in the list of start halts
			for(  uint16 s=0;  s<start_halt_count;  ++s  ) {
				if(  halt==start_halts[s]  ) {
					// destination halt is also a start halt -> within walking distance
					ware.set_ziel( start_halts[s] );
					ware.clear_transit_halts();
					if(  return_ware  ) {
						return_ware->set_ziel( start_halts[s] );
						ware.clear_transit_halts();
					}
					return ROUTE_WALK;
				}
			}
			end_halts.append(halt);

			// check connected component of target halt
			uint16 endhalt_conn_comp = halt->all_links[ware_catg_idx].catg_connected_component;
			if (endhalt_conn_comp == UNDECIDED_CONNECTED_COMPONENT) {
				// undefined: all start halts are probably connected to this target
				end_conn_comp_undefined = true;
			}
			else {
				// store connected component
				if (!end_conn_comp_undefined) {
					end_conn_comp.append_unique( endhalt_conn_comp );
				}
			}
		}
	}

	if(  end_halts.empty()  ) {
		// no end halt found
		ware.set_ziel( halthandle_t() );
		ware.clear_transit_halts();
		if(  return_ware  ) {
			return_ware->set_ziel( halthandle_t() );
			return_ware->clear_transit_halts();
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
		halt_data[ halt_id ].best_weight = UINT32_MAX;
		halt_data[ halt_id ].destination = 1u;
		halt_data[ halt_id ].depth       = 1u; // to distinct them from start halts
		markers[ halt_id ] = current_marker;
	}

	uint16 const max_transfers = welt->get_settings().get_max_transfers();
	uint16 const max_hops      = welt->get_settings().get_max_hops();
	uint16 allocation_pointer = 0;
	uint32 best_destination_weight = UINT32_MAX; // best weight among all destinations

	open_list.clear();

	uint32 overcrowded_nodes = 0;
	// initialise the origin node(s)
	for(  ;  allocation_pointer<start_halt_count;  ++allocation_pointer  ) {
		halthandle_t start_halt = start_halts[allocation_pointer];

		uint16 start_conn_comp = start_halt->all_links[ware_catg_idx].catg_connected_component;

		if (!end_conn_comp_undefined   &&  start_conn_comp != UNDECIDED_CONNECTED_COMPONENT  &&  !end_conn_comp.is_contained( start_conn_comp  )){
			// this start halt will not lead to any target
			continue;
		}
		open_list.insert( route_node_t(start_halt, 0) );

		halt_data_t & start_data = halt_data[ start_halt.get_id() ];
		start_data.best_weight = UINT32_MAX;
		start_data.destination = 0;
		start_data.depth       = 0;
		start_data.overcrowded = false; // start halt overcrowding is handled by routines calling this one
		start_data.transfer    = halthandle_t();

		markers[ start_halt.get_id() ] = current_marker;
	}

	// here the normal routing with overcrowded stops is done
	while (!open_list.empty())
	{
		if(  overcrowded_nodes == open_list.get_count()  ) {
			// all unexplored routes go over overcrowded stations
			return ROUTE_OVERCROWDED;
		}

		// take node out of open list
		route_node_t current_node = open_list.pop();
		// do not use aggregate_weight as it is _not_ the weight of the current_node
		// there might be a heuristic weight added

		const uint16 current_halt_id = current_node.halt.get_id();
		halt_data_t & current_halt_data = halt_data[ current_halt_id ];
		overcrowded_nodes -= current_halt_data.overcrowded;

		if(  current_halt_data.destination  ) {
			// destination found
			ware.set_ziel( current_node.halt );
			assert(current_halt_data.transfer.get_id() != 0);
			if(  return_ware  ) {
				// next transfer for the reverse route
				// if the end halt and its connections contain more than one transfer halt then
				// the transfer halt may not be the last transfer of the forward route
				// (the re-routing will happen in haltestelle_t::fetch_goods)

				// count the connected transfer halts (including end halt)
				uint8 t = current_node.halt->is_transfer(ware_catg_idx);
				FOR(vector_tpl<connection_t>, const& i, current_node.halt->all_links[ware_catg_idx].connections) {
					if (t > 1) {
						break;
					}
					t += i.halt.is_bound() && i.is_transfer;
				}
				if(  t<=1  ) {
					vector_tpl<halthandle_t> transit_halts;
					transit_halts.append(current_halt_data.transfer);
					return_ware->set_transit_halts( transit_halts );
				} else {
					return_ware->clear_transit_halts();
				}
			}
			// build the transit_halts
			vector_tpl<halthandle_t> transit_halts;
			build_transit_halts_from_halt_data(transit_halts, current_node.halt);
			ware.set_transit_halts(transit_halts);
			const halthandle_t first_transfer_halt = transit_halts.front();
			if(  return_ware  ) {
				// return ware's destination halt is the start halt of the forward trip
				assert( halt_data[ first_transfer_halt.get_id() ].transfer.get_id() );
				return_ware->set_ziel( halt_data[ first_transfer_halt.get_id() ].transfer );
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

		FOR(vector_tpl<connection_t>, const& current_conn, current_node.halt->all_links[ware_catg_idx].connections) {

			// halt may have been deleted or joined => test if still valid
			if(  !current_conn.halt.is_bound()  ) {
				// removal seems better though ...
				continue;
			}

			// since these are pre-calculated, they should be always pointing to a valid ground
			// (if not, we were just under construction, and will be fine after 16 steps)
			const uint16 reachable_halt_id = current_conn.halt.get_id();

			if(  markers[ reachable_halt_id ]!=current_marker  ) {
				// Case : not processed before

				// indicate that this halt has been processed
				markers[ reachable_halt_id ] = current_marker;

				if(  current_conn.halt.is_bound()  &&  current_conn.is_transfer  &&  allocation_pointer<max_hops  ) {
					// Case : transfer halt
					const uint32 total_weight = current_halt_data.best_weight + current_conn.weight;

					if(  total_weight < best_destination_weight  ) {
						const bool overcrowded_transfer = no_routing_over_overcrowding  &&  ( current_halt_data.overcrowded  ||  current_conn.halt->is_overcrowded( ware_idx ) );

						halt_data[ reachable_halt_id ].best_weight = total_weight;
						halt_data[ reachable_halt_id ].destination = 0;
						halt_data[ reachable_halt_id ].depth       = current_halt_data.depth + 1u;
						halt_data[ reachable_halt_id ].transfer    = current_node.halt;
						halt_data[ reachable_halt_id ].overcrowded = overcrowded_transfer;
						overcrowded_nodes                         += overcrowded_transfer;

						allocation_pointer++;
						// as the next halt is not a destination add WEIGHT_MIN
						// TODO: (TBGR) use estimated time. Should include waiting time
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

			} // if not processed before
			else if(  halt_data[ reachable_halt_id ].best_weight!=0  &&  halt_data[ reachable_halt_id ].depth>0) {
				// Case : processed before but not in closed list : that is, in open list
				//        --> can only be destination halt or transfer halt
				//        or start halt (filter the latter out with the condition depth>0)

				uint32 total_weight = current_halt_data.best_weight + current_conn.weight;

				if(  total_weight<halt_data[ reachable_halt_id ].best_weight  &&  total_weight<best_destination_weight  &&  allocation_pointer<max_hops  ) {
					// new weight is lower than lowest weight --> create new node and update halt data
					const bool overcrowded_transfer = no_routing_over_overcrowding  &&  ( current_halt_data.overcrowded  ||  ( !halt_data[reachable_halt_id].destination  &&  current_conn.halt->is_overcrowded( ware_idx ) ) );

					halt_data[ reachable_halt_id ].best_weight = total_weight;
					// no need to update destination, as halt nature (as destination or transfer) will not change
					halt_data[ reachable_halt_id ].depth       = current_halt_data.depth + 1u;
					halt_data[ reachable_halt_id ].transfer    = current_node.halt;
					halt_data[ reachable_halt_id ].overcrowded = overcrowded_transfer;
					overcrowded_nodes                         += overcrowded_transfer;

					if(  halt_data[reachable_halt_id].destination  ) {
						best_destination_weight = total_weight;
					}
					else {
						// as the next halt is not a destination add WEIGHT_MIN
						// TODO: (TBGR) use estimated time. Should include waiting time
						total_weight += WEIGHT_MIN;
					}

					allocation_pointer++;
					open_list.insert( route_node_t(current_conn.halt, total_weight) );
				}
			} // else if not in closed list
		} // for each connection entry

		// indicate that the current halt is in closed list
		current_halt_data.best_weight = 0;
	}

	// if the loop ends, nothing was found
	ware.set_ziel( halthandle_t() );
	ware.clear_transit_halts();
	if(  return_ware  ) {
		return_ware->set_ziel( halthandle_t() );
		return_ware->clear_transit_halts();
	}
	return NO_ROUTE;
}


void haltestelle_t::search_route_resumable(  ware_t &ware   )
{
	const uint8 ware_catg_idx = ware.get_desc()->get_catg_index();

	// continue search if start halt and good category did not change
	const bool resume_search = last_search_origin == self  &&  ware_catg_idx == last_search_ware_catg_idx;

	if (!resume_search) {
		last_search_origin = self;
		last_search_ware_catg_idx = ware_catg_idx;
		open_list.clear();
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

	uint32 best_destination_weight = UINT32_MAX;

	// reset next transfer and destination halt to null -> if they remain null after search, no route can be found
	ware.set_ziel( halthandle_t() );
	ware.clear_transit_halts();
	// find suitable destination halts for the ware packet's target position
	const planquadrat_t *const plan = welt->access( ware.get_zielpos() );
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
				vector_tpl<halthandle_t> transit_halts;
				build_transit_halts_from_halt_data(transit_halts, halt);
				ware.set_transit_halts(transit_halts);
			}
		}
		// for all halts with halt_data.weight < explored_weight one of the best routes is found
		const uint32 explored_weight = open_list.empty()  ? UINT32_MAX : open_list.front().aggregate_weight;

		if (best_destination_weight <= explored_weight  &&  best_destination_weight < UINT32_MAX) {
			// we explored best route to this destination in last run
			// (any other not yet explored connection will have larger weight)
			// no need to search route for this ware
			return;
		}
	}
	// we start in this connected component
	uint16 const conn_comp = all_links[ ware_catg_idx ].catg_connected_component;

	// find suitable destination halt(s), if any
	for( uint8 h=0;  h<plan->get_haltlist_count();  ++h  ) {
		const halthandle_t halt = halt_list[h];
		if(  halt.is_bound()  &&  halt->is_enabled(ware_catg_idx)  ) {

			// test for connected component
			uint16 const dest_comp = halt->all_links[ ware_catg_idx ].catg_connected_component;
			if (dest_comp != UNDECIDED_CONNECTED_COMPONENT  &&  conn_comp != UNDECIDED_CONNECTED_COMPONENT  &&  conn_comp != dest_comp) {
				continue;
			}

			// initialisations for destination halts => save some checking inside search loop
			if(  markers[ halt.get_id() ]!=current_marker  ) {
				// first time -> initialise marker and all halt data
				markers[ halt.get_id() ] = current_marker;
				halt_data[ halt.get_id() ].best_weight = UINT32_MAX;
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
		const uint32 current_weight = current_node.aggregate_weight;
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
			vector_tpl<halthandle_t> transit_halts;
			build_transit_halts_from_halt_data(transit_halts, current_node.halt);
			ware.set_transit_halts(transit_halts);
			// update best_destination_weight to leave loop due to first check above
			best_destination_weight = current_weight;
			// if this destination halt is not a transfer halt -> do not proceed to process its reachable halt(s)
			if(  !current_node.halt->is_transfer(ware_catg_idx)  ) {
				continue;
			}
		}

		// not start halt, not transfer halt -> do not expand further
		if(  current_halt_data.depth > 0   &&  !current_node.halt->is_transfer(ware_catg_idx)  ) {
			continue;
		}

		if(  current_halt_data.depth > max_transfers  ) {
			// maximum transfer limit is reached -> do not add reachable halts to open list
			continue;
		}

		FOR(vector_tpl<connection_t>, const& current_conn, current_node.halt->all_links[ware_catg_idx].connections) {
			const uint16 reachable_halt_id = current_conn.halt.get_id();

			const uint32 total_weight = current_weight + current_conn.weight;

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
				halt_data[ reachable_halt_id ].transfer    = current_node.halt;

				if(  current_conn.is_transfer  &&  allocation_pointer<max_hops  ) {
					// Case : transfer halt
					allocation_pointer++;
					open_list.insert( route_node_t(current_conn.halt, total_weight) );
				}
			} // if not processed before
			else {
				// Case : processed before (or destination halt)
				//        -> need to check whether we can reach it with smaller weight
				if(  total_weight<halt_data[ reachable_halt_id ].best_weight  ) {
					// new weight is lower than lowest weight --> update halt data

					halt_data[ reachable_halt_id ].best_weight = total_weight;
					halt_data[ reachable_halt_id ].transfer    = current_node.halt;

					// for transfer/destination nodes create new node
					if ( (halt_data[ reachable_halt_id ].destination  ||  current_conn.is_transfer )  &&  allocation_pointer<max_hops ) {
						halt_data[ reachable_halt_id ].depth = current_halt_data.depth + 1u;
						allocation_pointer++;
						open_list.insert( route_node_t(current_conn.halt, total_weight) );
					}
				}
			} // else processed before
		} // for each connection entry
	}

	// clear destinations since we may want to do another search with the same current_marker
	FOR(vector_tpl<uint16>, const i, dest_indices) {
		halt_data[i].destination = false;
		if (halt_data[i].best_weight == UINT32_MAX) {
			// not processed -> reset marker
			--markers[i];
		}
	}
}


/* 
 * A helper function of search_route and search_route_resumable.
 * The consturcted vector is set to transit_halts.
 * The vector includes all transit halts and the final destination.
 */
void haltestelle_t::build_transit_halts_from_halt_data(vector_tpl<halthandle_t> &transit_halts, const halthandle_t destination) {
	halthandle_t transfer_halt = destination;
	transit_halts.append(transfer_halt);
	while(  halt_data[ transfer_halt.get_id() ].depth > 1   ) {
		transfer_halt = halt_data[ transfer_halt.get_id() ].transfer;
		transit_halts.insert_at(0, transfer_halt);
	}
}


/**
 * Found route and station uncrowded
 */
void haltestelle_t::add_pax_happy(int n)
{
	book(n, HALT_HAPPY);
	recalc_status();
}


/**
 * Station in walking distance
 */
void haltestelle_t::add_pax_walked(int n)
{
	book(n, HALT_WALKED);
}


/**
 * Station crowded
 */
void haltestelle_t::add_pax_unhappy(int n)
{
	book(n, HALT_UNHAPPY);
	recalc_status();
}


/**
 * Found no route
 */
void haltestelle_t::add_pax_no_route(int n)
{
	book(n, HALT_NOROUTE);
}



void haltestelle_t::liefere_an_fabrik(const ware_t& ware) const
{
	fabrik_t *const factory = fabrik_t::get_fab(ware.get_zielpos() );
	if(  factory  ) {
		factory->liefere_an(ware.get_desc(), ware.menge);
	}
}


/* retrieves a ware packet for any destination in the list
 * needed, if the factory in question wants to remove something
 */
bool haltestelle_t::recall_ware( ware_t& w, uint32 menge )
{
	w.menge = 0;
	slist_tpl<cargo_item_t> *warray = cargo[w.get_desc()->get_catg_index()];
	if(warray!=NULL) {
		FOR(slist_tpl<cargo_item_t>, & tmp, *warray) {
			ware_t& goods = tmp->goods;
			// skip empty entries
			if(goods.menge==0  ||  w.get_index()!=goods.get_index()  ||  w.get_zielpos()!=goods.get_zielpos()) {
				continue;
			}

			// not too much?
			if(goods.menge > menge) {
				// not all can be loaded
				goods.menge -= menge;
				w.menge = menge;
			}
			else {
				// leave an empty entry => joining will more often work
				w.menge = goods.menge;
				goods.menge = 0;
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


void haltestelle_t::fetch_goods_nearest_first(slist_tpl<ware_t> &load, const goods_desc_t *good_category, uint32 requested_amount, const vector_tpl<halthandle_t>& destination_halts) {
	slist_tpl<cargo_item_t> *wares = cargo[good_category->get_catg_index()];
	if(  !wares  ||  wares->empty()  ) {
		return;
	}
	// first iterate over the next stop, then over the ware
	// might be a little slower, but ensures that passengers to nearest stop are served first
	// this allows for separate high speed and normal service
	for(  uint32 i=0; i < destination_halts.get_count();  i++  ) {
		halthandle_t plan_halt = destination_halts[i];

		// REMOVED: The random offset will ensure that all goods have an equal chance to be loaded.
		for(  slist_tpl<cargo_item_t>::iterator tmp = wares->begin();  tmp!=wares->end();  tmp++) {
			ware_t& goods = tmp->get()->goods;
			// skip empty entries
			if(goods.menge==0) {
				continue;
			}

			// goods without route -> returning passengers/mail
			if(  !goods.get_zwischenziel().is_bound()  ) {
				search_route_resumable(goods);
				if (!goods.get_ziel().is_bound()) {
					// no route anymore
					goods.menge = 0;
					continue;
				}
			}

			// compatible car and right target stop?
			if(  goods.get_zwischenziel()==plan_halt  ) {
				if(  plan_halt->is_overcrowded( goods.get_index() )  ) {
					if (welt->get_settings().is_avoid_overcrowding() && goods.get_ziel() != plan_halt) {
						// do not go for transfer to overcrowded transfer stop
						continue;
					}
				}

				// not too much?
				ware_t neu(goods);
				if(  goods.menge > requested_amount  ) {
					// not all can be loaded
					neu.menge = requested_amount;
					goods.menge -= requested_amount;
					requested_amount = 0;
				}
				else {
					requested_amount -= goods.menge;
					// leave an empty entry => joining will more often work
					goods.menge = 0;
				}
				load.insert(neu);

				book(neu.menge, HALT_DEPARTED);
				resort_freight_info = true;

				if (requested_amount==0) {
					return;
				}
			}
		}

		// nothing there to load
	}
}


void haltestelle_t::fetch_goods_FIFO(slist_tpl<ware_t> &load, const goods_desc_t *good_category, uint32 requested_amount, const vector_tpl<halthandle_t>& destination_halts) {
	slist_tpl<cargo_item_t> *wares = cargo[good_category->get_catg_index()];
	if(  !wares  ||  wares->empty()  ) {
		return;
	}
	// The table to check if a specific halt is contained in the destination_halts vector quickly.
	// The vector size is 65536 because the halt id is 16 bit.
	bool dest_halts_table[65536];
	MEMZERON(dest_halts_table, 65536);
	FOR(vector_tpl<halthandle_t>, const& h, destination_halts) {
		dest_halts_table[h.get_id()] = true;
	}

	for(  slist_tpl<cargo_item_t>::iterator ware = wares->begin();  ware!=wares->end();  ) {
		ware_t& goods = ware->get()->goods;
		// empty entry -> remove
		if(goods.menge==0) {
			ware = wares->erase(ware);
			continue;
		}

		// goods without route -> returning passengers/mail
		if(  !goods.get_zwischenziel().is_bound()  ) {
			search_route_resumable(goods);
			if (!goods.get_ziel().is_bound()) {
				// no route anymore
				ware = wares->erase(ware);
				continue;
			}
		}

		// right target stop, and not overcrowding?
		// do not go for transfer to overcrowded transfer stop
		if(
			!dest_halts_table[goods.get_zwischenziel().get_id()]  ||
			(welt->get_settings().is_avoid_overcrowding()  &&
				goods.get_zwischenziel()->is_overcrowded( goods.get_index() )  &&
				goods.get_ziel() != goods.get_zwischenziel())
		) {
			ware++;
			continue;
		}

		// not too much?
		ware_t neu(goods);
		if(  goods.menge > requested_amount  ) {
			// not all can be loaded
			neu.menge = requested_amount;
			goods.menge -= requested_amount;
			requested_amount = 0;
		}
		else {
			requested_amount -= goods.menge;
			// leave an empty entry => joining will more often work
			goods.menge = 0;
			ware = wares->erase(ware);
		}
		load.insert(neu);

		book(neu.menge, HALT_DEPARTED);
		resort_freight_info = true;

		if (requested_amount==0) {
			return;
		}
	}
}


uint32 haltestelle_t::get_ware_summe(const goods_desc_t *wtyp) const
{
	int sum = 0;
	const slist_tpl<cargo_item_t> * warray = cargo[wtyp->get_catg_index()];
	if(warray!=NULL) {
		FOR(slist_tpl<cargo_item_t>, const& i, *warray) {
			if (wtyp->get_index() == i->goods.get_index()) {
				sum += i->goods.menge;
			}
		}
	}
	return sum;
}


uint32 haltestelle_t::get_ware_fuer_zielpos(const goods_desc_t *wtyp, const koord zielpos) const
{
	const slist_tpl<cargo_item_t> * warray = cargo[wtyp->get_catg_index()];
	if(warray!=NULL) {
		FOR(slist_tpl<cargo_item_t>, const& ware, *warray) {
			const ware_t& goods = ware->goods;
			if(wtyp->get_index()==goods.get_index()  &&  goods.get_zielpos()==zielpos) {
				return goods.menge;
			}
		}
	}
	return 0;
}


uint32 haltestelle_t::get_ware_fuer_zwischenziel(const goods_desc_t *wtyp, const halthandle_t zwischenziel) const
{
	uint32 sum = 0;
	const slist_tpl<cargo_item_t> * warray = cargo[wtyp->get_catg_index()];
	if(warray!=NULL) {
		FOR(slist_tpl<cargo_item_t>, const& ware, *warray) {
			const ware_t& goods = ware->goods;
			if(wtyp->get_index()==goods.get_index()  &&  goods.get_zwischenziel()==zwischenziel) {
				sum += goods.menge;
			}
		}
	}
	return sum;
}


bool haltestelle_t::vereinige_waren(const ware_t &ware)
{
	// merge cargos only when "load nearest first" policy is applied.
	const settings_t &settings = world()->get_settings();
	const bool* wefl = waiting_amount_exceeds_FIFO_limit.access(ware.get_desc()->get_catg_index());
	if(  settings.get_goods_routing_policy()!=GRP_NF_RC  &&  (wefl==NULL  ||  !*wefl)  ) {
		return false;
	}

	// pruefen ob die ware mit bereits wartender ware vereinigt werden kann
	slist_tpl<cargo_item_t> * warray = cargo[ware.get_desc()->get_catg_index()];
	if(  !warray  ) {
		return false;
	}
	FOR(slist_tpl<cargo_item_t>, & tmp_cargo, *warray) {
		ware_t& tmp = tmp_cargo->goods;
		// join packets with same destination
		if(ware.same_destination(tmp)) {
			if(  ware.get_zwischenziel().is_bound()  &&  ware.get_zwischenziel()!=self  ) {
				// update route if there is newer route
				tmp.set_transit_halts( ware.get_transit_halts() );
			}
			tmp.menge += ware.menge;
			resort_freight_info = true;
			return true;
		}
	}
	return false;
}



// put the ware into the internal storage
// take care of all allocation necessary
std::shared_ptr<halt_waiting_goods_t> haltestelle_t::add_goods_to_halt(halt_waiting_goods_t wg)
{
	// now we have to add the ware to the stop
	slist_tpl<cargo_item_t> * warray = cargo[wg.goods.get_desc()->get_catg_index()];
	if(warray==NULL) {
		// this type was not stored here before ...
		warray = new slist_tpl<cargo_item_t>();
		cargo[wg.goods.get_desc()->get_catg_index()] = warray;
	}
	resort_freight_info = true;
	cargo_item_t goods_ref = cargo_item_t(new halt_waiting_goods_t(wg));
	warray->append(goods_ref);

	// Add the goods to fresh_cargo_ids if needed
	if(  world()->get_settings().get_goods_routing_policy()==goods_routing_policy_t::GRP_FIFO_ET  ) {
		fresh_cargo[wg.goods.get_desc()->get_catg_index()].push_back(std::weak_ptr<halt_waiting_goods_t>(goods_ref));
	}
	return goods_ref;
}



/* same as liefere an, but there will be no route calculated,
 * since it hase be calculated just before
 * (execption: route contains us as intermediate stop)
 */
uint32 haltestelle_t::starte_mit_route(ware_t ware)
{
	if(ware.get_ziel()==self) {
		if(  ware.to_factory  ) {
			// muss an factory geliefert werden
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
	add_goods_to_halt(halt_waiting_goods_t(ware, welt->get_ticks()));

	return ware.menge;
}


uint32 haltestelle_t::liefere_an(ware_t ware)
{
	// no valid next stops?
	if(!ware.get_ziel().is_bound()  ||  !ware.get_zwischenziel().is_bound()) {
		// write a log entry and discard the goods
dbg->warning("haltestelle_t::liefere_an()","%d %s delivered to %s have no longer a route to their destination!", ware.menge, translator::translate(ware.get_name()), get_name() );
		return ware.menge;
	}

	// did we arrived?
	if(  welt->access(ware.get_zielpos())->is_connected(self)  ) {
		if(  ware.to_factory  ) {
			// muss an factory geliefert werden
			liefere_an_fabrik(ware);
		}
		else if(  ware.is_passenger()  ) {
			// arriving passenger may create pedestrians
			if(  welt->get_settings().get_show_pax()  ) {
				int menge = ware.menge;
				FOR( slist_tpl<tile_t>, const& i, tiles ) {
					if (menge <= 0) {
						break;
					}
					menge = generate_pedestrians(i.grund->get_pos(), menge);
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

	// not near enough => we may need to do a re-routing
	if(  ware.get_zwischenziel()==self  ) {
		ware.pop_first_transit_halts();
	}
	if(  is_route_search_needed(ware)  ) {
		halthandle_t old_target = ware.get_ziel();

		search_route_resumable(ware);
		if (!ware.get_ziel().is_bound()) {
			// target halt no longer there => delete and remove from fab in transit
			fabrik_t::update_transit( &ware, false );
			return ware.menge;
		}
		// try to join with existing freight only if target has changed
		if(old_target != ware.get_ziel()  &&  vereinige_waren(ware)) {
			return ware.menge;
		}
	}
	
	// add to internal storage
	add_goods_to_halt(halt_waiting_goods_t(ware, welt->get_ticks()));

	return ware.menge;
}


/**
 * @param[out] buf Goods description text (buf)
 */
void haltestelle_t::get_freight_info(cbuffer_t & buf)
{
	if(resort_freight_info) {
		// resort only inf absolutely needed ...
		resort_freight_info = false;
		buf.clear();

		for(unsigned i=0; i<goods_manager_t::get_max_catg_index(); i++) {
			if(  !cargo[i]  ) {
				continue;
			}
			vector_tpl<ware_t> wvector;
			FOR(slist_tpl<cargo_item_t>, & w, *cargo[i]) {
				wvector.append(w->goods);
			}
			freight_list_sorter_t::sort_freight(wvector, buf, (freight_list_sorter_t::sort_mode_t)sortierung, NULL, "waiting");
		}
	}
}



void haltestelle_t::get_short_freight_info(cbuffer_t & buf) const
{
	bool got_one = false;

	for(unsigned int i=0; i<goods_manager_t::get_count(); i++) {
		const goods_desc_t *wtyp = goods_manager_t::get_info(i);
		if(gibt_ab(wtyp)) {

			// ignore goods with sum=zero
			const int summe=get_ware_summe(wtyp);
			if(summe>0) {

				if(got_one) {
					buf.append(", ");
				}

				int max = get_capacity( min(i, 2) );
				buf.printf("%d(%d)%s %s", summe, max, translator::translate(wtyp->get_mass()), translator::translate(wtyp->get_name()));

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


void haltestelle_t::get_throughput_info(cbuffer_t& buf) const
{
	// check if more than 1 month of history
	int month = get_finance_history( 1, HALT_ARRIVED) == 0 ||
		get_finance_history( 1, HALT_DEPARTED) == 0 ? 0 : 1;
	
	// get throughput of this month (either this or previous month)
	auto throughput = get_finance_history( month, HALT_ARRIVED) <
		get_finance_history( month, HALT_DEPARTED) ?
		get_finance_history( month, HALT_ARRIVED) :
		get_finance_history( month, HALT_DEPARTED);

	// add info to buffer
	buf.printf("%d ", throughput);
	buf.append(translator::translate("transfers"));
	buf.append("\n");
}

void haltestelle_t::get_waiting_occupancy_info(cbuffer_t& buf) const
{
	// set the waiting values to 0
	bool got_one = false;
	int waiting[3] = {0, 0, 0};

	// find the waiting values and add all wares together
	for(unsigned int i=0; i<goods_manager_t::get_count(); i++) {
		const goods_desc_t *wtyp = goods_manager_t::get_info(i);
		if(gibt_ab(wtyp)) {

			// ignore goods with sum=zero
			const int sum=get_ware_summe(wtyp);
			
			switch (i) {
				case goods_manager_t::INDEX_PAS:
					waiting[0] = sum;
					break;
				case goods_manager_t::INDEX_MAIL:
					waiting[1] = sum;
					break;
				default:
					waiting[2] += sum;
			}
		}
	}

	// prepare the label
	for (uint8 i = 0; i < 3; i++) {
		if (get_capacity(i)) {

			// ignore goods with sum=zero
			const int sum=get_capacity(i);
			if(sum>0) {

				if(got_one) {
					buf.append(", ");
				}

				auto typ = i == 2 ? "ware" : goods_manager_t::get_info(i)->get_name();
				

				auto per = (double) waiting[i]/sum * 100;
				buf.printf("%.2f%% %s", per, translator::translate(typ));

				got_one = true;
			}
		}
	}

	// add label suffix
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


void haltestelle_t::open_info_window()
{
	create_win( new halt_info_t(self), w_info, magic_halt_info + self.get_id() );
}


sint64 haltestelle_t::calc_maintenance() const
{
	sint64 maintenance = 0;
	FOR(  slist_tpl<tile_t>,  const& i,  tiles  ) {
		if(  gebaeude_t* const gb = i.grund->find<gebaeude_t>()  ) {
			maintenance += welt->get_settings().maint_building * gb->get_tile()->get_desc()->get_level();
		}
	}
	return maintenance;
}



// changes this to a public transfer exchange stop
void haltestelle_t::change_owner( player_t *player, bool halt_only )
{
	// check if already public
	if(  owner == player  ) {
		return;
	}

	// change owner of halt
	player_t* const prev_owner = owner;
	owner = player;
	rebuild_connections();
	rebuild_linked_connections();
	rebuild_connected_components();

	// tell the world of it ...
	if(  player == welt->get_public_player()  &&  env_t::networkmode  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("%s at (%i,%i) now public stop."), get_name(), get_basis_pos().x, get_basis_pos().y );
		welt->get_message()->add_message( buf, get_basis_pos(), message_t::ai, PLAYER_FLAG|player->get_player_nr(), IMG_EMPTY );
	}

	if(  halt_only  ) {
		// do not change
		return;
	}

	// process every tile of stop
	slist_tpl<halthandle_t> joining;
	FOR(slist_tpl<tile_t>, const& i, tiles) {

		grund_t* const gr = i.grund;
		if(  gebaeude_t* gb = gr->find<gebaeude_t>()  ) {
			// change ownership
			player_t *gbplayer =gb->get_owner();
			gb->set_owner(player);
			gb->set_flag(obj_t::dirty);
			sint64 const monthly_costs = welt->get_settings().maint_building * gb->get_tile()->get_desc()->get_level();
			waytype_t const costs_type = gb->get_waytype();
			player_t::add_maintenance(gbplayer, -monthly_costs, costs_type);
			player_t::add_maintenance(player, monthly_costs, costs_type);

			// cost is computed as cst_make_public_months
			sint64 const cost = -welt->scale_with_month_length(monthly_costs * welt->get_settings().cst_make_public_months);
			player_t::book_construction_costs(gbplayer, cost, get_basis_pos(), costs_type);
			player_t::book_construction_costs(player, -cost, koord::invalid, costs_type);
		}

		// change way ownership
		bool has_been_announced = false;
		for(  int j=0;  j<2;  j++  ) {
			if(  weg_t *w=gr->get_weg_nr(j)  ) {
				// change ownership of way...
				player_t *wplayer = w->get_owner();
				if(  prev_owner==wplayer  ) {
					w->set_owner( player );
					w->set_flag(obj_t::dirty);
					sint32 cost = w->get_desc()->get_maintenance();
					// of tunnel...
					if(  tunnel_t *t=gr->find<tunnel_t>()  ) {
						t->set_owner( player );
						t->set_flag(obj_t::dirty);
						cost = t->get_desc()->get_maintenance();
					}
					waytype_t const financetype = w->get_desc()->get_finance_waytype();
					player_t::add_maintenance( wplayer, -cost, financetype);
					player_t::add_maintenance( player, cost, financetype);
					// multiplayer notification message
					if(  prev_owner != welt->get_public_player()  &&  env_t::networkmode  &&  !has_been_announced  ) {
						cbuffer_t buf;
						buf.printf( translator::translate("(%s) now public way."), w->get_pos().get_str() );
						welt->get_message()->add_message( buf, w->get_pos().get_2d(), message_t::ai, PLAYER_FLAG|player->get_player_nr(), IMG_EMPTY );
						has_been_announced = true; // one message is enough
					}
					cost = -welt->scale_with_month_length(cost * (player==welt->get_public_player())*welt->get_settings().cst_make_public_months );
					player_t::book_construction_costs(wplayer, cost, koord::invalid, financetype);
				}
			}
		}

		// make way object public if any suitable
		for(  uint8 i = 1;  i < gr->get_top();  i++  ) {
			if(  wayobj_t *const wo = obj_cast<wayobj_t>(gr->obj_bei(i))  ) {
				player_t *woplayer = wo->get_owner();
				if(  prev_owner==woplayer  ) {
					sint32 const cost = wo->get_desc()->get_maintenance();
					// change ownership
					wo->set_owner( player );
					wo->set_flag(obj_t::dirty);
					waytype_t const financetype = wo->get_desc()->get_waytype();
					player_t::add_maintenance( woplayer, -cost, financetype);
					player_t::add_maintenance( player, cost, financetype);
					player_t::book_construction_costs( woplayer, cost, koord::invalid, financetype);
				}
			}
		}
	}
}


// merge stop
void haltestelle_t::merge_halt( halthandle_t halt_merged )
{
	if(  halt_merged ==  self  ||  !halt_merged.is_bound()  ) {
		return;
	}

	halt_merged->change_owner( owner, false );

	// add statistics
	for(  int month=0;  month<MAX_MONTHS;  month++  ) {
		for(  int type=0;  type<MAX_HALT_COST;  type++  ) {
			financial_history[month][type] += halt_merged->financial_history[month][type];
			halt_merged->financial_history[month][type] = 0;
		}
	}

	slist_tpl<tile_t> tiles_to_join;
	FOR(slist_tpl<tile_t>, const& i, halt_merged->get_tiles()) {
		tiles_to_join.append(i);
	}

	while(  !tiles_to_join.empty()  ) {

		// ATTENTION: Anz convoi reservation to this tile will be lost!
		grund_t *gr = tiles_to_join.remove_first().grund;

		// transfer tiles to us
		halt_merged->rem_grund(gr);
		add_grund(gr);
	}

	assert(!halt_merged->existiert_in_welt());

	// transfer goods
	halt_merged->transfer_goods(self);
	destroy(halt_merged);

	recalc_basis_pos();

	// also rebuild our connections
	recalc_station_type();
	rebuild_connections();
	rebuild_linked_connections();
	rebuild_connected_components();
}
// [mod : shingoushori] mod : changes this to a private transfer exchange stop 3/3
// changes this to a private transfer exchange stop
// public_undertaking == true : public undertaking mode : no cost spend
void haltestelle_t::make_private_and_join( player_t *player, bool public_undertaking )
{
	player_t *const public_owner = welt->get_public_player();

	// check if already private
	if(  owner == player  ) {
		return;
	}

	// process every tile of stop
	slist_tpl<halthandle_t> joining;
	FOR(slist_tpl<tile_t>, const& i, tiles) {
											 grund_t* const gr = i.grund;
		gebaeude_t* gb = gr->find<gebaeude_t>();
		if(  gb  ) {
			player_t *const current_owner = gb->get_owner();
			// change ownership
			gb->set_owner(player);
			gb->set_flag(obj_t::dirty);
			sint64 const monthly_costs = welt->get_settings().maint_building * gb->get_tile()->get_desc()->get_level();
			waytype_t const costs_type = gb->get_waytype();
			player_t::add_maintenance(current_owner, -monthly_costs, costs_type);
			player_t::add_maintenance(player, monthly_costs, costs_type);

			// cost is computed and transfered to private player
			if(  !public_undertaking  ) {
				sint64 const cost = -welt->scale_with_month_length(monthly_costs * welt->get_settings().cst_make_public_months);
				player_t::book_construction_costs(current_owner, -cost, get_basis_pos(), costs_type);
				player_t::book_construction_costs(player, cost, koord::invalid, costs_type);
			}
		}

		// search for stops to join, starting with this tile
		const planquadrat_t *pl = welt->access(gr->get_pos().get_2d());
		for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
			halthandle_t my_halt = pl->get_boden_bei(i)->get_halt();
			if(  my_halt.is_bound()  &&  my_halt->get_owner()==player  &&  !joining.is_contained(my_halt)  ) {
				joining.append(my_halt);
			}
		}
		// search neighbouring tiles
		for( uint8 i=0;  i<8;  i++  ) {
			const planquadrat_t *pl2 = welt->access(gr->get_pos().get_2d()+koord::neighbours[i]);
			if(  pl2  ) {
				for(  uint8 i=0;  i < pl2->get_boden_count();  i++  ) {
					halthandle_t my_halt = pl2->get_boden_bei(i)->get_halt();
					if(  my_halt.is_bound()  &&  my_halt->get_owner()==player  &&  !joining.is_contained(my_halt)  ) {
						joining.append(my_halt);
					}
				}
			}
		}
	}

	// transfer ownership
	owner = player;

	// set name to name of first public stop
	if(  !joining.empty()  ) {
		set_name( joining.front()->get_name());
	}

	while(  !joining.empty()  ) {
		// join this halt with me
		halthandle_t halt = joining.remove_first();

		// now with the second stop
		while(  halt.is_bound()  &&  halt!=self  ) {
			// add statistics
			for(  int month=0;  month<MAX_MONTHS;  month++  ) {
				for(  int type=0;  type<MAX_HALT_COST;  type++  ) {
					financial_history[month][type] += halt->financial_history[month][type];
					halt->financial_history[month][type] = 0;	// to avoid counting twice
				}
			}

			// we always take the first remaining tile and transfer it => more safe
			koord3d t = halt->get_basis_pos3d();
			grund_t *gr = welt->lookup(t);

			// transfer tiles to us
			halt->rem_grund(gr);
			add_grund(gr);
			// and check for existence
			if(!halt->existiert_in_welt()) {
				// transfer goods
				halt->transfer_goods(self);

				// rebuild connections of all linked halts
				// otherwise these halts would lose connections and freight might get lost
				// (until complete rebuild_connections task is finished)
				halt->rebuild_linked_connections();

				destroy(halt);
			}
		}
	}

	// tell the world of it ...
	if(  player != public_owner  &&  env_t::networkmode  ) {
		cbuffer_t buf;
		buf.printf( translator::translate("%s at (%i,%i) now private stop."), get_name(), get_basis_pos().x, get_basis_pos().y );
		welt->get_message()->add_message( buf, get_basis_pos(), message_t::ai, PLAYER_FLAG|player->get_player_nr(), IMG_EMPTY );
	}

	recalc_station_type();
}

void haltestelle_t::transfer_goods(halthandle_t halt)
{
	if (!self.is_bound() || !halt.is_bound()) {
		return;
	}
	// transfer goods to halt
	for(uint8 i=0; i<goods_manager_t::get_max_catg_index(); i++) {
		const slist_tpl<cargo_item_t> * warray = cargo[i];
		if (warray) {
			FOR(slist_tpl<cargo_item_t>, const& j, *warray) {
				halt->add_goods_to_halt(*j.get());
			}
			delete cargo[i];
			cargo[i] = NULL;
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
		enables = 0;
		station_type = invalid;
	}

	const gebaeude_t* gb = gr->find<gebaeude_t>();
	const building_desc_t *desc=gb?gb->get_tile()->get_desc():NULL;

	if(  gr->is_water()  &&  gb  ) {
		// may happen around oil rigs and so on
		station_type |= dock;
		// for water factories
		if(desc) {
			// enabled the matching types
			enables |= desc->get_enabled();
			if (welt->get_settings().is_separate_halt_capacities()) {
				if(desc->get_enabled()&1) {
					capacity[0] += desc->get_capacity();
				}
				if(desc->get_enabled()&2) {
					capacity[1] += desc->get_capacity();
				}
				if(desc->get_enabled()&4) {
					capacity[2] += desc->get_capacity();
				}
			}
			else {
				// no separate capacities: sum up all
				capacity[0] += desc->get_capacity();
				capacity[2] = capacity[1] = capacity[0];
			}
		}
		return;
	}

	if(  desc==NULL  ) {
		// no desc, but solid ground?!?
		dbg->error("haltestelle_t::get_station_type()","ground belongs to halt but no desc?");
		return;
	}

	// there is only one loading bay ...
	switch (desc->get_type()) {
		case building_desc_t::dock:
		case building_desc_t::flat_dock: station_type |= dock;         break;

		// two ways on ground can only happen for tram tracks on streets, there buses and trams can stop
		case building_desc_t::generic_stop:
			switch (desc->get_extra()) {
				case road_wt:
					station_type |= (desc->get_enabled()&3)!=0 ? busstop : loadingbay;
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
						station_type |= (desc->get_enabled()&3)!=0 ? busstop : loadingbay;
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
	enables |= desc->get_enabled();
	if (welt->get_settings().is_separate_halt_capacities()) {
		if(desc->get_enabled()&1) {
			capacity[0] += desc->get_capacity();
		}
		if(desc->get_enabled()&2) {
			capacity[1] += desc->get_capacity();
		}
		if(desc->get_enabled()&4) {
			capacity[2] += desc->get_capacity();
		}
	}
	else {
		// no separate capacities: sum up all
		capacity[0] += desc->get_capacity();
		capacity[2] = capacity[1] = capacity[0];
	}
}

/*
 * recalculated the station type(s)
 * since it iterates over all ground, this is better not done too often, because line management and station list
 * queries this information regularly; Thus, we do this, when adding new ground
 * This recalculates also the capacity from the building levels ...
 */
void haltestelle_t::recalc_station_type()
{
	capacity[0] = 0;
	capacity[1] = 0;
	capacity[2] = 0;
	enables = 0;
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



int haltestelle_t::generate_pedestrians(koord3d pos, int count)
{
	pedestrian_t::generate_pedestrians_at(pos, count);
	for(int i=0; i<4 && count>0; i++) {
		pedestrian_t::generate_pedestrians_at(pos+koord::nesw[i], count);
	}
	return count;
}

// necessary to load pre0.99.13 savegames
void warenziel_rdwr(loadsave_t *file)
{
	koord ziel;
	ziel.rdwr(file);
	char tn[256];
	file->rdwr_str(tn, lengthof(tn));
}


void haltestelle_t::rdwr(loadsave_t *file)
{
	xml_tag_t h( file, "haltestelle_t" );

	sint32 owner_n;
	koord3d k;

	// will restore halthandle_t after loading
	if(file->is_version_atleast(110, 6)) {
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
		owner_n = welt->sp2num( owner );
	}

	if(file->is_version_less(99, 8)) {
		init_pos.rdwr( file );
	}
	file->rdwr_long(owner_n);

	if(file->is_version_less(88, 6)) {
		bool dummy;
		file->rdwr_bool(dummy); // pax
		file->rdwr_bool(dummy); // mail
		file->rdwr_bool(dummy); // ware
	}

	if(file->is_loading()) {
		owner = welt->get_player(owner_n);
		k.rdwr( file );
		while(k!=koord3d::invalid) {
			grund_t *gr = welt->lookup(k);
			if(!gr) {
				dbg->error("haltestelle_t::rdwr()", "invalid position %s", k.get_str() );
				gr = welt->lookup_kartenboden(k.get_2d());
				dbg->error("haltestelle_t::rdwr()", "setting to %s", gr->get_pos().get_str() );
			}
			// during loading and saving halts will be referred by their base position
			// so we may already be defined ...
			if(gr->get_halt().is_bound()) {
				dbg->warning( "haltestelle_t::rdwr()", "bound to ground twice at (%i,%i)!", k.x, k.y );
			}
			// now check, if there is a building -> we allow no longer ground without building!
			const gebaeude_t* gb = gr->find<gebaeude_t>();
			const building_desc_t *desc=gb?gb->get_tile()->get_desc():NULL;
			if(desc) {
				add_grund( gr, false /*do not relink factories now*/ );
				// verbinde_fabriken will be called in finish_rd
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

	// Used only in loading, for constructing fresh_cargo reference lists from the archived addresses.
	// The hash table of (archived address value, shared_ptr) of cargo items.
	inthashtable_tpl<sint64, cargo_item_t> archived_cargo_address_table;

	init_pos = tiles.empty() ? koord::invalid : tiles.front().grund->get_pos().get_2d();
	if(file->is_saving()) {
		const char *s;
		for(unsigned i=0; i<goods_manager_t::get_max_catg_index(); i++) {
			slist_tpl<cargo_item_t> *warray = cargo[i];
			if(warray) {
				s = "y"; // needs to be non-empty
				file->rdwr_str(s);
				if(  file->is_version_less(112, 3)  ) {
					uint16 count = warray->get_count();
					file->rdwr_short(count);
				}
				else {
					uint32 count = warray->get_count();
					file->rdwr_long(count);
				}
				FOR(slist_tpl<cargo_item_t>, & ware, *warray) {
					ware->goods.rdwr(file);
					if(  file->get_OTRP_version()>=36  ) {
						file->rdwr_long(ware->arrived_time);
					}
					if(  file->get_OTRP_version()>=40  ) {
						sint64 pointer = (sint64)ware.get(); 
						file->rdwr_longlong(pointer); // save the address
					}
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
			uint32 count;
			if(  file->is_version_less(112, 3)  ) {
				uint16 scount;
				file->rdwr_short(scount);
				count = scount;
			}
			else {
				file->rdwr_long(count);
			}
			if(count>0) {
				for(  uint32 i = 0;  i < count;  i++  ) {
					// add to internal storage (use this function, since the old categories were different)
					ware_t ware(file);
					uint32 arrived_time = INVALID_CARGO_ARRIVED_TIME;
					if(  file->get_OTRP_version()>=36  ) {
						file->rdwr_long(arrived_time);
					}
					sint64 archived_address;
					if(  file->get_OTRP_version()>=40  ) {
						file->rdwr_longlong(archived_address);
					}
					if(  ware.get_desc()  &&  ware.menge>0  &&  welt->is_within_limits(ware.get_zielpos())  ) {
						cargo_item_t goods_ref = add_goods_to_halt(halt_waiting_goods_t(ware, arrived_time));
						if(  file->get_OTRP_version()>=40  ) {
							archived_cargo_address_table.put(archived_address, goods_ref);
						}
						// restore in-transit information
						fabrik_t::update_transit( &ware, true );
					}
					else if(  ware.menge>0  ) {
						if(  ware.get_desc()  ) {
							dbg->error( "haltestelle_t::rdwr()", "%i of %s to %s ignored!", ware.menge, ware.get_name(), ware.get_zielpos().get_str() );
						}
						else {
							dbg->error( "haltestelle_t::rdwr()", "%i of unknown to %s ignored!", ware.menge, ware.get_zielpos().get_str() );
						}
					}
				}
			}
			file->rdwr_str(s, lengthof(s));
		}

		// old games save the list with stations
		// however, we have to rebuilt them anyway for the new format
		if(file->is_version_less(99, 13)) {
			uint16 count;
			file->rdwr_short(count);
			for(int i=0; i<count; i++) {
				warenziel_rdwr(file);
			}
		}

	}

	if(  file->is_version_atleast(111, 1)  ) {
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

	// read and write departure slots.
	if(  file->get_OTRP_version()>=24  ) {
		for(uint8 idx=0; idx<DST_SIZE; idx++) {
			uint32 n = departure_slot_table[idx].get_count();
			file->rdwr_long(n);
			slist_tpl<departure_t>::iterator i = departure_slot_table[idx].begin();
			if(  file->is_loading()  ) {
				departure_slot_table[idx].clear();
			}
			for(uint8 k=0; k<n; k++) {
				departure_t d = file->is_loading() ? departure_t() : *i;
				file->rdwr_long(d.arr_tick);
				file->rdwr_long(d.dep_tick);
				file->rdwr_long(d.exp_tick);
				if(  file->get_OTRP_version()>=27  ) {
					file->rdwr_byte(d.stop_index);
				} else {
					d.stop_index = 0;
				}
				convoi_t::rdwr_convoihandle_t(file, d.cnv);
				if(  file->is_loading()  ) {
					departure_slot_table[idx].append(d);
				} else {
					i++; // increment iterator
				}
			}
		}
	}

	// read and write fresh_cargo
	std::function<void(loadsave_t*, sint64&)> rdwr_longlong_item = [](loadsave_t *file, sint64 &v) {
		file->rdwr_longlong(v);
	};
	if(  file->is_loading()  ) {
		FOR(std::vector<std::list<std::weak_ptr<halt_waiting_goods_t>>>, &list, fresh_cargo) {
			list.clear();
		}
	}
	if(  file->get_OTRP_version()>=40  ) {
		if(  file->is_loading()  ) {
			uint8 list_count;
			file->rdwr_byte(list_count);
			for(uint8 i=0; i<list_count; i++) {
				vector_tpl<sint64> ref_addresses;
				file->rdwr_vector(ref_addresses, rdwr_longlong_item);
				FOR(vector_tpl<sint64>, &addr, ref_addresses) {
					if(  cargo_item_t* ptr = archived_cargo_address_table.access(addr)  ) {
						fresh_cargo[i].push_back(std::weak_ptr(*ptr));
					}
				}
			}
		} else {
			uint8 list_count = fresh_cargo.size();
			file->rdwr_byte(list_count);
			FOR(std::vector<std::list<std::weak_ptr<halt_waiting_goods_t>>>, &list, fresh_cargo) {
				vector_tpl<sint64> ref_addresses;
				FOR(std::list<std::weak_ptr<halt_waiting_goods_t>>, &i, list) {
					std::shared_ptr<halt_waiting_goods_t> ptr = i.lock();
					if(  ptr  ) {
						sint64 pointer = (sint64)ptr.get();
						ref_addresses.append(pointer);
					}
				}
				file->rdwr_vector(ref_addresses, rdwr_longlong_item);
			}
		}
	}

	if(  file->get_OTRP_version()>=42  ) {
		file->rdwr_byte(flags);
	}
}



void haltestelle_t::finish_rd()
{
	verbinde_fabriken();

	stale_convois.clear();
	stale_lines.clear();
	// fix good destination coordinates
	for(unsigned i=0; i<goods_manager_t::get_max_catg_index(); i++) {
		if(cargo[i]) {
			slist_tpl<cargo_item_t> * warray = cargo[i];
			FOR(slist_tpl<cargo_item_t>, & j, *warray) {
				j->goods.finish_rd(welt);
			}
		}
	}

	// handle name for old stations which don't exist in kartenboden
	// also recover from stations without tiles (from broken savegames)
	grund_t* bd = welt->lookup(get_basis_pos3d());
	if(bd!=NULL) {

		// what kind of station here?
		recalc_station_type();

		if(  !bd->get_flag(grund_t::has_text)  ) {
			// restore label and bridges
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
			if(  all_names.get(current_name).is_bound()  &&  fabrik_t::get_fab(get_basis_pos())==NULL  ) {
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
		for (size_t k = MAX_MONTHS; k-- != 0;) {
			financial_history[k][j] = 0;
		}
	}
}



/**
 * Calculates a status color for status bars
 */
void haltestelle_t::recalc_status()
{
	status_color = color_idx_to_rgb(financial_history[0][HALT_CONVOIS_ARRIVED] > 0 ? COL_GREEN : COL_YELLOW);

	// since the status is ordered ...
	uint8 status_bits = 0;

	MEMZERO(overcrowded);

	uint64 total_sum = 0;
	if(get_pax_enabled()) {
		const uint32 max_ware = get_capacity(0);
		total_sum += get_ware_summe(goods_manager_t::passengers);
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

	if(get_mail_enabled()) {
		const uint32 max_ware = get_capacity(1);
		const uint32 mail = get_ware_summe(goods_manager_t::mail);
		total_sum += mail;
		if(mail>max_ware) {
			status_bits |= mail>max_ware+200 ? 2 : 1;
			overcrowded[0] |= 2;
		}
	}

	// now for all goods
	if(status_color!=color_idx_to_rgb(COL_RED)  &&  get_ware_enabled()) {
		const uint8  count = goods_manager_t::get_count();
		const uint32 max_ware = get_capacity(2);
		for(  uint32 i = 3;  i < count;  i++  ) {
			goods_desc_t const* const wtyp = goods_manager_t::get_info(i);
			const uint32 ware_sum = get_ware_summe(wtyp);
			total_sum += ware_sum;
			if(ware_sum>max_ware) {
				status_bits |= ware_sum > max_ware + 32 ? 2 : 1; // for now report only serious overcrowding on transfer stops
				overcrowded[wtyp->get_index()/8] |= 1<<(wtyp->get_index()%8);
			}
		}
	}

	// take the worst color for status
	if(  status_bits  ) {
		status_color = color_idx_to_rgb(status_bits&2 ? COL_RED : COL_ORANGE);
	}
	else {
		status_color = color_idx_to_rgb((financial_history[0][HALT_WAITING]+financial_history[0][HALT_DEPARTED] == 0) ? COL_YELLOW : COL_GREEN);
	}

	financial_history[0][HALT_WAITING] = total_sum;

	// update waiting_amount_exceeds_FIFO_limit
	const uint32 fifo_limit = world()->get_settings().get_waiting_limit_for_first_come_first_serve();
	for(  uint32 i = 0;  i<goods_manager_t::get_max_catg_index();  i++  ) {
		if(  cargo[i] != NULL  ) {
			const uint32 ware_sum = get_ware_summe(goods_manager_t::get_info(i));
			waiting_amount_exceeds_FIFO_limit.set(i, ware_sum > fifo_limit);
		}
	}
}



/**
 * Draws some nice colored bars giving some status information
 */
void haltestelle_t::display_status(sint16 xpos, sint16 ypos)
{
	// ignore freight that cannot reach to this station
	sint16 count = 0;
	for(  uint16 i = 0;  i < goods_manager_t::get_count();  i++  ) {
		if(  i == 2  ) {
			continue; // ignore freight none
		}
		if(  gibt_ab( goods_manager_t::get_info(i) )  ) {
			count++;
		}
	}
	ypos += -D_WAITINGBAR_WIDTH - LINESPACE/6;

	if(  count != last_bar_count  ) {
		// bars will shift x positions, mark entire station bar region dirty
		scr_coord_val max_bar_height = 0;
		for(  sint16 i = 0;  i < last_bar_count;  i++  ) {
			if(  last_bar_height[i] > max_bar_height  ) {
				max_bar_height = last_bar_height[i];
			}
		}
		const scr_coord_val x = xpos - (last_bar_count * D_WAITINGBAR_WIDTH - get_tile_raster_width()) / 2;
		mark_rect_dirty_wc( x - 1 - D_WAITINGBAR_WIDTH, ypos, x + last_bar_count * D_WAITINGBAR_WIDTH + 12 - 2, ypos - 11 );

		// reset bar heights for new count
		last_bar_height.clear();
		last_bar_height.resize( count );
		for(  sint16 i = 0;  i < count;  i++  ) {
			last_bar_height.append(0);
		}
		last_bar_count = count;
	}

	xpos -= (count * D_WAITINGBAR_WIDTH - get_tile_raster_width()) / 2;
	const int x = xpos;

	sint16 bar_height_index = 0;
	uint32 max_capacity;
	for(  uint8 i = 0;  i < goods_manager_t::get_count();  i++  ) {
		if(  i == 2  ) {
			continue; // ignore freight none
		}
		const goods_desc_t *wtyp = goods_manager_t::get_info(i);
		if(  gibt_ab( wtyp )  ) {
			if(  i < 2  ) {
				max_capacity = get_capacity(i);
			}
			else {
				max_capacity = get_capacity(2);
			}
			const uint32 sum = get_ware_summe( wtyp );
			uint32 v = min( sum, max_capacity );
			if(  max_capacity > 512  ) {
				v = 2 + (v * 128) / max_capacity;
			}
			else {
				v = (v / 4) + 2;
			}

			display_fillbox_wh_clip_rgb( xpos, ypos - v - 1, 1, v, color_idx_to_rgb( COL_GREY4 ), false );
			display_fillbox_wh_clip_rgb( xpos + 1, ypos - v - 1, D_WAITINGBAR_WIDTH - 2, v, wtyp->get_color(), false );
			display_fillbox_wh_clip_rgb( xpos + D_WAITINGBAR_WIDTH - 1, ypos - v - 1, 1, v, color_idx_to_rgb( COL_GREY1 ), false );

			// show up arrow for capped values
			if(  sum > max_capacity  ) {
				display_fillbox_wh_clip_rgb( xpos + (D_WAITINGBAR_WIDTH / 2) - 1, ypos - v - 6, 2, 4, color_idx_to_rgb( COL_WHITE ), false );
				display_fillbox_wh_clip_rgb( xpos + (D_WAITINGBAR_WIDTH / 2) - 2, ypos - v - 5, 4, 1, color_idx_to_rgb( COL_WHITE ), false );
				v += 5; // for marking dirty
			}

			if(  last_bar_height[bar_height_index] != (scr_coord_val)v  ) {
				if(  (scr_coord_val)v > last_bar_height[bar_height_index]  ) {
					// bar will be longer, mark new height dirty
					mark_rect_dirty_wc( xpos, ypos - v - 1, xpos + D_WAITINGBAR_WIDTH, ypos - 1 );
				}
				else {
					// bar will be shorter, mark old height dirty
					mark_rect_dirty_wc( xpos, ypos - last_bar_height[ bar_height_index ] - 1, xpos + D_WAITINGBAR_WIDTH, ypos - 1 );
				}
				last_bar_height[bar_height_index] = v;
			}

			bar_height_index++;
			xpos += D_WAITINGBAR_WIDTH;
		}
	}

	// status color box below
	bool dirty = false;
	if(  get_status_farbe() != last_status_color  ) {
		last_status_color = get_status_farbe();
		dirty = true;
	}
	display_fillbox_wh_clip_rgb( x - 1 - 4, ypos, count * D_WAITINGBAR_WIDTH + 12 - 2, D_WAITINGBAR_WIDTH, get_status_farbe(), dirty );
}



bool haltestelle_t::add_grund(grund_t *gr, bool relink_factories)
{
	assert(gr!=NULL);

	// new halt?
	if(  tiles.is_contained(gr)  ) {
		return false;
	}

	koord pos = gr->get_pos().get_2d();
	add_to_station_type( gr );
	gr->set_halt( self );
	tiles.append( gr );

	// add to hashtable
	if (all_koords) {
		sint32 n = get_halt_key( gr->get_pos(), welt->get_size().y );
		all_koords->set( n, self );
	}

	// appends this to the ground
	// after that, the surrounding ground will know of this station
	bool insert_unsorted = !relink_factories;
	uint16 const cov = welt->get_settings().get_station_coverage();
	for (int y = -cov; y <= cov; y++) {
		for (int x = -cov; x <= cov; x++) {
			koord p=pos+koord(x,y);
			planquadrat_t *plan = welt->access(p);
			if(plan) {
				plan->add_to_haltlist( self, insert_unsorted);
				plan->get_kartenboden()->set_flag(grund_t::dirty);
			}
		}
	}

	// since suddenly other factories may be connect to us too
	if (relink_factories) {
		verbinde_fabriken();
	}

	// check if we have to register line(s) and/or lineless convoy(s) which serve this halt
	vector_tpl<linehandle_t> check_line(0);

	// public halt: must iterate over all players lines / convoys
	bool public_halt = get_owner() == welt->get_public_player()  ||  is_other_player_connection_allowed();

	uint8 const pl_min = public_halt ? 0                : get_owner()->get_player_nr();
	uint8 const pl_max = public_halt ? MAX_PLAYER_COUNT : get_owner()->get_player_nr()+1;
	// iterate over all lines (public halt: all lines, other: only player's lines)
	for(  uint8 i=pl_min;  i<pl_max;  i++  ) {
		if(  player_t *player = welt->get_player(i)  ) {
			player->simlinemgmt.get_lines(simline_t::line, &check_line);
			FOR(  vector_tpl<linehandle_t>, const j, check_line  ) {
				// only add unknown lines
				if(  !registered_lines.is_contained(j)  &&  j->count_convoys() > 0  ) {
					FOR(  minivec_tpl<schedule_entry_t>, const& k, j->get_schedule()->entries  ) {
						if(  get_stoppable_halt(k.pos, player) == self  ) {
							registered_lines.append(j);
							break;
						}
					}
				}
			}
		}
	}
	// iterate over all convoys
	FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
		// only check lineless convoys which have matching ownership and which are not yet registered
		if(  !cnv->get_line().is_bound()  &&  (public_halt  ||  cnv->get_owner()==get_owner())  &&  !registered_convoys.is_contained(cnv)  ) {
			if(  const schedule_t *const schedule = cnv->get_schedule()  ) {
				FOR(minivec_tpl<schedule_entry_t>, const& k, schedule->entries) {
					if (get_stoppable_halt(k.pos, cnv->get_owner()) == self) {
						registered_convoys.append(cnv);
						break;
					}
				}
			}
		}
	}

	// This entire loop is just for the assertion below.
	// Consider deleting the assertion --neroden
	bool grund_is_where_it_should_be = false;
	const planquadrat_t* plan = welt->access(pos);
	for(  uint8 i=0;  i < plan->get_boden_count();  i++  ) {
		const grund_t* found_gr = plan->get_boden_bei(i);
		if (found_gr == gr) {
			grund_is_where_it_should_be = true;
			break;
		}
	}
	if (  !grund_is_where_it_should_be || gr->get_halt() != self || !gr->is_halt()  ) {
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
		delete lb;
		set_name( station_name_to_transfer );
	}

	bool remove_halt = true;
	planquadrat_t *pl = welt->access( gr->get_pos().get_2d() );
	if(pl) {
		// no longer present on this level
		gr->set_halt(halthandle_t());
		// still connected elsewhere?
		for(unsigned i=0;  i<pl->get_boden_count();  i++  ) {
			if(pl->get_boden_bei(i)->get_halt()==self) {
				// still connected with other ground => do not remove from plan ...
				remove_halt = false;
				break;
			}
		}
	}

	if (remove_halt) {
		// otherwise remove from plan ...
		if (pl) {
			pl->get_kartenboden()->set_flag(grund_t::dirty);
		}

		uint16 const cov = welt->get_settings().get_station_coverage();
		for (int y = -cov; y <= cov; y++) {
			for (int x = -cov; x <= cov; x++) {
				planquadrat_t *pl = welt->access( gr->get_pos().get_2d()+koord(x,y) );
				if(pl) {
					pl->remove_from_haltlist(self);
					pl->get_kartenboden()->set_flag(grund_t::dirty);
				}
			}
		}

		// factory reach may have been changed ...
		verbinde_fabriken();
	}

	// needs to be done, if this was a dock
	recalc_station_type();

	// remove lines eventually
	for(  size_t j = registered_lines.get_count();  j-- != 0;  ) {
		bool ok = false;
		FOR(  minivec_tpl<schedule_entry_t>, const& k, registered_lines[j]->get_schedule()->entries  ) {
			if(  get_stoppable_halt(k.pos, registered_lines[j]->get_owner()) == self  ) {
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

	// remove registered lineless convoys as well
	for(  size_t j = registered_convoys.get_count();  j-- != 0;  ) {
		bool ok = false;
		FOR(  minivec_tpl<schedule_entry_t>, const& k, registered_convoys[j]->get_schedule()->entries  ) {
			if(  get_stoppable_halt(k.pos, registered_convoys[j]->get_owner()) == self  ) {
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


/* marks a coverage area
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
 */
bool haltestelle_t::find_free_position(const waytype_t w,convoihandle_t cnv,const obj_t::typ d) const
{
	// iterate over all tiles
	// for road, we have to consider passing lane.
	if(  w==road_wt  ) {
		FOR(slist_tpl<tile_t>, const& i, tiles) {
			if(  !i.reservation[0].is_bound()  ||  !i.reservation[1].is_bound()  ) {
				// possibly there is empty slots.
				grund_t* const gr = i.grund;
				assert(gr);
				if(  get_empty_lane(gr, cnv)!=0  ) {
					return true;
				}
			}
		}
		// no empty tile.
		return false;
	}
	// for other waytypes...
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (i.reservation[0] == cnv || !i.reservation[0].is_bound()) {
			// not reserved
			grund_t* const gr = i.grund;
			assert(gr);
			// found a stop for this waytype but without object d ...
			if(gr->hat_weg(w)  &&  gr->suche_obj(d)==NULL) {
				// not occupied
				return true;
			}
		}
	}
	return false;
}


/* reserves a position (caution: railblocks work differently!
 */
bool haltestelle_t::reserve_position(grund_t *gr,convoihandle_t cnv)
{
	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i != tiles.end()) {
		if (i->reservation[0] == cnv  ||  i->reservation[1] == cnv) {
//DBG_MESSAGE("haltestelle_t::reserve_position()","gr=%d,%d already reserved by cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
			return true;
		}
		// not reserved
		vehicle_t const& v = *cnv->front();
		// road vehicles need special process to consider passing lane.
		if(v.get_waytype()==road_wt) {
			uint8 empty_lane = get_empty_lane(gr, cnv);
			if((empty_lane&1)!=0) {
				i->reservation[0] = cnv;
				return true;
			} else if((empty_lane&2)!=0) {
				i->reservation[1] = cnv;
				return true;
			} else {
				return false;
			}
		}
		// for other vehicle types...
		if (!i->reservation[0].is_bound()) {
			grund_t* gr = i->grund;
			if(gr) {
				// found a stop for this waytype but without object d ...
				if (gr->hat_weg(v.get_waytype()) && !gr->suche_obj(v.get_typ())) {
					// not occupied
//DBG_MESSAGE("haltestelle_t::reserve_position()","success for gr=%i,%i cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
					i->reservation[0] = cnv;
					return true;
				}
			}
		}
	}
//DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
	return false;
}


/** frees a reserved  position (caution: railblocks work differently!
 */
bool haltestelle_t::unreserve_position(grund_t *gr, convoihandle_t cnv)
{
	slist_tpl<tile_t>::iterator i = std::find(tiles.begin(), tiles.end(), gr);
	if (i != tiles.end()) {
		for(uint8 k=0; k<2; k++) {
			if (i->reservation[k] == cnv) {
				i->reservation[k] = convoihandle_t();
				return true;
			}
		}
	}
DBG_MESSAGE("haltestelle_t::unreserve_position()","failed for gr=%p",gr);
	return false;
}


/** can a convoi reserve this position?
 */
bool haltestelle_t::is_reservable(const grund_t *gr, convoihandle_t cnv) const
{
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (gr == i.grund) {
			if (i.reservation[0] == cnv) {
DBG_MESSAGE("haltestelle_t::is_reservable()","gr=%d,%d already reserved by cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
				return true;
			}
			// not reserved
			if (!i.reservation[0].is_bound()) {
				// found a stop for this waytype but without object d ...
				vehicle_t const& v = *cnv->front();
				if (gr->hat_weg(v.get_waytype()) && !gr->suche_obj(v.get_typ())) {
					// not occupied
					return true;
				}
			}
			return false;
		}
	}
DBG_MESSAGE("haltestelle_t::reserve_position()","failed for gr=%i,%i, cnv=%d",gr->get_pos().x,gr->get_pos().y,cnv.get_id());
	return false;
}

/* haltestelle_t::is_reservable for road vehicles
 * The returned value is a bit pattern.
 * 0 represents that the both lane is filled.
 * 1 represents that the traffic lane is empty.
 * 2 represents that the passing lane is empty.
 * 3 represents that the both lane is empty.
 * @author THLeaderH
 */
uint8 haltestelle_t::get_empty_lane(const grund_t *gr, convoihandle_t cnv) const {
	FOR(slist_tpl<tile_t>, const& i, tiles) {
		if (  gr == i.grund  ) {
			if (  i.reservation[0] == cnv  ) {
				// already reserved the traffic lane.
				return 1;
			}
			if (  i.reservation[1] == cnv  ) {
				// already reserved the passing lane.
				return 2;
			}
			// not reserved
			const strasse_t* str = dynamic_cast<strasse_t*> (gr->get_weg(road_wt));
			if(  !str  ) { return 0; }
			const overtaking_mode_t overtaking_mode = str->get_overtaking_mode();
			uint8 empty = 0;
			for (uint8 k=1; k<=2; k++) {
				if(  overtaking_mode!=halt_mode  &&  k==2  ) { break; }
				if(  !i.reservation[k-1].is_bound()  ) {
					// check whether this place is not occupied...
					// since up to 2 vehicles can exist on one tile, we have to check all objects on the tile.
					empty |= k; // raise the empty bit
					for(uint8 h=0; h<gr->get_top(); h++) {
						if(  road_vehicle_t* rv = obj_cast<road_vehicle_t> (gr->obj_bei(h))  ) {
							bool is_overtaking = rv->get_convoi()->is_overtaking();
							// If a vehicle exists on the same lane, drop the empty bit.
							if(  k==1 ? !is_overtaking : is_overtaking  ) { empty &= ~k; }
						}
						else if(  private_car_t* pc = obj_cast<private_car_t> (gr->obj_bei(h))  ) {
							bool is_overtaking = pc->is_overtaking();
							if(  k==1 ? !is_overtaking : is_overtaking  ) { empty &= ~k; }
						}
					}
				}
			}
			return empty;
		}
	}
	return 0;
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

/* check if the station given is covered by this station */
bool haltestelle_t::is_halt_covered(const halthandle_t &halt) const
{
	uint16 const cov = welt->get_settings().get_station_coverage();
	FOR(slist_tpl<tile_t>, const& i, halt->get_tiles()) {
		if(  gebaeude_t* const gb = i.grund->find<gebaeude_t>()  ) {
			if ( koord_distance( gb->get_pos().get_2d(), get_next_pos( gb->get_pos().get_2d() )) <= cov ) {
				return true;
			}
		}
	}
	return false;
}


bool haltestelle_t::book_departure (uint32 arr_tick, uint32 dep_tick, uint32 exp_tick, convoihandle_t cnv) {
	// check if departure_slot_group_id is properly initialized.
	if(  cnv->get_line().is_bound()  &&  cnv->get_line()->get_schedule()->get_departure_slot_group_id()==0  ) {
		dbg->error("haltestelle_t::book_departure", "departure_slot_group_id is zero for %s", cnv->get_name());
	}
	const uint8 idx = dep_tick % DST_SIZE;
	slist_tpl<departure_t>::iterator i = departure_slot_table[idx].begin();
	const uint8 stop_index = cnv->get_schedule()->get_current_stop_exluding_depot();
	const uint32 current_ticks = welt->get_ticks();
	while(  i!=departure_slot_table[idx].end()  ) {
		if(  is_first_ticks_bigger(welt->get_ticks(), i->exp_tick)  ||  !i->cnv.is_bound()  ) {
			// This entry is already obsolete. Just remove it.
			i = departure_slot_table[idx].erase(i);
			continue;
		}
		if(  is_first_ticks_bigger(i->dep_tick, current_ticks) && (world()->lookup(i->cnv->get_pos())->get_halt() != self)  ) {
			// The convoy is not on this halt while the convoy is yet to depart. Handle it as an invalid.
			i = departure_slot_table[idx].erase(i);
			continue;
		}
		if(  i->cnv==cnv  &&  i->arr_tick==arr_tick  ) {
			// The requested slot is already reserved by this convoy.
			return true;
		}
		const linehandle_t line_a = i->cnv->get_line();
		const linehandle_t line_b = cnv->get_line();
		if(
			i->dep_tick==dep_tick  &&
			line_a.is_bound()  &&  line_b.is_bound()  &&
			line_a->get_schedule()->get_departure_slot_group_id()==line_b->get_schedule()->get_departure_slot_group_id()  &&
			i->stop_index==stop_index
		) {
			// The slot is already reserved by other convoy.
			return false;
		}
		i++;
	}
	// reserve the slot.
	departure_t dep(arr_tick, dep_tick, exp_tick, stop_index, cnv);
	departure_slot_table[idx].insert(departure_slot_table[idx].begin(), dep);
	return true;
}


bool haltestelle_t::erase_departure(uint32 dep_tick, convoihandle_t cnv) {
	const uint8 idx = dep_tick % DST_SIZE;
	slist_tpl<departure_t>::iterator i = departure_slot_table[idx].begin();
	// find and remove the requested departure slot
	while(  i!=departure_slot_table[idx].end()  ) {
		if(  i->cnv==cnv  &&  i->dep_tick==dep_tick  ) {
			departure_slot_table[idx].erase(i);
			return true;
		}
		i++;
	}
	return false; // we cannot find the requested departure slot.
}


bool haltestelle_t::is_departure_booked(uint32 dep_tick, uint8 stop_index, linehandle_t line) const {
	const uint8 idx = dep_tick % DST_SIZE;
	slist_tpl<departure_t>::const_iterator i = departure_slot_table[idx].begin();
	while(  i!=departure_slot_table[idx].end()  ) {
		if(  i->dep_tick==dep_tick  &&  i->stop_index==stop_index  &&  i->cnv->get_line()==line  ) {
			return true;
		}
		i++;
	}
	return false;
}


// A subroutine of calc_destination_halt.
// Returns if the schedule entry whose journey time is not registered exists.
bool unregistered_journey_time_exists(const schedule_t* schedule, player_t* player) {
	for(uint8 i=1; i<schedule->get_count()+1; i++) {
		const schedule_entry_t& this_entry = schedule->entries[i%schedule->get_count()];
		const schedule_entry_t& prev_entry = schedule->entries[i-1];
		if(  
			this_entry.get_median_journey_time()>0  || // valid record exists
			this_entry.pos==prev_entry.pos  ||
			haltestelle_t::get_stoppable_halt(this_entry.pos, player)==haltestelle_t::get_stoppable_halt(prev_entry.pos, player)
		) {
			// valid record exists.
			continue;
		}
		return true;
	}
	return false;
}


void haltestelle_t::calc_destination_halt(inthashtable_tpl<uint8, vector_tpl<halthandle_t>> &destination_halts, const vector_tpl<reachable_halt_t> &reachable_halts, const minivec_tpl<uint8> &goods_category_indexes, convoihandle_t cnv) {
	// initialize destination_halts
	destination_halts.clear();
	FOR(const minivec_tpl<uint8>, const& i, goods_category_indexes) {
		destination_halts.put(i);
	}

	traveler_t traveler;
	if(  cnv->get_line().is_bound()  ) {
		traveler = cnv->get_line();
	} else {
		traveler = cnv;
	}
	const bool is_tbgr_enabled = welt->get_settings().get_goods_routing_policy()==goods_routing_policy_t::GRP_FIFO_ET;
	const schedule_t* schedule = std::visit(
		[&](const auto &t) { return t->get_schedule(); }, traveler
	);
	if(  !is_tbgr_enabled  ||  cnv->get_schedule()->is_temporary()  ||  unregistered_journey_time_exists(schedule, cnv->get_owner()) ) {
		// We accept all halts in reachable_halts
		FOR(const minivec_tpl<uint8>, const& i, goods_category_indexes) {
			FOR(const vector_tpl<reachable_halt_t>, const& rh, reachable_halts) {
				destination_halts.access(i)->append(rh.halt);
			}
		}
		return;
	}

	const uint32 additional_ticks_by_schedule = (uint64)schedule->get_additional_base_waiting_time() * world()->ticks_per_world_month / world()->get_settings().get_spacing_shift_divisor();
	const uint32 base_waiting_ticks = world()->get_settings().get_base_waiting_ticks(schedule->get_waytype()) + additional_ticks_by_schedule;
	FOR(const minivec_tpl<uint8>, const& g_index, goods_category_indexes) {
		FOR(const vector_tpl<reachable_halt_t>, const& rh, reachable_halts) {
			connection_t connection;
			FOR(const vector_tpl<connection_t>, const& j, all_links[g_index].connections) {
				if(  j.halt==rh.halt  ) {
					connection = j;
					break;
				}
			}
			if(  !connection.halt.is_bound()  ) {
				// We cannot find the connection. We have to skip this halt.
				continue;
			}

			const bool is_fastest_traveler_valid = std::visit(
				[](const auto &v) { return v.is_bound(); }, 
				connection.best_weight_traveler
			);
			// This convoy is already calculated as the fastest traveler to the halt.
			const bool is_fastest_traveler = connection.best_weight_traveler==traveler;
			const auto is_shortest_journey = [&]() {
				// The journey time is shorter than (wait time + journey time) of the fastest route.
				// The connection weight contains the base waiting time offset, so we subtract it.
				return rh.journey_time + base_waiting_ticks < connection.weight;
			};
			
			if(  !is_fastest_traveler_valid  ||  is_fastest_traveler  ||  is_shortest_journey()  ) {
				destination_halts.access(g_index)->append(rh.halt);
			}
		}
	}
}


// Returns if the route search is needed for the given ware.
// When TBGR is enabled, the route search is needed only when the next transfer halt is not valid.
bool haltestelle_t::is_route_search_needed(const ware_t &ware) const {
	if(  world()->get_settings().get_goods_routing_policy()!=goods_routing_policy_t::GRP_FIFO_ET  ) {
		// Not using time based goods routing -> Always reroute
		return true;
	}
	const halthandle_t next_transfer_halt = ware.get_zwischenziel();
	if(  !next_transfer_halt.is_bound()  ||  next_transfer_halt==self  ) {
		// The next transfer halt is not planned.
		return true;
	}
	// Check if the valid connection exists for each transfer step.
	halthandle_t transfer_halt = self;
	for(uint8 t_idx = 0; t_idx < ware.get_transit_halts().get_count(); t_idx++) {
		const halthandle_t next_transfer_halt = ware.get_transit_halts()[t_idx];
		bool connection_found = false;
		FOR(vector_tpl<connection_t>, const& i, transfer_halt->get_connections(ware.get_desc()->get_catg_index())) {
			if(  i.halt==next_transfer_halt  ) {
				// The planned next transfer halt is valid. Check the next step.
				connection_found = true;
				break;
			}
		}
		if(  !connection_found  ) {
			return true; // No available connection for this step.
		}
		transfer_halt = next_transfer_halt;
	}
	return false;
}


void haltestelle_t::extinguish_all_waiting_goods() {
	for(uint8 goods_ctg_idx=0; goods_ctg_idx<goods_manager_t::get_max_catg_index(); goods_ctg_idx++) {
		slist_tpl<cargo_item_t>* goods_list = cargo[goods_ctg_idx];
		if(  !goods_list  ) {
			continue;
		}
		while(  !goods_list->empty()  ) {
			goods_list->remove_first();
		}
	}
}


// TODO: Use (amount, arrived_time) type instead of halt_waiting_goods_t.
void haltestelle_t::fetch_loadable_fresh_goods(vector_tpl<loadable_fresh_goods_t>& to_array, const uint8 goods_category_index, const vector_tpl<halthandle_t>& destination_halts) {
	auto iterator = fresh_cargo[goods_category_index].begin();
	while(  iterator!=fresh_cargo[goods_category_index].end()  ) {
		cargo_item_t item = iterator->lock();
		if(  !item  ) {
			// The reference is not valid. Just remove it from the list.
			iterator = fresh_cargo[goods_category_index].erase(iterator);
			continue;
		}
		if(  !destination_halts.is_contained(item->goods.get_zwischenziel())  ) {
			// The goods cannot be loaded.
			iterator++;
			continue;
		}
		to_array.append(loadable_fresh_goods_t(item->goods.menge, item->arrived_time));
		iterator = fresh_cargo[goods_category_index].erase(iterator);
	}
}


void haltestelle_t::toggle_other_player_connection_allowed() {
	flags ^= HS_ALLOW_OTHER_PLAYER_CONNECTION;
	if(  (flags&HS_ALLOW_OTHER_PLAYER_CONNECTION)==0  ) {
		// We have to exclude the other player's connections.
		for(uint32 i=registered_lines.get_count(); i-->0;) {
			if(  registered_lines[i]->get_owner()!=get_owner()  ) {
				stale_lines.append_unique(registered_lines[i]);
				registered_lines.remove_at(i);
			}
		}
		for(uint32 i=registered_convoys.get_count(); i-->0;) {
			if(  registered_convoys[i]->get_owner()!=get_owner()  ) {
				stale_convois.append_unique(registered_convoys[i]);
				registered_convoys.remove_at(i);
			}
		}
	}
}
