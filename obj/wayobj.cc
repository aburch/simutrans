/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>

#include "../boden/grund.h"
#include "../simworld.h"
#include "../display/simimg.h"
#include "simobj.h"
#include "../player/simplay.h"
#include "../simtool.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/ribi.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../descriptor/bridge_desc.h"
#include "../descriptor/tunnel_desc.h"
#include "../descriptor/way_obj_desc.h"

#include "../gui/tool_selector.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/simstring.h"

#include "bruecke.h"
#include "tunnel.h"
#include "wayobj.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t wayobj_calc_image_mutex;
static recursive_mutex_maker_t wayobj_cim_maker(wayobj_calc_image_mutex);
#endif

// the descriptions ...
const way_obj_desc_t *wayobj_t::default_oberleitung=NULL;
stringhashtable_tpl<const way_obj_desc_t *> wayobj_t::table;


wayobj_t::wayobj_t(loadsave_t* const file) : obj_no_info_t()
{
	rdwr(file);
}


wayobj_t::wayobj_t(koord3d const pos, player_t* const owner, ribi_t::ribi const d, way_obj_desc_t const* const b) : obj_no_info_t(pos)
{
	desc = b;
	dir = d;
	set_owner(owner);
}


wayobj_t::~wayobj_t()
{
	if(!desc) {
		return;
	}
	if(get_owner()) {
		player_t::add_maintenance(get_owner(), -desc->get_maintenance(), get_waytype());
	}
	if(desc->is_overhead_line()) {
		grund_t *gr=welt->lookup(get_pos());
		weg_t *weg=NULL;
		if(gr) {
			const waytype_t wt = (desc->get_wtyp()==tram_wt) ? track_wt : desc->get_wtyp();
			weg = gr->get_weg(wt);
			if(weg) {
				// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
				weg->set_electrify(false);
				// restore old speed limit
				sint32 max_speed = weg->hat_gehweg() ? 50 : weg->get_desc()->get_topspeed();
				if(gr->get_typ()==grund_t::tunnelboden) {
					tunnel_t *t = gr->find<tunnel_t>(1);
					if(t) {
						max_speed = t->get_desc()->get_topspeed();
					}
				}
				if(gr->get_typ()==grund_t::brueckenboden) {
					bruecke_t *b = gr->find<bruecke_t>(1);
					if(b) {
						max_speed = b->get_desc()->get_topspeed();
					}
				}
				// Updating way's max speed is needed unless the way is a railway track.
				if(  wt!=track_wt  ) {
					weg->set_max_speed(max_speed);
				}
				weg->set_max_wayobj_speed(0);
			}
			else {
				dbg->warning("wayobj_t::~wayobj_t()","ground was not a way!");
			}
		}
	}

	mark_image_dirty( get_front_image_test(), 0 );
	mark_image_dirty( get_image_test(), 0 );
	grund_t *gr = welt->lookup( get_pos() );
	if( gr ) {
		for( uint8 i = 0; i < 4; i++ ) {
			// Remove ribis from adjacent wayobj.
			if( ribi_t::nesw[i] & get_dir() ) {
				grund_t *next_gr;
				if( gr->get_neighbour( next_gr, desc->get_wtyp(), ribi_t::nesw[i] ) ) {
					wayobj_t *wo2 = next_gr->get_wayobj( desc->get_wtyp() );
					if( wo2 ) {
						wo2->mark_image_dirty( wo2->get_front_image_test(), 0 );
						wo2->mark_image_dirty( wo2->get_image_test(), 0 );
						wo2->set_dir( wo2->get_dir() & ~ribi_t::backward(ribi_t::nesw[i]) );
						wo2->mark_image_dirty( wo2->get_front_image_test(), 0 );
						wo2->mark_image_dirty( wo2->get_image_test(), 0 );
						wo2->set_flag(obj_t::dirty);
					}
				}
			}
		}
	}
}


void wayobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "wayobj_t" );
	obj_t::rdwr(file);
	if(file->is_version_atleast(89, 0)) {
		uint8 ddir = dir;
		file->rdwr_byte(ddir);
		dir = ddir;
		if(file->is_saving()) {
			const char *s = desc->get_name();
			file->rdwr_str(s);
		}
		else {
			char bname[128];
			file->rdwr_str(bname, lengthof(bname));

			desc = wayobj_t::table.get(bname);
			if(desc==NULL) {
				desc = wayobj_t::table.get(translator::compatibility_name(bname));
				if(desc==NULL) {
					if(strstr(bname,"atenary")  ||  strstr(bname,"electri")  ||  strstr(bname,"power")  ) {
						desc = default_oberleitung;
					}
				}
				if(desc==NULL) {
					dbg->warning("wayobj_t::rwdr", "description %s for wayobj_t at %d,%d not found, will be removed!", bname, get_pos().x, get_pos().y );
					welt->add_missing_paks( bname, karte_t::MISSING_WAYOBJ );
				}
				else {
					dbg->warning("wayobj_t::rwdr", "wayobj %s at %d,%d replaced by %s", bname, get_pos().x, get_pos().y, desc->get_name() );
				}
			}
		}
	}
	else {
		desc = default_oberleitung;
		dir = dir_unknown;
	}
}


void wayobj_t::cleanup(player_t *player)
{
	if(desc) {
		player_t::book_construction_costs(player, -desc->get_price(), get_pos().get_2d(), desc->get_wtyp());
	}
}


// returns NULL, if removal is allowed
// players can remove public owned wayobjs
const char *wayobj_t::is_deletable(const player_t *player)
{
	if(  get_owner_nr()==PUBLIC_PLAYER_NR  ) {
		return NULL;
	}
	return obj_t::is_deletable(player);
}


void wayobj_t::finish_rd()
{
	// (re)set dir
	if(dir==dir_unknown) {
		const waytype_t wt = (desc->get_wtyp()==tram_wt) ? track_wt : desc->get_wtyp();
		weg_t *w=welt->lookup(get_pos())->get_weg(wt);
		if(w) {
			dir = w->get_ribi_unmasked();
		}
		else {
			dir = ribi_t::all;
		}
	}

	// electrify a way if we are a catenary
	if(desc->is_overhead_line()) {
		const waytype_t wt = (desc->get_wtyp()==tram_wt) ? track_wt : desc->get_wtyp();
		weg_t *weg = welt->lookup(get_pos())->get_weg(wt);
		if (wt == any_wt) {
			weg_t *weg2 = welt->lookup(get_pos())->get_weg_nr(1);
			if (weg2) {
				weg2->set_electrify(true);
				if(weg2->get_max_speed()>desc->get_topspeed()) {
					weg2->set_max_speed(desc->get_topspeed());
				}
			}
		}
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->set_electrify(true);
			if(wt == track_wt){
				weg->set_max_wayobj_speed(desc->get_topspeed());
			}
			else if(weg->get_max_speed()>desc->get_topspeed()) {
				weg->set_max_speed(desc->get_topspeed());
			}
		}
		else {
			dbg->warning("wayobj_t::finish_rd()","ground was not a way!");
		}
	}

	if(get_owner()) {
		player_t::add_maintenance(get_owner(), desc->get_maintenance(), desc->get_wtyp());
	}
}


void wayobj_t::rotate90()
{
	obj_t::rotate90();
	dir = ribi_t::rotate90( dir);
	hang = slope_t::rotate90( hang );
}


// helper function: gets the ribi on next tile
ribi_t::ribi wayobj_t::find_next_ribi(const grund_t *start, ribi_t::ribi const dir, const waytype_t wt) const
{
	grund_t *to;
	ribi_t::ribi r1 = ribi_t::none;
	if(start->get_neighbour(to,wt,dir)) {
		const wayobj_t* wo = to->get_wayobj( wt );
		if(wo) {
			r1 = wo->get_dir();
		}
	}
	return r1;
}


