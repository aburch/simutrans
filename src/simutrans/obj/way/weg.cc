/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "weg.h"

#include "schiene.h"
#include "strasse.h"
#include "monorail.h"
#include "maglev.h"
#include "narrowgauge.h"
#include "kanal.h"
#include "runway.h"


#include "../../ground/grund.h"
#include "../../world/simworld.h"
#include "../../display/simimg.h"
#include "../../simhalt.h"
#include "../../obj/simobj.h"
#include "../../player/simplay.h"
#include "../../obj/roadsign.h"
#include "../../obj/signal.h"
#include "../../obj/crossing.h"
#include "../../utils/cbuffer.h"
#include "../../dataobj/environment.h" // TILE_HEIGHT_STEP
#include "../../dataobj/translator.h"
#include "../../dataobj/loadsave.h"
#include "../../descriptor/way_desc.h"
#include "../../descriptor/roadsign_desc.h"

#include "../../tpl/slist_tpl.h"

#ifdef MULTI_THREAD
#include "../../utils/simthread.h"
static pthread_mutex_t weg_calc_image_mutex;
static recursive_mutex_maker_t weg_cim_maker(weg_calc_image_mutex);
#endif

/**
 * Alle instantiierten Wege
 */
slist_tpl <weg_t *> alle_wege;


/**
 * Get list of all ways
 */
const slist_tpl <weg_t *> & weg_t::get_alle_wege()
{
	return alle_wege;
}


// returns a way with matching waytype
weg_t* weg_t::alloc(waytype_t wt)
{
	weg_t *weg = NULL;
	switch(wt) {
		case tram_wt:
		case track_wt:
			weg = new schiene_t();
			break;
		case monorail_wt:
			weg = new monorail_t();
			break;
		case maglev_wt:
			weg = new maglev_t();
			break;
		case narrowgauge_wt:
			weg = new narrowgauge_t();
			break;
		case road_wt:
			weg = new strasse_t();
			break;
		case water_wt:
			weg = new kanal_t();
			break;
		case air_wt:
			weg = new runway_t();
			break;
		default:
			// keep compiler happy; should never reach here anyway
			assert(0);
			break;
	}
	return weg;
}


// returns a string with the "official name of the waytype"
const char *weg_t::waytype_to_string(waytype_t wt)
{
	switch(wt) {
		case tram_wt:        return "tram_track";
		case track_wt:       return "track";
		case monorail_wt:    return "monorail_track";
		case maglev_wt:      return "maglev_track";
		case narrowgauge_wt: return "narrowgauge_track";
		case road_wt:        return "road";
		case water_wt:       return "water";
		case air_wt:         return "air_wt";
		default:
			// keep compiler happy; should never reach here anyway
			break;
	}
	return "invalid waytype";
}


void weg_t::set_desc(const way_desc_t *b)
{
	desc = b;

	if(  hat_gehweg() &&  desc->get_wtyp() == road_wt  &&  desc->get_topspeed() > 50  ) {
		max_speed = 50;
	}
	else {
		max_speed = desc->get_topspeed();
	}
}


/**
 * initializes statistic array
 */
void weg_t::init_statistics()
{
	for(  int type=0;  type<MAX_WAY_STATISTICS;  type++  ) {
		for(  int month=0;  month<MAX_WAY_STAT_MONTHS;  month++  ) {
			statistics[month][type] = 0;
		}
	}
}


/**
 * Initializes all member variables
 */
void weg_t::init()
{
	ribi = ribi_maske = ribi_t::none;
	max_speed = 450;
	desc = 0;
	init_statistics();
	alle_wege.insert(this);
	flags = 0;
	image = IMG_EMPTY;
	foreground_image = IMG_EMPTY;
}


weg_t::~weg_t()
{
	alle_wege.remove(this);
	player_t *player=get_owner();
	if(player) {
		player_t::add_maintenance( player,  -desc->get_maintenance(), desc->get_finance_waytype() );
	}
}


