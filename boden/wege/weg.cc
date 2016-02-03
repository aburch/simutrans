/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Basisklasse für Wege in Simutrans.
 *
 * 14.06.00 getrennt von simgrund.cc
 * Überarbeitet Januar 2001
 *
 * derived from simobj.h in 2007
 *
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../tpl/slist_tpl.h"

#include "weg.h"

#include "schiene.h"
#include "strasse.h"
#include "monorail.h"
#include "maglev.h"
#include "narrowgauge.h"
#include "kanal.h"
#include "runway.h"


#include "../grund.h"
#include "../../simmesg.h"
#include "../../simworld.h"
#include "../../display/simimg.h"
#include "../../simhalt.h"
#include "../../simobj.h"
#include "../../player/simplay.h"
#include "../../obj/wayobj.h"
#include "../../obj/roadsign.h"
#include "../../obj/signal.h"
#include "../../obj/crossing.h"
#include "../../obj/bruecke.h"
#include "../../obj/tunnel.h"
#include "../../obj/gebaeude.h" // for ::should_city_adopt_this
#include "../../utils/cbuffer_t.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/loadsave.h"
#include "../../besch/weg_besch.h"
#include "../../besch/tunnel_besch.h"
#include "../../besch/roadsign_besch.h"
#include "../../besch/haus_besch.h" // for ::should_city_adopt_this

#include "../../bauer/wegbauer.h"

#ifdef MULTI_THREAD
#include "../../utils/simthread.h"
static pthread_mutex_t weg_calc_bild_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

/**
 * Alle instantiierten Wege
 * @author Hj. Malthaner
 */
slist_tpl <weg_t *> alle_wege;


/**
 * Get list of all ways
 * @author Hj. Malthaner
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
		case tram_wt:	return "tram_track";
		case track_wt:	return "track";
		case monorail_wt: return "monorail_track";
		case maglev_wt: return "maglev_track";
		case narrowgauge_wt: return "narrowgauge_track";
		case road_wt:	return "road";
		case water_wt:	return "water";
		case air_wt:	return "air";
		default:
			// keep compiler happy; should never reach here anyway
			break;
	}
	return "invalid waytype";
}


void weg_t::set_besch(const weg_besch_t *b, bool from_saved_game)
{
	if(besch)
	{
		// Remove the old maintenance cost
		sint32 old_maint = get_besch()->get_wartung();
		check_diagonal();
		if(is_diagonal())
		{
			old_maint *= 10;
			old_maint /= 14;
		}
		player_t::add_maintenance(get_owner(), -old_maint, get_besch()->get_finance_waytype());
	}
	
	besch = b;
	// Add the new maintenance cost
	sint32 maint = get_besch()->get_wartung();
	if(is_diagonal())
	{
		maint *= 10;
		maint /= 14;
	}
	player_t::add_maintenance(get_owner(), maint, get_besch()->get_finance_waytype());

	grund_t* gr = welt->lookup(get_pos());
	if(!gr)
	{
		gr = welt->lookup_kartenboden(get_pos().get_2d());
	}
	const bruecke_t *bridge = gr ? gr->find<bruecke_t>() : NULL;
	const tunnel_t *tunnel = gr ? gr->find<tunnel_t>() : NULL;
	const hang_t::typ hang = gr ? gr->get_weg_hang() : hang_t::flach;

	if(hang != hang_t::flach) 
	{
		const uint slope_height = (hang & 7) ? 1 : 2;
		if(slope_height == 1)
		{
			if(bridge)
			{
				max_speed = min(besch->get_topspeed_gradient_1(), bridge->get_besch()->get_topspeed_gradient_1());
			}
			else if(tunnel)
			{
				max_speed = min(besch->get_topspeed_gradient_1(), tunnel->get_besch()->get_topspeed_gradient_1());
			}
			else
			{
				max_speed = besch->get_topspeed_gradient_1();
			}
		}
		else
		{
			if(bridge)
			{
				max_speed = min(besch->get_topspeed_gradient_2(), bridge->get_besch()->get_topspeed_gradient_2());
			}
			else if(tunnel)
			{
				max_speed = min(besch->get_topspeed_gradient_2(), tunnel->get_besch()->get_topspeed_gradient_2());
			}
			else
			{
				max_speed = besch->get_topspeed_gradient_2();
			}
		}
	}
	else
	{
		if(bridge)
			{
				max_speed = min(besch->get_topspeed(), bridge->get_besch()->get_topspeed());
			}
		else if(tunnel)
			{
				max_speed = min(besch->get_topspeed(), tunnel->get_besch()->get_topspeed());
			}
			else
			{
				max_speed = besch->get_topspeed();
			}
	}

	const sint32 city_road_topspeed = welt->get_city_road()->get_topspeed();

	if(hat_gehweg() && besch->get_wtyp() == road_wt)
	{
		max_speed = min(max_speed, city_road_topspeed);
	}
	
	max_axle_load = besch->get_max_axle_load();
	
	// Clear the old constraints then add all sources of constraints again.
	// (Removing will not work in cases where a way and another object, 
	// such as a bridge, tunnel or wayobject, share a constraint).
	clear_way_constraints();
	add_way_constraints(besch->get_way_constraints()); // Add the way's own constraints
	if(bridge)
	{
		add_way_constraints(bridge->get_besch()->get_way_constraints());
	}
	if(tunnel)
	{
		add_way_constraints(tunnel->get_besch()->get_way_constraints());
	}
	const wayobj_t* wayobj = gr ? gr->get_wayobj(get_waytype()) : NULL;
	if(wayobj)
	{
		add_way_constraints(wayobj->get_besch()->get_way_constraints());
	}

	if(besch->is_mothballed())
	{	
		degraded = true;
		remaining_wear_capacity = 0;
	}
	else if(!from_saved_game)
	{
		remaining_wear_capacity = besch->get_wear_capacity();
		last_renewal_month_year = welt->get_timeline_year_month();
		degraded = false;
		replacement_way = besch;
		const grund_t* gr = welt->lookup(get_pos());
		if(gr)
		{
			roadsign_t* rs = gr->find<roadsign_t>();
			if(!rs)
			{
				rs =  gr->find<signal_t>();
			} 
			if(rs && rs->get_besch()->is_retired(welt->get_timeline_year_month()))
			{
				// Upgrade obsolete signals and signs when upgrading the underlying way if possible.
				rs->upgrade(welt->lookup_kartenboden(get_pos().get_2d())->get_hoehe() != get_pos().z); 
			}
		}
	}
}


/**
 * initializes statistic array
 * @author hsiegeln
 */
