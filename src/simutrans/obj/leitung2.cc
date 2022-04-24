/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t verbinde_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t calc_image_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pumpe_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t senke_list_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#include "leitung2.h"
#include "../simdebug.h"
#include "../world/simworld.h"
#include "simobj.h"
#include "../player/simplay.h"
#include "../display/simimg.h"
#include "../simfab.h"
#include "../simskin.h"

#include "../display/simgraph.h"

#include "../utils/cbuffer.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/powernet.h"
#include "../dataobj/environment.h"
#include "../dataobj/pakset_manager.h"

#include "../ground/grund.h"
#include "../builder/wegbauer.h"

const uint32 POWER_TO_MW = 12;

// use same precision as powernet
const uint8 leitung_t::FRACTION_PRECISION = powernet_t::FRACTION_PRECISION;

/**
 * returns possible directions for powerline on this tile
 */
ribi_t::ribi get_powerline_ribi(grund_t *gr)
{
	slope_t::type slope = gr->get_weg_hang();
	ribi_t::ribi ribi = (ribi_t::ribi)ribi_t::all;
	if (slope == slope_t::flat) {
		// respect possible directions for bridge and tunnel starts
		if (gr->ist_karten_boden()  &&  (gr->ist_tunnel()  ||  gr->ist_bruecke())) {
			ribi = ribi_t::doubles( ribi_type( gr->get_grund_hang() ) );
		}
	}
	else {
		ribi = ribi_t::doubles( ribi_type(slope) );
	}
	return ribi;
}

int leitung_t::gimme_neighbours(leitung_t **conn)
{
	int count = 0;
	grund_t *gr_base = welt->lookup(get_pos());
	ribi_t::ribi ribi = get_powerline_ribi(gr_base);
	for(int i=0; i<4; i++) {
		// get next connected tile (if there)
		grund_t *gr;
		conn[i] = NULL;
		if(  (ribi & ribi_t::nesw[i])  &&  gr_base->get_neighbour( gr, invalid_wt, ribi_t::nesw[i] ) ) {
			leitung_t *lt = gr->get_leitung();
			// check that we can connect to the other tile: correct slope,
			// both ground or both tunnel or both not tunnel
			bool const ok = (gr->ist_karten_boden()  &&  gr_base->ist_karten_boden())  ||  (gr->ist_tunnel()==gr_base->ist_tunnel());
			if(  lt  &&  (ribi_t::backward(ribi_t::nesw[i]) & get_powerline_ribi(gr))  &&  ok  ) {
				const player_t *owner = get_owner();
				const player_t *other = lt->get_owner();
				const player_t *super = welt->get_public_player();
				if (owner==other  ||  owner==super  ||  other==super) {
					conn[i] = lt;
					count++;
				}
			}
		}
	}
	return count;
}


fabrik_t *leitung_t::suche_fab_4(const koord pos)
{
	for(int k=0; k<4; k++) {
		fabrik_t *fab = fabrik_t::get_fab( pos+koord::nesw[k] );
		if(fab) {
			return fab;
		}
	}
	return NULL;
}


leitung_t::leitung_t(loadsave_t *file) : obj_t()
{
	image = IMG_EMPTY;
	set_net(NULL);
	ribi = ribi_t::none;
	is_transformer = false;
	rdwr(file);
}


leitung_t::leitung_t(koord3d pos, player_t *player) : obj_t(pos)
{
	image = IMG_EMPTY;
	set_net(NULL);
	set_owner( player );
	set_desc(way_builder_t::leitung_desc);
	is_transformer = false;
}


leitung_t::~leitung_t()
{
	if (welt->is_destroying()) {
		return;
	}

	grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		leitung_t *conn[4];
		int neighbours = gimme_neighbours(conn);
		gr->obj_remove(this);
		set_flag( obj_t::not_on_map );

		if(neighbours>1) {
			// only reconnect if two connections ...
			bool first = true;
			for(int i=0; i<4; i++) {
				if(conn[i]!=NULL) {
					if(!first) {
						// replace both nets
						powernet_t *new_net = new powernet_t();
						conn[i]->replace(new_net);
					}
					first = false;
				}
			}
		}

		// recalc images
		for(int i=0; i<4; i++) {
			if(conn[i]!=NULL) {
				conn[i]->calc_neighbourhood();
			}
		}

		if(neighbours==0) {
			delete net;
		}
		player_t::add_maintenance(get_owner(), -get_maintenance(), powerline_wt);
	}
}