void weg_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "weg_t" );

	// save owner
	if(  file->is_version_atleast(99, 6)  ) {
		sint8 spnum=get_owner_nr();
		file->rdwr_byte(spnum);
		set_owner_nr(spnum);
	}

	// all connected directions
	uint8 dummy8 = ribi;
	file->rdwr_byte(dummy8);
	if(  file->is_loading()  ) {
		ribi = dummy8 & 15; // before: high bits was maske
		ribi_maske = 0; // maske will be restored by signal/roadsing
	}

	uint16 dummy16=max_speed;
	file->rdwr_short(dummy16);
	max_speed=dummy16;

	if(  file->is_version_atleast(89, 0)  ) {
		dummy8 = flags;
		file->rdwr_byte(dummy8);
		if(  file->is_loading()  ) {
			// all other flags are restored afterwards
			flags = dummy8 & HAS_SIDEWALK;
		}
	}

	for(  int type=0;  type<MAX_WAY_STATISTICS;  type++  ) {
		for(  int month=0;  month<MAX_WAY_STAT_MONTHS;  month++  ) {
			sint32 w = statistics[month][type];
			file->rdwr_long(w);
			statistics[month][type] = (sint16)w;
			// DBG_DEBUG("weg_t::rdwr()", "statistics[%d][%d]=%d", month, type, statistics[month][type]);
		}
	}
}


void weg_t::info(cbuffer_t & buf) const
{
	obj_t::info(buf);

	buf.printf("%s %u%s", translator::translate("Max. speed:"), max_speed, translator::translate("km/h\n"));
	buf.printf("%s%u",    translator::translate("\nRibi (unmasked)"), get_ribi_unmasked());
	buf.printf("%s%u\n",  translator::translate("\nRibi (masked)"),   get_ribi());

	if(has_sign()) {
		buf.append(translator::translate("\nwith sign/signal\n"));
	}

	if(is_electrified()) {
		buf.append(translator::translate("\nelektrified"));
	}
	else {
		buf.append(translator::translate("\nnot elektrified"));
	}

#if 1
	buf.printf(translator::translate("convoi passed last\nmonth %i\n"), statistics[1][1]);
#else
	// Debug - output stats
	buf.append("\n");
	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=0; month<MAX_WAY_STAT_MONTHS; month++) {
			buf.printf("%d ", statistics[month][type]);
		}
	buf.append("\n");
	}
#endif
	buf.append("\n");
	if (char const* const maker = get_desc()->get_copyright()) {
		buf.printf(translator::translate("Constructed by %s"), maker);
		buf.append("\n");
	}
}


/**
 * called during map rotation
 */
void weg_t::rotate90()
{
	obj_t::rotate90();
	ribi = ribi_t::rotate90( ribi );
	ribi_maske = ribi_t::rotate90( ribi_maske );
}


/**
 * counts signals on this tile;
 * It would be enough for the signals to register and unregister themselves, but this is more secure ...
 */
void weg_t::count_sign()
{
	// Either only sign or signal please ...
	flags &= ~(HAS_SIGN|HAS_SIGNAL|HAS_CROSSING);
	const grund_t *gr=welt->lookup(get_pos());
	if(gr) {
		uint8 i = 1;
		// if there is a crossing, the start index is at least three ...
		if(  gr->ist_uebergang()  ) {
			flags |= HAS_CROSSING;
			i = 3;
			const crossing_t* cr = gr->find<crossing_t>();
			uint32 top_speed = cr->get_desc()->get_maxspeed( cr->get_desc()->get_waytype(0)==get_waytype() ? 0 : 1);
			if(  top_speed < max_speed  ) {
				max_speed = top_speed;
			}
		}
		// since way 0 is at least present here ...
		for( ;  i<gr->get_top();  i++  ) {
			obj_t *obj=gr->obj_bei(i);
			// sign for us?
			if(  roadsign_t const* const sign = obj_cast<roadsign_t>(obj)  ) {
				if(  sign->get_desc()->get_wtyp() == get_desc()->get_wtyp()  ) {
					// here is a sign ...
					flags |= HAS_SIGN;
					return;
				}
			}
			if(  signal_t const* const signal = obj_cast<signal_t>(obj)  ) {
				if(  signal->get_desc()->get_wtyp() == get_desc()->get_wtyp()  ) {
					// here is a signal ...
					flags |= HAS_SIGNAL;
					return;
				}
			}
		}
	}
}