void weg_t::init_statistics()
{
	for(  int type=0;  type<MAX_WAY_STATISTICS;  type++  ) {
		for(  int month=0;  month<MAX_WAY_STAT_MONTHS;  month++  ) {
			statistics[month][type] = 0;
		}
	}
	creation_month_year = welt->get_timeline_year_month();
}


/**
 * Initializes all member variables
 * @author Hj. Malthaner
 */
void weg_t::init()
{
	ribi = ribi_maske = ribi_t::keine;
	max_speed = 450;
	max_axle_load = 1000;
	bridge_weight_limit = UINT32_MAX_VALUE;
	besch = 0;
	init_statistics();
	alle_wege.insert(this);
	flags = 0;
	image = IMG_LEER;
	after_bild = IMG_LEER;
	public_right_of_way = false;
	degraded = false;
	remaining_wear_capacity = 100000000;
	replacement_way = NULL;
}


weg_t::~weg_t()
{
	alle_wege.remove(this);
	player_t *player=get_owner();
	if(player  &&  besch) 
	{
		sint32 maint = besch->get_wartung();
		if(is_diagonal())
		{
			maint *= 10;
			maint /= 14;
		}
		player_t::add_maintenance( player,  -maint, besch->get_finance_waytype() );
	}
}


void weg_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "weg_t" );

	// save owner
	if(  file->get_version() >= 99006  ) {
		sint8 spnum=get_player_nr();
		file->rdwr_byte(spnum);
		set_player_nr(spnum);
	}

	// all connected directions
	uint8 dummy8 = ribi;
	file->rdwr_byte(dummy8);
	if(  file->is_loading()  ) {
		ribi = dummy8 & 15;	// before: high bits was maske
		ribi_maske = 0;	// maske will be restored by signal/roadsing
	}

	sint16 dummy16=max_speed;
	file->rdwr_short(dummy16);
	max_speed=dummy16;

	if(  file->get_version() >= 89000  ) {
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

	if(file->get_experimental_version() >= 1)
	{
		if (max_axle_load > 32768) {
			dbg->error("weg_t::rdwr()", "Max axle load out of range");
		}
		uint16 wdummy16 = max_axle_load;
		file->rdwr_short(wdummy16);
		max_axle_load = wdummy16;
	}

	if(file->get_experimental_version() >= 12)
	{
		bool prow = public_right_of_way;
		file->rdwr_bool(prow);
		public_right_of_way = prow;
#ifdef SPECIAL_RESCUE_12_3
		if(file->is_saving())
		{
			uint32 rwc = remaining_wear_capacity;
			file->rdwr_long(rwc);
			remaining_wear_capacity = rwc;
			uint16 cmy = creation_month_year;
			file->rdwr_short(cmy);
			creation_month_year = cmy;
			uint16 lrmy = last_renewal_month_year;
			file->rdwr_short(lrmy);
			last_renewal_month_year = lrmy;
			bool deg = degraded;
			file->rdwr_bool(deg);
			degraded = deg;
		}
#else
		uint32 rwc = remaining_wear_capacity;
		file->rdwr_long(rwc);
		remaining_wear_capacity = rwc;
		uint16 cmy = creation_month_year;
		file->rdwr_short(cmy);
		creation_month_year = cmy;
		uint16 lrmy = last_renewal_month_year;
		file->rdwr_short(lrmy);
		last_renewal_month_year = lrmy;
		bool deg = degraded;
		file->rdwr_bool(deg);
		degraded = deg;
#endif
	}
}