void leitung_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, -desc->get_price()/2, get_pos().get_2d(), powerline_wt);
	mark_image_dirty( image, 0 );
}


/**
 * called during map rotation
 */
void leitung_t::rotate90()
{
	obj_t::rotate90();
	ribi = ribi_t::rotate90( ribi );
}


/**
 * replace networks connection
 * non-trivial to handle transformers correctly
 */
void leitung_t::replace(powernet_t* new_net)
{
	if (get_net() != new_net) {
		// convert myself ...
//DBG_MESSAGE("leitung_t::replace()","My net %p by %p at (%i,%i)",new_net,current,base_pos.x,base_pos.y);
		set_net(new_net);
	}

	leitung_t * conn[4];
	if(gimme_neighbours(conn)>0) {
		for(int i=0; i<4; i++) {
			if(conn[i] && conn[i]->get_net()!=new_net) {
				conn[i]->replace(new_net);
			}
		}
	}
}


/**
 * Connect this piece of powerline to its neighbours
 * -> this can merge power networks
 */
void leitung_t::verbinde()
{
	// first get my own ...
	powernet_t *new_net = get_net();
//DBG_MESSAGE("leitung_t::verbinde()","Searching net at (%i,%i)",get_pos().x,get_pos().x);
	leitung_t * conn[4];
	if(gimme_neighbours(conn)>0) {
		for( uint8 i=0;  i<4 && new_net==NULL;  i++  ) {
			if(conn[i]) {
				new_net = conn[i]->get_net();
			}
		}
	}

//DBG_MESSAGE("leitung_t::verbinde()","Found net %p",new_net);

	// we are alone?
	if(get_net()==NULL) {
		if(new_net!=NULL) {
			replace(new_net);
		}
		else {
			// then we start a new net
			set_net(new powernet_t());
//DBG_MESSAGE("leitung_t::verbinde()","Creating new net %p",new_net);
		}
	}
	else if(new_net) {
		powernet_t *my_net = get_net();
		for( uint8 i=0;  i<4;  i++  ) {
			if(conn[i] && conn[i]->get_net()!=new_net) {
				conn[i]->replace(new_net);
			}
		}
		if(my_net && my_net!=new_net) {
			delete my_net;
		}
	}
}


void leitung_t::calc_image()
{
	is_crossing = false;
	const koord pos = get_pos().get_2d();
	bool snow = get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate;

	grund_t *gr = welt->lookup(get_pos());
	if(gr==NULL) {
		// no valid ground; usually happens during building ...
		return;
	}
	if(gr->ist_bruecke() || (gr->get_typ()==grund_t::tunnelboden && gr->ist_karten_boden())) {
		// don't display on a bridge or in a tunnel)
		set_image(IMG_EMPTY);
		return;
	}

	image_id old_image = get_image();
	slope_t::type hang = gr->get_weg_hang();
	if(hang != slope_t::flat) {
		set_image( desc->get_slope_image_id(hang, snow));
	}
	else {
		if(gr->hat_wege()) {
			// crossing with road or rail
			weg_t* way = gr->get_weg_nr(0);
			if(ribi_t::is_straight_ew(way->get_ribi())) {
				set_image( desc->get_diagonal_image_id(ribi_t::north|ribi_t::east, snow));
			}
			else {
				set_image( desc->get_diagonal_image_id(ribi_t::south|ribi_t::east, snow));
			}
			is_crossing = true;
		}
		else {
			if(ribi_t::is_straight(ribi)  &&  !ribi_t::is_single(ribi)  &&  (pos.x+pos.y)&1) {
				// every second skip mast
				if(ribi_t::is_straight_ns(ribi)) {
					set_image( desc->get_diagonal_image_id(ribi_t::north|ribi_t::west, snow));
				}
				else {
					set_image( desc->get_diagonal_image_id(ribi_t::south|ribi_t::west, snow));
				}
			}
			else {
				set_image( desc->get_image_id(ribi, snow));
			}
		}
	}
	if (old_image != get_image()) {
		mark_image_dirty(old_image,0);
	}
}