void wayobj_t::calc_image()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &wayobj_calc_image_mutex );
#endif
	grund_t *gr = welt->lookup(get_pos());
	diagonal = false;
	if(gr) {
		const waytype_t wt = (desc->get_wtyp()==tram_wt) ? track_wt : desc->get_wtyp();
		weg_t *w=gr->get_weg(wt);
		if(!w) {
			dbg->error("wayobj_t::calc_image()","without way at (%s)", get_pos().get_str() );
			// well, we are not on a way anymore? => delete us
			cleanup(get_owner());
			delete this;
			gr->set_flag(grund_t::dirty);
#ifdef MULTI_THREAD
			pthread_mutex_unlock( &wayobj_calc_image_mutex );
#endif
			return;
		}

		ribi_t::ribi sec_way_ribi_unmasked = 0;
		if(wt == any_wt) {
			if (weg_t *sec_w = gr->get_weg_nr(1)) {
				sec_way_ribi_unmasked = sec_w->get_ribi_unmasked();
			}
		}

		set_yoff( -gr->get_weg_yoff() );
		set_xoff( 0 );
		dir &= (w->get_ribi_unmasked() | sec_way_ribi_unmasked);

		// if there is a slope, we are finished, only four choices here (so far)
		hang = gr->get_weg_hang();
		if(hang!=slope_t::flat) {
#ifdef MULTI_THREAD
			pthread_mutex_unlock( &wayobj_calc_image_mutex );
#endif
			return;
		}

		// find out whether using diagonals or straight lines
		if(ribi_t::is_bend(dir)  &&  desc->has_diagonal_image()) {
			ribi_t::ribi r1 = ribi_t::none, r2 = ribi_t::none;

			// get the ribis of the ways that connect to us
			// r1 will be 45 degree clockwise ribi (eg northeast->east), r2 will be anticlockwise ribi (eg northeast->north)
			r1 = find_next_ribi( gr, ribi_t::rotate45(dir), wt );
			r2 = find_next_ribi( gr, ribi_t::rotate45l(dir), wt );

			// diagonal if r1 or r2 are our reverse and neither one is 90 degree rotation of us
			diagonal = (r1 == ribi_t::backward(dir) || r2 == ribi_t::backward(dir)) && r1 != ribi_t::rotate90l(dir) && r2 != ribi_t::rotate90(dir);

			if(diagonal) {
				// with this, we avoid calling us endlessly
				// HACK (originally by hajo?)
				static int rekursion = 0;

				if(rekursion == 0) {
					grund_t *to;
					rekursion++;
					for(int r = 0; r < 4; r++) {
						if(gr->get_neighbour(to, wt, ribi_t::nesw[r])) {
							wayobj_t* wo = to->get_wayobj( wt );
							if(wo) {
								wo->calc_image();
							}
						}
					}
					rekursion--;
				}

				image_id after = desc->get_front_diagonal_image_id(dir);
				image_id image = desc->get_back_diagonal_image_id(dir);
				if(image==IMG_EMPTY  &&  after==IMG_EMPTY) {
					// no diagonals available
					diagonal = false;
				}
			}
		}
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &wayobj_calc_image_mutex );
#endif
}



/******************** static stuff from here **********************/


/* better use this constrcutor for new wayobj; it will extend a matching obj or make an new one
 */