/**
 * Info-text für diesen Weg
 * @author Hj. Malthaner
 */
void weg_t::info(cbuffer_t & buf, bool is_bridge) const
{
	obj_t::info(buf);
	if(public_right_of_way)
	{
		buf.append(translator::translate("Public right of way"));
		buf.append("\n\n");
	}
	if(degraded)
	{
		buf.append(translator::translate("Degraded"));
		buf.append("\n\n");
	}
	buf.append(translator::translate("Max. speed:"));
	buf.append(" ");
	buf.append(max_speed);
	buf.append(translator::translate("km/h\n"));
	if(besch->get_styp() == weg_t::type_elevated || waytype == air_wt || waytype == water_wt)
	{
		buf.append(translator::translate("\nMax. weight:"));
	}
	else
	{
		buf.append(translator::translate("\nMax. axle load:"));
	}
	buf.append(" ");
	buf.append(max_axle_load);
	buf.append(translator::translate("tonnen"));
	buf.append("\n");
	if(is_bridge && bridge_weight_limit < UINT32_MAX_VALUE)
	{
		buf.append(translator::translate("Max. bridge weight:"));
		buf.append(" ");
		buf.append(bridge_weight_limit);
		buf.append(translator::translate("tonnen"));
		buf.append("\n");
	}

	buf.append("\n");
	buf.append(translator::translate("Condition"));
	buf.append(": ");
	char tmpbuf_cond[40];
	sprintf(tmpbuf_cond, "%u%%", get_condition_percent());
	buf.append(tmpbuf_cond);
	buf.append("\n");
	buf.append(translator::translate("Built"));
	buf.append(": ");
	char tmpbuf_built[40];
	sprintf(tmpbuf_built, "%s, %i", translator::get_month_name(creation_month_year%12), creation_month_year/12 );
	buf.append(tmpbuf_built);
	buf.append("\n");
	buf.append(translator::translate("Last renewed"));
	buf.append(": ");
	char tmpbuf_renewed[40];
	sprintf(tmpbuf_renewed, "%s, %i", translator::get_month_name(last_renewal_month_year%12), last_renewal_month_year/12 );
	buf.append(tmpbuf_renewed);
	buf.append("\n");
	buf.append(translator::translate("To be renewed with"));
	buf.append(": ");
	if(replacement_way)
	{
		const uint16 time = welt->get_timeline_year_month();
		bool is_current = !time || replacement_way->get_intro_year_month() <= time && time < replacement_way->get_retire_year_month();
		if(!is_current)
		{
			buf.append(translator::translate(wegbauer_t::weg_search(replacement_way->get_waytype(), replacement_way->get_topspeed(), (const sint32)replacement_way->get_axle_load(), time, (weg_t::system_type)replacement_way->get_styp(), replacement_way->get_wear_capacity())->get_name()));
		}
		else
		{
			buf.append(translator::translate(replacement_way->get_name()));
		}
	}
	else
	{
		buf.append(translator::translate("keine"));
	}
	buf.append("\n");
	
	for(sint8 i = 0; i < way_constraints.get_count(); i ++)
	{
		if(way_constraints.get_permissive(i))
		{
			buf.append("\n");
			char tmpbuf[30];
			sprintf(tmpbuf, "Permissive %i", i);
			buf.append(translator::translate(tmpbuf));
			buf.append("\n");
		}
	}
	bool any_prohibitive = false;
	for(sint8 i = 0; i < way_constraints.get_count(); i ++)
	{
		if(way_constraints.get_prohibitive(i))
		{
			if(!any_prohibitive)
			{
				buf.append("\n");
				buf.append("Restrictions:");
			}
			any_prohibitive = true;
			buf.append("\n");
			char tmpbuf[30];
			sprintf(tmpbuf, "Prohibitive %i", i);
			buf.append(translator::translate(tmpbuf));
			buf.append("\n");
		}
	}
#ifdef DEBUG
	buf.append(translator::translate("\nRibi (unmasked)"));
	buf.append(get_ribi_unmasked());

	buf.append(translator::translate("\nRibi (masked)"));
	buf.append(get_ribi());
	buf.append("\n");
#endif
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
}