/**
 * Recalculates the images of all neighbouring
 * powerlines and the powerline itself
 */
void leitung_t::calc_neighbourhood()
{
	leitung_t *conn[4];
	ribi = ribi_t::none;
	if(gimme_neighbours(conn)>0) {
		for( uint8 i=0;  i<4 ;  i++  ) {
			if(conn[i]  &&  conn[i]->get_net()==get_net()) {
				ribi |= ribi_t::nesw[i];
				conn[i]->add_ribi(ribi_t::backward(ribi_t::nesw[i]));
				conn[i]->calc_image();
			}
		}
	}
	set_flag( obj_t::dirty );
	calc_image();
}


void leitung_t::info(cbuffer_t & buf) const
{
	obj_t::info(buf);

	powernet_t * const net = get_net();

	buf.printf(translator::translate("Net ID: %p"), net);
	buf.printf("\n");
	//buf.printf(translator::translate("Capacity: %.0f MW"), (double)(net->get_max_capacity() >> POWER_TO_MW));
	//buf.printf("\n");
	buf.printf(translator::translate("Demand: %.0f MW"), (double)(net->get_demand() >> POWER_TO_MW));
	buf.printf("\n");
	buf.printf(translator::translate("Generation: %.0f MW"), (double)(net->get_supply() >> POWER_TO_MW));
	buf.printf("\n");
	buf.printf(translator::translate("Usage: %.0f %%"), (double)((100 * net->get_normal_demand()) >> powernet_t::FRACTION_PRECISION));
}


void leitung_t::finish_rd()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &verbinde_mutex );
	verbinde();
	pthread_mutex_unlock( &verbinde_mutex );
	pthread_mutex_lock( &calc_image_mutex );
	calc_neighbourhood();
	pthread_mutex_unlock( &calc_image_mutex );
#else
	verbinde();
	calc_neighbourhood();
#endif

	grund_t *gr = welt->lookup(get_pos());
	assert(gr); (void)gr;

	player_t::add_maintenance(get_owner(), get_maintenance(), powerline_wt);
}

void leitung_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "leitung_t" );

	obj_t::rdwr(file);

	// no longer save power net pointer as it is no longer used
	if(  file->is_version_less(120, 4)  ) {
		uint32 value = 0;
		file->rdwr_long(value);
	}
	if(  file->is_loading()  ) {
		set_net(NULL);
	}

	if(get_typ()==leitung) {
		/* ATTENTION: during loading thus MUST not be called from the constructor!!!
		* (Otherwise it will be always true!
		*/
		if(file->is_version_atleast(102, 3)) {
			if(file->is_saving()) {
				const char *s = desc->get_name();
				file->rdwr_str(s);
			}
			else {
				char bname[128];
				file->rdwr_str(bname, lengthof(bname));

				const way_desc_t *desc = way_builder_t::get_desc(bname);
				if(desc==NULL) {
					desc = way_builder_t::get_desc(translator::compatibility_name(bname));
					if(desc==NULL) {
						pakset_manager_t::add_missing_paks( bname, MISSING_WAY );
						desc = way_builder_t::leitung_desc;

						if (!desc) {
							dbg->fatal("leitung_t::rdwr", "Trying to load powerline but pakset has none!");
						}
					}
					dbg->warning("leitung_t::rdwr()", "Unknown powerline %s replaced by %s", bname, desc->get_name() );
				}
				set_desc(desc);
			}
		}
		else {
			if (file->is_loading()) {
				set_desc(way_builder_t::leitung_desc);
			}
		}
	}
}

// returns NULL, if removal is allowed
// players can remove public owned powerlines
const char *leitung_t::is_deletable(const player_t *player)
{
	if(  get_owner_nr()==PUBLIC_PLAYER_NR  &&  player  ) {
		return NULL;
	}
	return obj_t::is_deletable(player);
}


sint64 leitung_t::get_maintenance() const
{
	if (!is_transformer) {
		return desc->get_maintenance();
	}
	else {
		return -welt->get_settings().cst_maintain_transformer;
	}
}


