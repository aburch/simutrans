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
 * derived from simdings.h in 2007
 *
 * von Hj. Malthaner
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


#include "../grund.h"
#include "../../simworld.h"
#include "../../simimg.h"
#include "../../simhalt.h"
#include "../../simdings.h"
#include "../../player/simplay.h"
#include "../../dings/roadsign.h"
#include "../../dings/signal.h"
#include "../../dings/crossing.h"
#include "../../utils/cbuffer_t.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/loadsave.h"
#include "../../besch/weg_besch.h"
#include "../../besch/roadsign_besch.h"

#include "../../tpl/slist_tpl.h"

#if MULTI_THREAD>1
#include <pthread.h>
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
			weg = new schiene_t(welt);
			break;
		case monorail_wt:
			weg = new monorail_t(welt);
			break;
		case maglev_wt:
			weg = new maglev_t(welt);
			break;
		case narrowgauge_wt:
			weg = new narrowgauge_t(welt);
			break;
		case road_wt:
			weg = new strasse_t(welt);
			break;
		case water_wt:
			weg = new kanal_t(welt);
			break;
		case air_wt:
			weg = new runway_t(welt);
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


/**
 * Setzt neue Beschreibung. Ersetzt alte Höchstgeschwindigkeit
 * mit wert aus Beschreibung.
 * @author Hj. Malthaner
 */
void weg_t::set_besch(const weg_besch_t *b)
{
	besch = b;
	if(  hat_gehweg() &&  besch->get_wtyp() == road_wt  &&  besch->get_topspeed() > 50  ) {
		max_speed = 50;
	}
	else {
		max_speed = besch->get_topspeed();
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
}


/**
 * Initializes all member variables
 * @author Hj. Malthaner
 */
void weg_t::init()
{
	ribi = ribi_maske = ribi_t::keine;
	max_speed = 450;
	besch = 0;
	init_statistics();
	alle_wege.insert(this);
	flags = 0;
	bild = IMG_LEER;
	after_bild = IMG_LEER;
}


weg_t::~weg_t()
{
	alle_wege.remove(this);
	spieler_t *sp=get_besitzer();
	if(sp) {
		spieler_t::add_maintenance( sp,  -besch->get_wartung() );
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

	uint16 dummy16=max_speed;
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
}


/**
 * Info-text für diesen Weg
 * @author Hj. Malthaner
 */
void weg_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);

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
}


/**
 * called during map rotation
 * @author prissi
 */