void wayobj_t::extend_wayobj(koord3d pos, player_t *owner, ribi_t::ribi dir, const way_obj_desc_t *desc, bool keep_existing_faster_way)
{
	grund_t *gr=welt->lookup(pos);
	if(gr) {
		wayobj_t *existing_wayobj = gr->get_wayobj( desc->get_wtyp() );
		if( existing_wayobj ) {
			if(  ( existing_wayobj->get_desc()->get_topspeed() < desc->get_topspeed()  ||  !keep_existing_faster_way)  &&  player_t::check_owner(owner, existing_wayobj->get_owner())
				&&  existing_wayobj->get_desc() != desc  )
			{
				// replace slower by faster if desired
				dir = dir | existing_wayobj->get_dir();
				gr->set_flag(grund_t::dirty);
				delete existing_wayobj;
			}
			else {
				// extend this one instead
				existing_wayobj->set_dir(dir|existing_wayobj->get_dir());
				existing_wayobj->mark_image_dirty( existing_wayobj->get_front_image_test(), 0 );
				existing_wayobj->mark_image_dirty( existing_wayobj->get_image_test(), 0 );
				existing_wayobj->set_flag(obj_t::dirty);
				return;
			}
		}

		// nothing found => make a new one
		wayobj_t *wo = new wayobj_t(pos,owner,dir,desc);
		gr->obj_add(wo);
		wo->finish_rd();
		wo->calc_image();
		wo->mark_image_dirty( wo->get_front_image_test(), 0 );
		wo->set_flag(obj_t::dirty);
		player_t::book_construction_costs( owner,  -desc->get_price(), pos.get_2d(), desc->get_wtyp());

		for( uint8 i = 0; i < 4; i++ ) {
		// Extend wayobjects around the new one, that aren't already connected.
			if( ribi_t::nesw[i] & ~wo->get_dir() ) {
				grund_t *next_gr;
				if( gr->get_neighbour( next_gr, desc->get_wtyp(), ribi_t::nesw[i] ) ) {
					wayobj_t *wo2 = next_gr->get_wayobj( desc->get_wtyp() );
					if( wo2 ) {
						wo2->set_dir( wo2->get_dir() | ribi_t::backward(ribi_t::nesw[i]) );
						wo2->mark_image_dirty( wo2->get_front_image_test(), 0 );
						wo->set_dir( wo->get_dir() | ribi_t::nesw[i] );
						wo->mark_image_dirty( wo->get_front_image_test(), 0 );
					}
				}
			}
		}
	}
}