/************************************ from here on pump (source) stuff ********************************************/

slist_tpl<pumpe_t *> pumpe_t::pumpe_list;


void pumpe_t::new_world()
{
	pumpe_list.clear();
}


void pumpe_t::step_all(uint32 delta_t)
{
	for(pumpe_t* const p : pumpe_list) {
		p->step(delta_t);
	}
}


pumpe_t::pumpe_t(loadsave_t *file ) : leitung_t( koord3d::invalid, NULL )
{
	fab = NULL;
	power_supply = 0;
	is_transformer = true;
	rdwr( file );
}


pumpe_t::pumpe_t(koord3d pos, player_t *player) : leitung_t(pos, player)
{
	fab = NULL;
	power_supply = 0;
	is_transformer = true;
	player_t::book_construction_costs(player, welt->get_settings().cst_transformer, get_pos().get_2d(), powerline_wt);
}


pumpe_t::~pumpe_t()
{
	if(fab) {
		fab->remove_transformer_connected(this);
		fab = NULL;
	}
	if(  net != NULL  ) {
		net->sub_supply(power_supply);
	}
	pumpe_list.remove( this );
}

void pumpe_t::step(uint32 delta_t)
{
	if(  fab == NULL  ) {
		return;
	}
	else if(  delta_t == 0  ) {
		return;
	}

	// usage logic could go here

	// resolve image
	uint16 winter_offset = 0;
	if(  skinverwaltung_t::senke->get_count() > 3  &&  (get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate)  ) {
		winter_offset = 2;
	}
	uint16 const image_offset = power_supply > 0 ? 1 : 0;
	image_id const new_image = skinverwaltung_t::pumpe->get_image_id(image_offset + winter_offset);

	// update image
	if(  image != new_image  ) {
		set_flag(obj_t::dirty);
		set_image(new_image);
	}
}

void pumpe_t::set_net(powernet_t * p)
{
	powernet_t * p_old = get_net();
	if(  p_old != NULL  ) {
		p_old->sub_supply(power_supply);
	}

	leitung_t::set_net(p);

	if(  p != NULL  ) {
		p->add_supply(power_supply);
	}
}

void pumpe_t::set_power_supply(uint32 newsupply)
{
	// update power network
	powernet_t *const p = get_net();
	if(  p != NULL  ) {
		p->sub_supply(power_supply);
		p->add_supply(newsupply);
	}

	power_supply = newsupply;
}

sint32 pumpe_t::get_power_consumption() const
{
	powernet_t const *const p = get_net();
	return p->get_normal_demand();
}

void pumpe_t::rdwr(loadsave_t * file)
{
	xml_tag_t d( file, "pumpe_t" );

	leitung_t::rdwr(file);

	// current power state
	if(  file->is_version_atleast(120, 4)  ) {
		file->rdwr_long(power_supply);
	}
}


void pumpe_t::finish_rd()
{
	leitung_t::finish_rd();

	assert(get_net());

	if(  fab==NULL  ) {
		if(welt->lookup(get_pos())->ist_karten_boden()) {
			// on surface, check around
			fab = leitung_t::suche_fab_4(get_pos().get_2d());
		}
		else {
			// underground, check directly above
			fab = fabrik_t::get_fab(get_pos().get_2d());
		}
		if(  fab  ) {
			// only add when factory there
			fab->add_transformer_connected(this);
		}
	}

#ifdef MULTI_THREAD
	pthread_mutex_lock( &pumpe_list_mutex );
	pumpe_list.insert( this );
	pthread_mutex_unlock( &pumpe_list_mutex );

	pthread_mutex_lock( &calc_image_mutex );
	set_image(skinverwaltung_t::pumpe->get_image_id(0));
	is_crossing = false;
	pthread_mutex_unlock( &calc_image_mutex );
#else
	pumpe_list.insert( this );
	set_image(skinverwaltung_t::pumpe->get_image_id(0));
	is_crossing = false;
#endif
}