/**
 * called during map rotation
 * @author prissi
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
 * @author prissi
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
			sint32 top_speed = cr->get_besch()->get_maxspeed( cr->get_besch()->get_waytype(0)==get_waytype() ? 0 : 1);
			if(  top_speed < max_speed  ) {
				max_speed = top_speed;
			}
		}
		// since way 0 is at least present here ...
		for( ;  i<gr->get_top();  i++  ) {
			obj_t *obj=gr->obj_bei(i);
			// sign for us?
			if(  roadsign_t const* const sign = obj_cast<roadsign_t>(obj)  ) {
				if(  sign->get_besch()->get_wtyp() == get_besch()->get_wtyp()  ) {
					// here is a sign ...
					flags |= HAS_SIGN;
					return;
				}
			}
			if(  signal_t const* const signal = obj_cast<signal_t>(obj)  ) {
				if(  signal->get_besch()->get_wtyp() == get_besch()->get_wtyp()  ) {
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
			set_bild( besch->get_bild_nr( ribi, snow ) );
			set_after_bild( besch->get_bild_nr( ribi, snow, true ) );
			break;
		case image_slope:
			set_bild( besch->get_hang_bild_nr( (hang_t::typ)ribi, snow ) );
			set_after_bild( besch->get_hang_bild_nr( (hang_t::typ)ribi, snow, true ) );
			break;
		case image_switch:
			set_bild( besch->get_bild_nr_switch(ribi, snow, switch_nw) );
			set_after_bild( besch->get_bild_nr_switch(ribi, snow, switch_nw, true) );
			break;
		case image_diagonal:
			set_bild( besch->get_diagonal_bild_nr(ribi, snow) );
			set_after_bild( besch->get_diagonal_bild_nr(ribi, snow, true) );
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
	if(  besch == NULL  ||  image == IMG_LEER  ) {
		return true;
	}

	grund_t *from = welt->lookup( get_pos() );

	// use snow image if above snowline and above ground
	bool snow = (from->ist_karten_boden()  ||  !from->ist_tunnel())  &&  (get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate);
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

	hang_t::typ hang = from->get_weg_hang();
	if(  hang != hang_t::flach  ) {
		set_images( image_slope, hang, snow );
		return true;
	}

	if(  is_diagonal()  ) 
	{
		if( besch->get_diagonal_bild_nr(ribi, snow) != IMG_LEER  ||
			besch->get_diagonal_bild_nr(ribi, snow, true) != IMG_LEER) 
		{
			set_images(image_diagonal, ribi, snow);
		}
		else
		{
			set_images(image_flat, ribi, snow);
		}
	}
	else if(  ribi_t::is_threeway( ribi )  &&  besch->has_switch_bild()  ) {
		// there might be two states of the switch; remember it when changing seasons
		if(  image == besch->get_bild_nr_switch( ribi, old_snow, false )  ) {
			set_images( image_switch, ribi, snow, false );
		}
		else if(  image == besch->get_bild_nr_switch( ribi, old_snow, true )  ) {
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
	pthread_mutex_lock( &weg_calc_bild_mutex );
}


void weg_t::unlock_mutex()
{
	pthread_mutex_unlock( &weg_calc_bild_mutex );
}
#endif


void weg_t::calc_image()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &weg_calc_bild_mutex );
#endif
	grund_t *from = welt->lookup(get_pos());
	grund_t *to;
	image_id old_bild = image;

	if(  from==NULL  ||  besch==NULL  ||  !from->is_visible()  ) {
		// no ground, in tunnel
		set_bild(IMG_LEER);
		set_after_bild(IMG_LEER);
		if(  from==NULL  ) {
			dbg->error( "weg_t::calc_image()", "Own way at %s not found!", get_pos().get_str() );
		}
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &weg_calc_bild_mutex );
#endif
		return;	// otherwise crashing during enlargement
	}
	else if(  from->ist_tunnel() &&  from->ist_karten_boden()  &&  (grund_t::underground_mode==grund_t::ugm_none || (grund_t::underground_mode==grund_t::ugm_level && from->get_hoehe()<grund_t::underground_level))  ) {
		// in tunnel mouth, no underground mode
		// TODO: Consider special treatment of tunnel portal images here.
		set_bild(IMG_LEER);
		set_after_bild(IMG_LEER);
	}
	else {
		// use snow image if above snowline and above ground
		bool snow = (from->ist_karten_boden()  ||  !from->ist_tunnel())  &&  (get_pos().z >= welt->get_snowline() || welt->get_climate( get_pos().get_2d() ) == arctic_climate  );
		flags &= ~IS_SNOW;
		if(  snow  ) {
			flags |= IS_SNOW;
		}

		hang_t::typ hang = from->get_weg_hang();
		if(hang != hang_t::flach) {
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
					if(  from->get_neighbour(to, get_waytype(), ribi_t::nsow[r])  ) {
						// can fail on water tiles
						if(  weg_t *w=to->get_weg(get_waytype())  )  {
							// and will only change the outcome, if it has a diagonal image ...
							if(  w->get_besch()->has_diagonal_bild()  ) {
								w->calc_image();
							}
						}
					}
				}
				recursion--;
			}

			// try diagonal image
			if(  besch->has_diagonal_bild()  ) {
				check_diagonal();

				// now apply diagonal image
				if(is_diagonal()) {
					if( besch->get_diagonal_bild_nr(ribi, snow) != IMG_LEER  ||
					    besch->get_diagonal_bild_nr(ribi, snow, true) != IMG_LEER) {
						set_images(image_diagonal, ribi, snow);
					}
				}
			}
		}
	}
	if (image!=old_bild && from != NULL) {
		mark_image_dirty(old_bild, from->get_weg_yoff());
		mark_image_dirty(image, from->get_weg_yoff());
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &weg_calc_bild_mutex );
#endif
}

// checks, if this way qualifies as diagonal
void weg_t::check_diagonal()
{
	bool diagonal = false;
	flags &= ~IS_DIAGONAL;

	const ribi_t::ribi ribi = get_ribi_unmasked();
	if(  !ribi_t::ist_kurve(ribi)  ) {
		// This is not a curve, it can't be a diagonal
		return;
	}

	grund_t *from = welt->lookup(get_pos());
	grund_t *to;

	ribi_t::ribi r1 = ribi_t::keine;
	ribi_t::ribi r2 = ribi_t::keine;

	// get the ribis of the ways that connect to us
	// r1 will be 45 degree clockwise ribi (eg nordost->ost), r2 will be anticlockwise ribi (eg nordost->nord)
	if(  from->get_neighbour(to, get_waytype(), ribi_t::rotate45(ribi))  ) {
		r1 = to->get_weg_ribi_unmasked(get_waytype());
	}

	if(  from->get_neighbour(to, get_waytype(), ribi_t::rotate45l(ribi))  ) {
		r2 = to->get_weg_ribi_unmasked(get_waytype());
	}

	// diagonal if r1 or r2 are our reverse and neither one is 90 degree rotation of us
	diagonal = (r1 == ribi_t::rueckwaerts(ribi) || r2 == ribi_t::rueckwaerts(ribi)) && r1 != ribi_t::rotate90l(ribi) && r2 != ribi_t::rotate90(ribi);

	if(  diagonal  ) {
		flags |= IS_DIAGONAL;
	}
}


/**
 * new month
 * @author hsiegeln
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
	player_t *player=get_owner();
	if(player && besch) 
	{
		sint32 maint = besch->get_wartung();
		check_diagonal();
		if(is_diagonal())
		{
			maint *= 10;
			maint /= 14;
		}
		player_t::add_maintenance( player,  maint, besch->get_finance_waytype() );
	}
}


// returns NULL, if removal is allowed
// players can remove public owned ways (Depracated)
const char *weg_t:: is_deletable(const player_t *player, bool allow_public)
{
	if(allow_public && get_player_nr() == 1) 
	{
		return NULL;
	}
	return obj_t:: is_deletable(player);
}

/**
 *  Check whether the city should adopt the road.
 *  (Adopting the road sets a speed limit and builds a sidewalk.)
 */