void weg_t::rotate90()
{
	ding_t::rotate90();
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
			uint32 top_speed = cr->get_besch()->get_maxspeed( cr->get_besch()->get_waytype(0)==get_waytype() ? 0 : 1);
			if(  top_speed < max_speed  ) {
				max_speed = top_speed;
			}
		}
		// since way 0 is at least present here ...
		for( ;  i<gr->get_top();  i++  ) {
			ding_t *d=gr->obj_bei(i);
			// sign for us?
			if(  roadsign_t const* const sign = ding_cast<roadsign_t>(d)  ) {
				if(  sign->get_besch()->get_wtyp() == get_besch()->get_wtyp()  ) {
					// here is a sign ...
					flags |= HAS_SIGN;
					return;
				}
			}
			if(  signal_t const* const signal = ding_cast<signal_t>(d)  ) {
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
bool weg_t::check_season( const long )
{
	// use snow image if above snowline and above ground
	bool snow = (get_pos().z >= welt->get_snowline());
	bool old_snow = (flags&IS_SNOW)!=0;
	if(  !(snow ^ old_snow)  ) {
		// season is not changing ...
		return true;
	}

	// no way to calculate this or no image set (not visible, in tunnel mouth, etc)
	if(  besch==NULL  ||  bild==IMG_LEER  ) {
		return true;
	}

	grund_t *from = welt->lookup(get_pos());
	if(  from->ist_bruecke()  &&  from->obj_bei(0)==this  ) {
		// first way on a bridge (bruecke_t will set the image)
		return true;
	}

	// set new season
	flags &= ~IS_SNOW;
	if(  snow  ) {
		flags |= IS_SNOW;
	}

	hang_t::typ hang = from->get_weg_hang();
	if(hang != hang_t::flach) {
		set_images(image_slope, hang, snow);
		return true;
	}

	if(  is_diagonal()  ) {
		set_images(image_diagonal, ribi, snow);
	}
	else if(  ribi_t::is_threeway(ribi)  &&  besch->has_switch_bild()  ) {
		// there might be two states of the switch; remeber it when changing saesons
		if(  bild==besch->get_bild_nr_switch(ribi, old_snow, false)  ) {
			set_images(image_switch, ribi, snow, false);
		}
		else if(  bild==besch->get_bild_nr_switch(ribi, old_snow, true)  ) {
			set_images(image_switch, ribi, snow, true);
		}
		else {
			set_images(image_flat, ribi, snow);
		}
	}
	else {
		set_images(image_flat, ribi, snow);
	}

	return true;
}


#if MULTI_THREAD>1
void weg_t::lock_mutex()
{
	pthread_mutex_lock( &weg_calc_bild_mutex );
}


void weg_t::unlock_mutex()
{
	pthread_mutex_unlock( &weg_calc_bild_mutex );
}
#endif


void weg_t::calc_bild()
{
#if MULTI_THREAD>1
	pthread_mutex_lock( &weg_calc_bild_mutex );
#endif
	grund_t *from = welt->lookup(get_pos());
	grund_t *to;
	image_id old_bild = bild;

	if(  from==NULL  ||  besch==NULL  ||  !from->is_visible()  ) {
		// no ground, in tunnel
		set_bild(IMG_LEER);
		set_after_bild(IMG_LEER);
		if(  from==NULL  ) {
			dbg->error( "weg_t::calc_bild()", "Own way at %s not found!", get_pos().get_str() );
		}
#if MULTI_THREAD>1
		pthread_mutex_unlock( &weg_calc_bild_mutex );
#endif
		return;	// otherwise crashing during enlargement
	}
	else if(  from->ist_tunnel() &&  from->ist_karten_boden()  &&  (grund_t::underground_mode==grund_t::ugm_none || (grund_t::underground_mode==grund_t::ugm_level && from->get_hoehe()<grund_t::underground_level))  ) {
		// in tunnel mouth, no underground mode
		set_bild(IMG_LEER);
		set_after_bild(IMG_LEER);
	}
	else if(  from->ist_bruecke()  &&  from->obj_bei(0)==this  ) {
		// first way on a bridge (bruecke_t will set the image)
#if MULTI_THREAD>1
		pthread_mutex_unlock( &weg_calc_bild_mutex );
#endif
		return;
	}
	else {
		// use snow image if above snowline and above ground
		bool snow = (!from->ist_tunnel()   ||  from->ist_karten_boden())  &&  (get_pos().z >= welt->get_snowline());
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
			// flat way
			set_images(image_flat, ribi, snow);

			// try diagonal image
			if(  besch->has_diagonal_bild()  ) {

				bool const old_is_diagonal = is_diagonal();

				check_diagonal();

				if(is_diagonal()  || old_is_diagonal) {
					static int recursion = 0; /* Communicate among different instances of this method */

					// recalc image of neighbors also when this changed to non-diagonal
					if(recursion == 0) {
						recursion++;
						for(int r = 0; r < 4; r++) {
							if(from->get_neighbour(to, get_waytype(), ribi_t::nsow[r])) {
								to->get_weg(get_waytype())->calc_bild();
							}
						}
						recursion--;
					}

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
	}
	if(  bild!=old_bild  ) {
		mark_image_dirty(old_bild, from->get_weg_yoff());
		mark_image_dirty(bild, from->get_weg_yoff());
	}
#if MULTI_THREAD>1
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
	ribi_t::ribi r1 = ribi_t::keine, r2 = ribi_t::keine;

	switch(ribi) {

		case ribi_t::nordost:
			if(  from->get_neighbour(to, get_waytype(), ribi_t::ost)  ) {
				r1 = to->get_weg_ribi_unmasked(get_waytype());
			}
			if(  from->get_neighbour(to, get_waytype(), ribi_t::nord)  ) {
				r2 = to->get_weg_ribi_unmasked(get_waytype());
			}
			diagonal =
				(r1 == ribi_t::suedwest || r2 == ribi_t::suedwest) &&
				r1 != ribi_t::nordwest &&
				r2 != ribi_t::suedost;
		break;

		case ribi_t::suedost:
			if(  from->get_neighbour(to, get_waytype(), ribi_t::ost)  ) {
				r1 = to->get_weg_ribi_unmasked(get_waytype());
			}
			if(  from->get_neighbour(to, get_waytype(), ribi_t::sued)  ) {
				r2 = to->get_weg_ribi_unmasked(get_waytype());
			}
			diagonal =
				(r1 == ribi_t::nordwest || r2 == ribi_t::nordwest) &&
				r1 != ribi_t::suedwest &&
				r2 != ribi_t::nordost;
		break;

		case ribi_t::nordwest:
			if(  from->get_neighbour(to, get_waytype(), ribi_t::west)  ) {
				r1 = to->get_weg_ribi_unmasked(get_waytype());
			}
			if(  from->get_neighbour(to, get_waytype(), ribi_t::nord)  ) {
				r2 = to->get_weg_ribi_unmasked(get_waytype());
			}
			diagonal =
				(r1 == ribi_t::suedost || r2 == ribi_t::suedost) &&
				r1 != ribi_t::nordost &&
				r2 != ribi_t::suedwest;
		break;

		case ribi_t::suedwest:
			if(  from->get_neighbour(to, get_waytype(), ribi_t::west)  ) {
				r1 = to->get_weg_ribi_unmasked(get_waytype());
			}
			if(  from->get_neighbour(to, get_waytype(), ribi_t::sued)  ) {
				r2 = to->get_weg_ribi_unmasked(get_waytype());
			}
			diagonal =
				(r1 == ribi_t::nordost || r2 == ribi_t::nordost) &&
				r1 != ribi_t::suedost &&
				r2 != ribi_t::nordwest;
		break;
	}

	if(  diagonal  ) {
		flags |= IS_DIAGONAL;
	}
}


/**
 * new month
 * @author hsiegeln
 */
void weg_t::neuer_monat()
{
	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=MAX_WAY_STAT_MONTHS-1; month>0; month--) {
			statistics[month][type] = statistics[month-1][type];
		}
		statistics[0][type] = 0;
	}
}


// correct speed and maintenance
void weg_t::laden_abschliessen()
{
	spieler_t *sp = get_besitzer();
	if(  sp  &&  besch  ) {
		spieler_t::add_maintenance( sp,  besch->get_wartung() );
	}
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *weg_t::ist_entfernbar(const spieler_t *sp)
{
	if(  get_player_nr()==1  ) {
		return NULL;
	}
	return ding_t::ist_entfernbar(sp);
}