void pumpe_t::info(cbuffer_t & buf) const
{
	obj_t::info( buf );

	buf.printf(translator::translate("Net ID: %p"), get_net());
	buf.printf("\n");
	buf.printf(translator::translate("Generation: %.0f MW"), (double)(power_supply >> POWER_TO_MW));
	buf.printf("\n");
	buf.printf(translator::translate("Usage: %.0f %%"), (double)((100 * get_net()->get_normal_demand()) >> powernet_t::FRACTION_PRECISION));
	buf.printf("\n"); // pad for consistent dialog size
}


/************************************ Distriubtion Transformer Code ********************************************/

slist_tpl<senke_t *> senke_t::senke_list;
uint32 senke_t::payment_timer = 0;

void senke_t::new_world()
{
	senke_list.clear();
	payment_timer = 0;
}

void senke_t::static_rdwr(loadsave_t *file)
{
	if(  file->is_version_atleast(120, 4)  ) {
		file->rdwr_long(payment_timer);
	}
}

void senke_t::step_all(uint32 delta_t)
{
	// payment period (could be tied to game setting)
	const uint32 pay_period = PRODUCTION_DELTA_T * 10; // 10 seconds

	// revenue payout timer
	payment_timer += delta_t;
	const bool payout = payment_timer >= pay_period;
	payment_timer %= pay_period;

	// step all distribution transformers
	for(senke_t* const s : senke_list) {
		s->step(delta_t);
		if (payout) {
			s->pay_revenue();
		}
	}
}


senke_t::senke_t(loadsave_t *file) : leitung_t( koord3d::invalid, NULL )
{
	fab = NULL;
	delta_t_sum = 0;
	next_t = 0;
	power_demand = 0;
	energy_acc = 0;
	is_transformer = true;

	rdwr( file );

	welt->sync.add(this);
}


senke_t::senke_t(koord3d pos, player_t *player) : leitung_t(pos, player)
{
	fab = NULL;
	delta_t_sum = 0;
	next_t = 0;
	power_demand = 0;
	energy_acc = 0;
	is_transformer = true;

	player_t::book_construction_costs(player, welt->get_settings().cst_transformer, get_pos().get_2d(), powerline_wt);

	welt->sync.add(this);
}


senke_t::~senke_t()
{
	// one last final income
	pay_revenue();

	welt->sync.remove( this );
	if(fab!=NULL) {
		fab->remove_transformer_connected(this);
		fab = NULL;
	}
	if(  net != NULL  ) {
		net->sub_demand(power_demand);
	}
	senke_list.remove( this );
}

void senke_t::step(uint32 delta_t)
{
	if(  fab == NULL  ) {
		return;
	}
	else if(  delta_t == 0  ) {
		return;
	}

	// energy metering logic
	energy_acc += ((uint64)power_demand * (uint64)get_net()->get_normal_supply() * (uint64)delta_t) / ((uint64)PRODUCTION_DELTA_T << powernet_t::FRACTION_PRECISION);
}

void senke_t::pay_revenue()
{
	// megajoules (megawatt seconds) per cent
	const uint64 mjpc = (1 << POWER_TO_MW) / CREDIT_PER_MWS; // should be tied to game setting

	// calculate payment in cent
	const sint64 payment = (sint64)(energy_acc / mjpc);

	// make payment
	if(  payment  >  0  ) {
		// enough has accumulated for a payment
		get_owner()->book_revenue( payment, get_pos().get_2d(), powerline_wt );

		// remove payment from accumulator
		energy_acc %= mjpc;
	}
}

void senke_t::set_net(powernet_t * p)
{
	powernet_t * p_old = get_net();
	if(  p_old != NULL  ) {
		p_old->sub_demand(power_demand);
	}

	leitung_t::set_net(p);

	if(  p != NULL  ) {
		p->add_demand(power_demand);
	}
}

void senke_t::set_power_demand(uint32 newdemand)
{
	// update power network
	powernet_t *const p = get_net();
	if(  p != NULL  ) {
		p->sub_demand(power_demand);
		p->add_demand(newdemand);
	}

	power_demand = newdemand;
}

sint32 senke_t::get_power_satisfaction() const
{
	powernet_t const *const p = get_net();
	return p->get_normal_supply();
}