void weg_t::set_images(image_type typ, uint8 ribi, bool snow, bool switch_nw)
{
	switch(typ) {
		case image_flat:
		default:
			set_image( desc->get_image_id( ribi, snow ) );
			set_foreground_image( desc->get_image_id( ribi, snow, true ) );
			break;
		case image_slope:
			set_image( desc->get_slope_image_id( (slope_t::type)ribi, snow ) );
			set_foreground_image( desc->get_slope_image_id( (slope_t::type)ribi, snow, true ) );
			break;
		case image_switch:
			set_image( desc->get_switch_image_id(ribi, snow, switch_nw) );
			set_foreground_image( desc->get_switch_image_id(ribi, snow, switch_nw, true) );
			break;
		case image_diagonal:
			set_image( desc->get_diagonal_image_id(ribi, snow) );
			set_foreground_image( desc->get_diagonal_image_id(ribi, snow, true) );
			break;
	}
}


// much faster recalculation of season image
bool weg_t::check_season(const bool calc_only_season_change)
{
	if(  calc_only_season_change  ) { // nothing depends on season, only snowline
		return true;
	}

	// no way to calculate this or no image set (not visible, in tunnel mouth, etc)
	if(  desc == NULL  ||  image == IMG_EMPTY  ) {
		return true;
	}

	grund_t *from = welt->lookup( get_pos() );
	if(  from->ist_bruecke()  &&  from->obj_bei(0) == this  ) {
		// first way on a bridge (bruecke_t will set the image)
		return true;
	}

	// use snow image if above snowline and above ground
	bool snow = (from->ist_karten_boden()  ||  !from->ist_tunnel())  &&  (get_pos().z  + from->get_weg_yoff()/TILE_HEIGHT_STEP >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate);
	bool old_snow = (flags&IS_SNOW) != 0;
	if(  !(snow ^ old_snow)  ) {
		// season is not changing ...
		return true;
	}

	// set snow flake
	flags &= ~IS_SNOW;
	if(  snow  ) {
		flags |= IS_SNOW;
	}

	slope_t::type hang = from->get_weg_hang();
	if(  hang != slope_t::flat  ) {
		set_images( image_slope, hang, snow );
		return true;
	}

	if(  is_diagonal()  ) {
		set_images( image_diagonal, ribi, snow );
	}
	else if(  ribi_t::is_threeway( ribi )  &&  desc->has_switch_image()  ) {
		// there might be two states of the switch; remember it when changing seasons
		if(  image == desc->get_switch_image_id( ribi, old_snow, false )  ) {
			set_images( image_switch, ribi, snow, false );
		}
		else if(  image == desc->get_switch_image_id( ribi, old_snow, true )  ) {
			set_images( image_switch, ribi, snow, true );
		}
		else {
			set_images( image_flat, ribi, snow );
		}
	}
	else {
		set_images( image_flat, ribi, snow );
	}

	return true;
}


#ifdef MULTI_THREAD
void weg_t::lock_mutex()
{
	pthread_mutex_lock( &weg_calc_image_mutex );
}


void weg_t::unlock_mutex()
{
	pthread_mutex_unlock( &weg_calc_image_mutex );
}
#endif