bool weg_t::should_city_adopt_this(const player_t* player)
{
	if(!welt->get_settings().get_towns_adopt_player_roads())
	{
		return false;
	}
	if(get_waytype() != road_wt) 
	{
		// Cities only adopt roads
		return false;
	}
	if(get_besch()->get_styp() == weg_t::type_elevated)
	{
		// It would be too profitable for players if cities adopted elevated roads
		return false;
	}
	if(get_besch()->get_styp() == weg_t::type_underground) 
	{
		// It would be too profitable for players if cities adopted tunnels
		return false;
	}
	if(player && player->is_public_service())
	{
		// Do not adopt public service roads, so that players can't mess with them
		return false;
	}
	const koord & pos = this->get_pos().get_2d();
	if(!welt->get_city(pos))
	{
		// Don't adopt roads outside the city limits.
		// Note, this also returns false on invalid coordinates
		return false;
	}

	bool has_neighbouring_building = false;
	for(uint8 i = 0; i < 8; i ++)
	{
		// Look for neighbouring buildings at ground level
		const grund_t* const gr = welt->lookup_kartenboden(pos + koord::neighbours[i]);
		if(!gr)
		{
			continue;
		}
		const gebaeude_t* const neighbouring_building = gr->find<gebaeude_t>();
		if(!neighbouring_building) 
		{
			continue;
		}
		const haus_besch_t* const besch = neighbouring_building->get_tile()->get_besch();
		// Most buildings count, including station extension buildings.
		// But some do *not*, namely platforms and depots.
		switch(besch->get_typ())
		{
			case gebaeude_t::wohnung:
			case gebaeude_t::gewerbe:
			case gebaeude_t::industrie:
				has_neighbouring_building = true;
				break;
			default:
				; // keep looking
		}
		switch(besch->get_utyp())
		{
			case haus_besch_t::attraction_city:
			case haus_besch_t::attraction_land:
			case haus_besch_t::denkmal: // monument
			case haus_besch_t::fabrik: // factory
			case haus_besch_t::rathaus: // town hall
			case haus_besch_t::generic_extension:
			case haus_besch_t::firmensitz: // HQ
			case haus_besch_t::dock: // dock
			case haus_besch_t::flat_dock: 
				has_neighbouring_building = (bool)welt->get_city(pos);
				break;
			case haus_besch_t::depot:
			case haus_besch_t::signalbox:
			case haus_besch_t::generic_stop:
			default:
				; // continue checking
		}
	}
	// If we found a neighbouring building, we will adopt the road.
	return has_neighbouring_building;
}