sync_result senke_t::sync_step(uint32 delta_t)
{
	if(fab==NULL) {
		return SYNC_DELETE;
	}

	// advance timers
	delta_t_sum += delta_t;
	next_t += delta_t;

	// change graphics at most 16 times a second
	if(  next_t > PRODUCTION_DELTA_T / 16  ) {
		// enforce timer periods
		delta_t_sum %= PRODUCTION_DELTA_T; // 1 second
		next_t %= PRODUCTION_DELTA_T / 16; // 1/16 seconds

		// determine pwm period for image change
		uint32 pwm_period = 0;
		const sint32 satisfaction = get_net()->get_normal_supply();
		if(  satisfaction  >=  1 << powernet_t::FRACTION_PRECISION  ) {
			// always on
			pwm_period = PRODUCTION_DELTA_T;
		}
		else if(  satisfaction  >=  ((7 << powernet_t::FRACTION_PRECISION) / 8)  ) {
			// limit to at most 7/8 of a second
			pwm_period = 7 * PRODUCTION_DELTA_T / 8;
		}
		else if(  satisfaction  >  ((1 << powernet_t::FRACTION_PRECISION) / 8)  ) {
			// duty cycle based on power satisfaction
			pwm_period = (uint32)(((uint64)PRODUCTION_DELTA_T * (uint64)satisfaction) >> powernet_t::FRACTION_PRECISION);
		}
		else if(  satisfaction  >  0  ) {
			// limit to at least 1/8 of a second
			pwm_period = PRODUCTION_DELTA_T / 8;
		}

		// determine image with PWM logic
		const uint16 work_offset = (delta_t_sum < pwm_period) ? 1 : 0;

		// apply seasonal image offset
		uint16 winter_offset = 0;
		if(  skinverwaltung_t::senke->get_count() > 3  &&  (get_pos().z >= welt->get_snowline()  ||
			 welt->get_climate(get_pos().get_2d()) == arctic_climate)  ) {
			winter_offset = 2;
		}

		// update displayed image
		image_id new_image = skinverwaltung_t::senke->get_image_id(work_offset + winter_offset);
		if(  image != new_image  ) {
			set_flag( obj_t::dirty );
			set_image( new_image );
		}
	}
	return SYNC_OK;
}


void senke_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "senke_t" );

	leitung_t::rdwr(file);

	// current power state
	if(  file->is_version_atleast(120, 4)  ) {
		file->rdwr_longlong((sint64 &)energy_acc);
		file->rdwr_long(power_demand);
	}
	else if (file->is_loading()) {
		energy_acc = 0;
		power_demand = 0;
	}

	if (file->is_version_atleast(122, 1)) {
		file->rdwr_long(delta_t_sum);
		file->rdwr_long(next_t);
	}
	else if (file->is_loading()) {
		delta_t_sum = 0;
		next_t = 0;
	}
}

void senke_t::finish_rd()
{
	leitung_t::finish_rd();

	assert(get_net());

	if(  fab==NULL  ) {
		if(welt->lookup(get_pos())->ist_karten_boden()) {
			// on surface, check around
			fab = leitung_t::suche_fab_4(get_pos().get_2d());
		}
		else {
			// underground, check directly above
			fab = fabrik_t::get_fab(get_pos().get_2d());
		}
		if(  fab  ) {
			fab->add_transformer_connected(this);
		}
	}

#ifdef MULTI_THREAD
	pthread_mutex_lock( &senke_list_mutex );
#endif
	senke_list.insert( this );
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &senke_list_mutex );
	pthread_mutex_lock( &calc_image_mutex );
#endif
	set_image(skinverwaltung_t::senke->get_image_id(0));
	is_crossing = false;
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &calc_image_mutex );
#endif
}

void senke_t::info(cbuffer_t & buf) const
{
	obj_t::info( buf );

	buf.printf(translator::translate("Net ID: %p"), get_net());
	buf.printf("\n");
	buf.printf(translator::translate("Demand: %.0f MW"), (double)(power_demand >> POWER_TO_MW));
	buf.printf("\n");
	buf.printf(translator::translate("Supplied: %.0f %%"), (double)((100 * get_net()->get_normal_supply()) >> powernet_t::FRACTION_PRECISION));
	buf.printf("\n"); // pad for consistent dialog size
}