void weg_t::calc_image()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &weg_calc_image_mutex );
#endif
	grund_t *from = welt->lookup(get_pos());
	grund_t *to;
	image_id old_image = image;

	if(  from==NULL  ||  desc==NULL  ) {
		// no ground, in tunnel
		set_image(IMG_EMPTY);
		set_foreground_image(IMG_EMPTY);
		if(  from==NULL  ) {
			dbg->error( "weg_t::calc_image()", "Own way at %s not found!", get_pos().get_str() );
		}
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &weg_calc_image_mutex );
#endif
		return; // otherwise crashing during enlargement
	}
	else if(  from->ist_tunnel() &&  from->ist_karten_boden()  &&  (grund_t::underground_mode==grund_t::ugm_none || (grund_t::underground_mode==grund_t::ugm_level && from->get_hoehe()<grund_t::underground_level))  ) {
		// handled by tunnel mouth, no underground mode
//		set_image(IMG_EMPTY);
//		set_foreground_image(IMG_EMPTY);
	}
	else if(  from->ist_bruecke()  &&  from->obj_bei(0)==this  ) {
		// first way on a bridge (bruecke_t will set the image)
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &weg_calc_image_mutex );
#endif
		return;
	}
	else {
		// use snow image if above snowline and above ground
		bool snow = (from->ist_karten_boden()  ||  !from->ist_tunnel())  &&  (get_pos().z + from->get_weg_yoff()/TILE_HEIGHT_STEP >= welt->get_snowline() || welt->get_climate( get_pos().get_2d() ) == arctic_climate  );
		flags &= ~IS_SNOW;
		if(  snow  ) {
			flags |= IS_SNOW;
		}

		slope_t::type hang = from->get_weg_hang();
		if(hang != slope_t::flat) {
			// on slope
			set_images(image_slope, hang, snow);
		}
		else {
			static int recursion = 0; /* Communicate among different instances of this method */

			// flat way
			set_images(image_flat, ribi, snow);

			// recalc image of neighbors also when this changed to non-diagonal
			if(recursion == 0) {
				recursion++;
				for(int r = 0; r < 4; r++) {
					if(  from->get_neighbour(to, get_waytype(), ribi_t::nesw[r])  ) {
						// can fail on water tiles
						if(  weg_t *w=to->get_weg(get_waytype())  )  {
							// and will only change the outcome, if it has a diagonal image ...
							if(  w->get_desc()->has_diagonal_image()  ) {
								w->calc_image();
							}
						}
					}
				}
				recursion--;
			}

			// try diagonal image
			if(  desc->has_diagonal_image()  ) {
				check_diagonal();

				// now apply diagonal image
				if(is_diagonal()) {
					if( desc->get_diagonal_image_id(ribi, snow) != IMG_EMPTY  ||
					    desc->get_diagonal_image_id(ribi, snow, true) != IMG_EMPTY) {
						set_images(image_diagonal, ribi, snow);
					}
				}
			}
		}
	}
	if(  image!=old_image  ) {
		mark_image_dirty(old_image, from->get_weg_yoff());
		mark_image_dirty(image, from->get_weg_yoff());
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &weg_calc_image_mutex );
#endif
}


// checks, if this way qualifies as diagonal
void weg_t::check_diagonal()
{
	bool diagonal = false;
	flags &= ~IS_DIAGONAL;

	const ribi_t::ribi ribi = get_ribi_unmasked();
	if(  !ribi_t::is_bend(ribi)  ) {
		// This is not a curve, it can't be a diagonal
		return;
	}

	grund_t *from = welt->lookup(get_pos());
	grund_t *to;

	ribi_t::ribi r1 = ribi_t::none;
	ribi_t::ribi r2 = ribi_t::none;

	// get the ribis of the ways that connect to us
	// r1 will be 45 degree clockwise ribi (eg northeast->east), r2 will be anticlockwise ribi (eg northeast->north)
	if(  from->get_neighbour(to, get_waytype(), ribi_t::rotate45(ribi))  ) {
		r1 = to->get_weg_ribi_unmasked(get_waytype());
	}

	if(  from->get_neighbour(to, get_waytype(), ribi_t::rotate45l(ribi))  ) {
		r2 = to->get_weg_ribi_unmasked(get_waytype());
	}

	// diagonal if r1 or r2 are our reverse and neither one is 90 degree rotation of us
	diagonal = (r1 == ribi_t::backward(ribi) || r2 == ribi_t::backward(ribi)) && r1 != ribi_t::rotate90l(ribi) && r2 != ribi_t::rotate90(ribi);

	if(  diagonal  ) {
		flags |= IS_DIAGONAL;
	}
}


/**
 * new month
 */
void weg_t::new_month()
{
	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=MAX_WAY_STAT_MONTHS-1; month>0; month--) {
			statistics[month][type] = statistics[month-1][type];
		}
		statistics[0][type] = 0;
	}
}


// correct speed and maintenance
void weg_t::finish_rd()
{
	player_t *player = get_owner();
	if(  player  &&  desc  ) {
		player_t::add_maintenance( player,  desc->get_maintenance(), desc->get_finance_waytype() );
	}
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *weg_t::is_deletable(const player_t *player)
{
	if(  get_owner_nr()==PUBLIC_PLAYER_NR  ) {
		return NULL;
	}
	return obj_t::is_deletable(player);
}