uint32 weg_t::get_condition_percent() const
{
	if(besch->get_wear_capacity() == 0)
	{
		// Necessary to avoid divisions by zero on mothballed ways
		return 0;
	}
	// Necessary to avoid overflow. Speed not important as this is for the UI.
	// Running calculations should use fractions (e.g., "if(remaining_wear_capacity < besch->get_wear_capacity() / 6)"). 
	const sint64 remaining_wear_capacity_percent = (sint64)remaining_wear_capacity  * 100ll;
	const sint64 intermediate_result = remaining_wear_capacity_percent / (sint64)besch->get_wear_capacity();
	return (uint32)intermediate_result; 
}

void weg_t::wear_way(uint32 wear)
{
	if(remaining_wear_capacity == UINT32_MAX_VALUE)
	{
		// If ways are defined with UINT32_MAX_VALUE,
		// this feature is intended to be disabled.
		return;
	}
	if(remaining_wear_capacity > wear)
	{
		const uint32 degridation_fraction = welt->get_settings().get_way_degridation_fraction();
		remaining_wear_capacity -= wear;
		if(remaining_wear_capacity < besch->get_wear_capacity() / degridation_fraction)
		{
			if(!renew())
			{
				degrade();
			}
		}
	}
	else
	{
		remaining_wear_capacity = 0;
		if(!renew())
		{
			degrade();
		}
	}
}