// to sort wayobj for always the same menu order
static bool compare_wayobj_desc(const way_obj_desc_t* a, const way_obj_desc_t* b)
{
	int diff = a->get_wtyp() - b->get_wtyp();
	if (diff == 0) {
		diff = a->get_topspeed() - b->get_topspeed();
	}
	if (diff == 0) {
		/* Some speed: sort by name */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


bool wayobj_t::successfully_loaded()
{
	if(table.empty()) {
		dbg->warning("wayobj_t::successfully_loaded()", "No obj found - may crash when loading catenary.");
	}

	way_obj_desc_t const* def = 0;
	FOR(stringhashtable_tpl<way_obj_desc_t const*>, const& i, table) {
		way_obj_desc_t const& b = *i.value;
		if (!b.is_overhead_line())                          continue;
		if (b.get_wtyp()     != track_wt)                   continue;
		if (def && def->get_topspeed() >= b.get_topspeed()) continue;
		def = &b;
	}
	default_oberleitung = def;

	return true;
}


bool wayobj_t::register_desc(way_obj_desc_t *desc)
{
	// avoid duplicates with same name
	if(  const way_obj_desc_t *old_desc = table.remove(desc->get_name())  ) {
		dbg->doubled( "wayobj", desc->get_name() );
		tool_t::general_tool.remove( old_desc->get_builder() );
		delete old_desc->get_builder();
		delete old_desc;
	}

	if(  desc->get_cursor()->get_image_id(1)!=IMG_EMPTY  ) {
		// only add images for wayobjexts with cursor ...
		tool_build_wayobj_t *tool = new tool_build_wayobj_t();
		tool->set_icon( desc->get_cursor()->get_image_id(1) );
		tool->cursor = desc->get_cursor()->get_image_id(0);
		tool->set_default_param(desc->get_name());
		tool_t::general_tool.append( tool );
		desc->set_builder( tool );
	}
	else {
		desc->set_builder( NULL );
	}

	table.put(desc->get_name(), desc);
DBG_DEBUG( "wayobj_t::register_desc()","%s", desc->get_name() );
	return true;
}


/**
 * Fill menu with icons of given wayobjects from the list
 */
void wayobj_t::fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_WAYOBJ | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time=welt->get_timeline_year_month();

	vector_tpl<const way_obj_desc_t *>matching;

	FOR(stringhashtable_tpl<way_obj_desc_t const*>, const& i, table) {
		way_obj_desc_t const* const desc = i.value;
		if(  desc->is_available(time)  ) {

			DBG_DEBUG("wayobj_t::fill_menu()", "try to add %s(%p)", desc->get_name(), desc);
			if(  desc->get_builder()  &&  wtyp==desc->get_wtyp()  ) {
				// only add items with a cursor
				matching.append(desc);
			}
		}
	}
	// sort the tools before adding to menu
	std::sort(matching.begin(), matching.end(), compare_wayobj_desc);
	FOR(vector_tpl<way_obj_desc_t const*>, const i, matching) {
		tool_selector->add_tool_selector(i->get_builder());
	}
}


const way_obj_desc_t *wayobj_t::get_overhead_line(waytype_t wt, uint16 time)
{
	FOR(stringhashtable_tpl<way_obj_desc_t const*>, const& i, table) {
		way_obj_desc_t const* const desc = i.value;
		if(  desc->is_available(time)  &&  desc->get_wtyp()==wt  &&  desc->is_overhead_line()  ) {
			return desc;
		}
	}
	return NULL;
}


const way_obj_desc_t* wayobj_t::find_desc(const char *str)
{
	return wayobj_t::table.get(str);
}

image_id wayobj_t::is_need_crossing_image(ribi_t::ribi ribi, bool front) const
{
	if(ribi == ribi_t::northsoutheast){
		grund_t *from = welt->lookup(get_pos());
		grund_t *east;
		if(from->get_neighbour(east, track_wt, ribi_t::east) && east->get_wayobj(track_wt)){
			ribi_t::ribi east_ribi = east->get_wayobj(track_wt)->get_dir();
			if(east_ribi == ribi_t::northwest){
				return desc->get_crossing_image_id(ribi, true, front);
			}
			else if(east_ribi == ribi_t::southwest){
				return desc->get_crossing_image_id(ribi, false, front);
			}
			else if(east_ribi == ribi_t::northsouthwest){
				grund_t *north;
				grund_t *south;
				grund_t *north_east;
				grund_t *south_east;
				if(  from->get_neighbour(north, track_wt, ribi_t::north) && from->get_neighbour(south, track_wt, ribi_t::south)
						&& east->get_neighbour(north_east, track_wt, ribi_t::north) && east->get_neighbour(south_east, track_wt, ribi_t::south))
				{
					ribi_t::ribi north_masked_ribi = north->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi south_masked_ribi = south->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi north_east_masked_ribi = north_east->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi south_east_masked_ribi = south_east->get_weg(track_wt)->get_ribi_masked();
					if((north_masked_ribi & ribi_t::northwest) && (south_east_masked_ribi & ribi_t::southeast) && (south_masked_ribi != ribi_t::north || north_east_masked_ribi != ribi_t::south)){
						return desc->get_crossing_image_id(ribi, true, front);
					}
					else if((north_masked_ribi & ribi_t::southwest) && (south_east_masked_ribi & ribi_t::northeast) && (south_masked_ribi != ribi_t::south || north_east_masked_ribi != ribi_t::north)){
						return desc->get_crossing_image_id(ribi, true, front);
					}
					else if((south_masked_ribi & ribi_t::southwest) && (north_east_masked_ribi & ribi_t::northeast) && (north_masked_ribi != ribi_t::south || south_east_masked_ribi != ribi_t::north)){
						return desc->get_crossing_image_id(ribi, false, front);
					}
					else if((south_masked_ribi & ribi_t::northwest) && (north_east_masked_ribi & ribi_t::southeast) && (north_masked_ribi != ribi_t::north || south_east_masked_ribi != ribi_t::south)){
						return desc->get_crossing_image_id(ribi, false, front);
					}
				}
			}
			else if( east_ribi == ribi_t::all ){
				grund_t *south;
				grund_t *north_east;
				grund_t *east_east;
				if(  from->get_neighbour(south, track_wt, ribi_t::south)
						&& east->get_neighbour(north_east, track_wt, ribi_t::north)
						&& east->get_neighbour(east_east, track_wt, ribi_t::east))
				{
					ribi_t::ribi south_masked_ribi = south->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi north_east_masked_ribi = north_east->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi east_east_masked_ribi = east_east->get_weg(track_wt)->get_ribi_masked();
					if((north_east_masked_ribi & ribi_t::north) && (south_masked_ribi & ribi_t::southwest) && east_east->get_weg(track_wt)->get_ribi_unmasked() == ribi_t::northwest && (east_east_masked_ribi & ribi_t::north)){
						return desc->get_crossing_image_id(ribi, false, front);
					}
				}
				
				if(desc->has_switch_ex_image() && from->is_use_wayobj_ex_image(ribi_t::backward(ribi))){
					return desc->get_crossing_image_ex_id(ribi, false, front);
				}
			}
			else if(desc->has_switch_ex_image() && from->is_use_wayobj_ex_image(ribi_t::backward(ribi))){
				return desc->get_crossing_image_ex_id(ribi, false, front);
			}
		}
	}
	else if(ribi == ribi_t::northsouthwest){
		grund_t *from = welt->lookup(get_pos());
		grund_t *west = welt->lookup(from->get_pos() + koord3d(-1,0,0));
		if(west && west->get_wayobj(track_wt)){
			ribi_t::ribi west_ribi = west->get_wayobj(track_wt)->get_dir();
			if(west_ribi == ribi_t::southeast){
				return desc->get_crossing_image_id(ribi,true,front);
			}
			else if(west_ribi == ribi_t::northeast){
				return desc->get_crossing_image_id(ribi,false,front);
			}
			else if(west_ribi == ribi_t::northsoutheast){
				grund_t *north;
				grund_t *south;
				grund_t *north_west;
				grund_t *south_west;
				if(  from->get_neighbour(north, track_wt, ribi_t::north) && from->get_neighbour(south, track_wt, ribi_t::south)
						&& west->get_neighbour(north_west, track_wt, ribi_t::north) && west->get_neighbour(south_west, track_wt, ribi_t::south))
				{
					ribi_t::ribi north_masked_ribi = north->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi south_masked_ribi = south->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi north_west_masked_ribi = north_west->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi south_west_masked_ribi = south_west->get_weg(track_wt)->get_ribi_masked();

					if((south_masked_ribi & ribi_t::northeast) && (north_west_masked_ribi & ribi_t::southwest) && (north_masked_ribi != ribi_t::north || south_west_masked_ribi != ribi_t::south)){
						return desc->get_crossing_image_id(ribi,true,front);
					}
					else if((south_masked_ribi & ribi_t::southeast) && (north_west_masked_ribi & ribi_t::northwest) && (north_masked_ribi != ribi_t::south || south_west_masked_ribi != ribi_t::north)){
						return desc->get_crossing_image_id(ribi,true,front);
					}
					else if((north_masked_ribi & ribi_t::northeast) && (south_west_masked_ribi & ribi_t::southwest) && (south_masked_ribi != ribi_t::north || north_west_masked_ribi != ribi_t::south)){
						return desc->get_crossing_image_id(ribi,false,front);
					}
					else if((north_masked_ribi & ribi_t::southeast) && (south_west_masked_ribi & ribi_t::northwest) && (south_masked_ribi != ribi_t::south || north_west_masked_ribi != ribi_t::north)){
						return desc->get_crossing_image_id(ribi,false,front);
					}
				}
			}
			else if(desc->has_switch_ex_image() && from->is_use_wayobj_ex_image(ribi_t::backward(ribi))){
				return desc->get_crossing_image_ex_id(ribi, false, front);
			}
		}
	}
	else if(ribi == ribi_t::northeastwest){
		grund_t *from = welt->lookup(get_pos());
		grund_t *north = welt->lookup(from->get_pos() + koord3d(0,-1,0));
		if(north && north->get_wayobj(track_wt)){
			ribi_t::ribi north_ribi = north->get_wayobj(track_wt)->get_dir();
			if(north_ribi == ribi_t::southeast){
				return desc->get_crossing_image_id(ribi,true,front);
			}
			else if(north_ribi == ribi_t::southwest){
				return desc->get_crossing_image_id(ribi,false,front);
			}
			else if(north_ribi == ribi_t::southeastwest){
				grund_t *west;
				grund_t *east;
				grund_t *north_west;
				grund_t *north_east;
				if(  from->get_neighbour(west, track_wt, ribi_t::west) && from->get_neighbour(east, track_wt, ribi_t::east)
						&& north->get_neighbour(north_west, track_wt, ribi_t::west) && north->get_neighbour(north_east, track_wt, ribi_t::east))
				{
					ribi_t::ribi west_masked_ribi = west->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi east_masked_ribi = east->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi north_west_masked_ribi = north_west->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi north_east_masked_ribi = north_east->get_weg(track_wt)->get_ribi_masked();

					if((east_masked_ribi & ribi_t::southwest) && (north_west_masked_ribi & ribi_t::northeast) && (west_masked_ribi != ribi_t::west || north_east_masked_ribi != ribi_t::east)){
						return desc->get_crossing_image_id(ribi,true,front);
					}
					else if((east_masked_ribi & ribi_t::southeast) && (north_west_masked_ribi & ribi_t::northwest) && (west_masked_ribi != ribi_t::east || north_east_masked_ribi != ribi_t::west)){
						return desc->get_crossing_image_id(ribi,true,front);
					}
					else if((west_masked_ribi & ribi_t::southwest) && (north_east_masked_ribi & ribi_t::northeast) && (east_masked_ribi != ribi_t::west || north_west_masked_ribi != ribi_t::east)){
						return desc->get_crossing_image_id(ribi,false,front);
					}
					else if((west_masked_ribi & ribi_t::southeast) && (north_east_masked_ribi & ribi_t::northwest) && (east_masked_ribi != ribi_t::east || north_west_masked_ribi != ribi_t::west)){
						return desc->get_crossing_image_id(ribi,false,front);
					}
				}
			}
			else if(desc->has_switch_ex_image() && from->is_use_wayobj_ex_image(ribi_t::backward(ribi))){
				return desc->get_crossing_image_ex_id(ribi, false, front);
			}
		}
	}
	else if(ribi == ribi_t::southeastwest){
		grund_t *from = welt->lookup(get_pos());
		grund_t *south = welt->lookup(from->get_pos() + koord3d(0,1,0));
		if(south && south->get_wayobj(track_wt)){
			ribi_t::ribi south_ribi = south->get_wayobj(track_wt)->get_dir();
			if(south_ribi == ribi_t::northwest){
				return desc->get_crossing_image_id(ribi,true,front);
			}
			else if(south_ribi == ribi_t::northeast){
				return desc->get_crossing_image_id(ribi,false,front);
			}
			else if(south_ribi == ribi_t::northeastwest){
				grund_t *west;
				grund_t *east;
				grund_t *south_west;
				grund_t *south_east;
				if(  from->get_neighbour(west, track_wt, ribi_t::west) && from->get_neighbour(east, track_wt, ribi_t::east)
						&& south->get_neighbour(south_west, track_wt, ribi_t::west) && south->get_neighbour(south_east, track_wt, ribi_t::east))
				{
					ribi_t::ribi west_masked_ribi = west->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi east_masked_ribi = east->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi south_west_masked_ribi = south_west->get_weg(track_wt)->get_ribi_masked();
					ribi_t::ribi south_east_masked_ribi = south_east->get_weg(track_wt)->get_ribi_masked();
					if((west_masked_ribi & ribi_t::northwest) && (south_east_masked_ribi & ribi_t::southeast) && (east_masked_ribi != ribi_t::west || south_west_masked_ribi != ribi_t::east)){
						return desc->get_crossing_image_id(ribi,true,front);
					}
					else if((west_masked_ribi & ribi_t::northeast) && (south_east_masked_ribi & ribi_t::southwest) && (east_masked_ribi != ribi_t::east || south_west_masked_ribi != ribi_t::west)){
						return desc->get_crossing_image_id(ribi,true,front);
					}
					else if((east_masked_ribi & ribi_t::northwest) && (south_west_masked_ribi & ribi_t::southeast) && (west_masked_ribi != ribi_t::west || south_east_masked_ribi != ribi_t::east)){
						return desc->get_crossing_image_id(ribi,false,front);
					}
					else if((east_masked_ribi & ribi_t::northeast) && (south_west_masked_ribi & ribi_t::southwest) && (west_masked_ribi != ribi_t::east || south_east_masked_ribi != ribi_t::west)){
						return desc->get_crossing_image_id(ribi,false,front);
					}
				}
			}
			else if(desc->has_switch_ex_image() && from->is_use_wayobj_ex_image(ribi_t::backward(ribi))){
				return desc->get_crossing_image_ex_id(ribi, false, front);
			}
		}
	}
	else if(ribi == ribi_t::all && desc->has_switch_ex_image()){
		grund_t *from = welt->lookup(get_pos());
		grund_t *north;
		grund_t *south;
		grund_t *west;
		grund_t *east;
		if(  from->get_neighbour(north, track_wt, ribi_t::north) && from->get_neighbour(south, track_wt, ribi_t::south)
		&& from->get_neighbour(west, track_wt, ribi_t::west) && from->get_neighbour(east, track_wt, ribi_t::east)
		&& north->get_wayobj(track_wt) && south->get_wayobj(track_wt) && west->get_wayobj(track_wt) && east->get_wayobj(track_wt))
		{
			ribi_t::ribi north_ribi = north->get_wayobj(track_wt)->get_dir();
			ribi_t::ribi south_ribi = south->get_wayobj(track_wt)->get_dir();
			ribi_t::ribi west_ribi = west->get_wayobj(track_wt)->get_dir();
			ribi_t::ribi east_ribi = east->get_wayobj(track_wt)->get_dir();
			if(ribi_t::is_bend(north_ribi) && ribi_t::is_bend(south_ribi) && (north_ribi & south_ribi)){
				return desc->get_crossing_image_ex_id((ribi_t::backward(north_ribi) | ribi_t::backward(south_ribi)),true,front);
			}
			else if(ribi_t::is_bend(east_ribi) && ribi_t::is_bend(west_ribi) && (east_ribi & west_ribi)){
				return desc->get_crossing_image_ex_id((ribi_t::backward(east_ribi) | ribi_t::backward(west_ribi)),true,front);
			}
			ribi_t::ribi north_masked_ribi = north->get_weg(track_wt)->get_ribi_masked();
			// ribi_t::ribi south_masked_ribi = south->get_weg(get_waytype())->get_ribi_masked();
			
			if( east_ribi == ribi_t::northwest && east->get_weg(track_wt)->get_ribi() == ribi_t::west){
				grund_t *southwest;
				if( west->get_neighbour(southwest, track_wt, ribi_t::south) ){
					ribi_t::ribi southwest_masked_ribi = southwest->get_weg(track_wt)->get_ribi_masked();
					if((north_masked_ribi & ribi_t::north) && (southwest_masked_ribi & ribi_t::southwest)){						
						return desc->get_crossing_image_ex_id(ribi_t::southeastwest,false,front);
					}
				}
			}
/* 			else if( east_ribi == ribi_t::southwest ){
				grund_t *northwest;
				if( west->get_neighbour(northwest, get_waytype(), ribi_t::north) && northwest->get_weg(get_waytype())){
					ribi_t::ribi northwest_masked_ribi = northwest->get_weg(get_waytype())->get_ribi_masked();
					if((south_masked_ribi & ribi_t::south) && (northwest_masked_ribi & ribi_t::north)){
						set_images(image_ex, ribi_t::northeastwest, snow, false);
						west->get_weg(get_waytype())->set_images(image_switch, west_ribi, snow, true);
						return;
					}
				}
			} */
		}
	}

	return front ? desc->get_front_image_id(ribi) : desc->get_back_image_id(ribi);
}