bool weg_t::renew()
{
	if(!replacement_way)
	{
		return false;
	}

	player_t* const player = get_owner();
	bool success = false;
	const sint64 price = besch->get_upgrade_group() == replacement_way->get_upgrade_group() ? replacement_way->get_way_only_cost() : replacement_way->get_preis();
	if((!player && welt->get_city(get_pos().get_2d())) || (player && (player->can_afford(price) || player->is_public_service())))
	{
		// Unowned ways in cities are assumed to be owned by the city and will be renewed by it.
		const uint16 time = welt->get_timeline_year_month();
		bool is_current = !time || (replacement_way->get_intro_year_month() <= time && time < replacement_way->get_retire_year_month());
		if(!is_current)
		{
			replacement_way = wegbauer_t::weg_search(replacement_way->get_waytype(), replacement_way->get_topspeed(), (const sint32)replacement_way->get_axle_load(), time, (weg_t::system_type)replacement_way->get_styp(), replacement_way->get_wear_capacity());
		}
		
		if(!replacement_way)
		{
			// If the way search cannot find a replacement way, use the current way as a fallback.
			replacement_way = besch;
		}
		
		set_besch(replacement_way);
		success = true;
		if(player)
		{
			player->book_way_renewal(price, replacement_way->get_waytype());
		}
	}
	else if(player && !player->get_has_been_warned_about_no_money_for_renewals())
	{
		welt->get_message()->add_message(translator::translate("Not enough money to carry out essential way renewal work.\n"), get_pos().get_2d(), message_t::warnings, player->get_player_nr());
		player->set_has_been_warned_about_no_money_for_renewals(true); // Only warn once a month.
	}

	return success;
}

void weg_t::degrade()
{
	if(public_right_of_way)
	{
		// Do not degrade public rights of way, as these should remain passable.
		// Instead, take them out of private ownership and renew them with the default way.
		set_owner(NULL); 
		if(waytype == road_wt)
		{
			const stadt_t* city = welt->get_city(get_pos().get_2d()); 
			const weg_besch_t* wb = city ? welt->get_settings().get_city_road_type(welt->get_timeline_year_month()) : welt->get_settings().get_intercity_road_type(welt->get_timeline_year_month());
			set_besch(wb ? wb : besch);
		}
		else
		{
			set_besch(besch);
		}
	}
	else
	{
		if(remaining_wear_capacity)
		{
			// There is some wear left, but this way is in a degraded state. Reduce the speed limit.
			if(!degraded)
			{
				// Only do this once, or else this will carry on reducing for ever.
				max_speed /= 2;
				degraded = true;
			}
		}
		else
		{
			// Totally worn out: impassable. 
			max_speed = 0;
			degraded = true;
			const weg_besch_t* mothballed_type = wegbauer_t::way_search_mothballed(get_waytype(), (weg_t::system_type)besch->get_styp()); 
			if(mothballed_type)
			{
				set_besch(mothballed_type);
				calc_image();
			}
		}
	}
}

signal_t *weg_t::get_signal(ribi_t::ribi direction_of_travel) const
{
	signal_t* sig = welt->lookup(get_pos())->find<signal_t>();
	ribi_t::ribi way_direction = (ribi_t::ribi)ribi_maske;
	if((direction_of_travel & way_direction) == 0)
	{
		return sig;
	}
	else return NULL;
}