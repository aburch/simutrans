/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <ctype.h>

#if MULTI_THREAD>1
#include <pthread.h>
static pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t add_to_city_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


#include "../bauer/hausbauer.h"
#include "../gui/money_frame.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simfab.h"
#include "../simimg.h"
#include "../simgraph.h"
#include "../simhalt.h"
#include "../simwin.h"
#include "../simcity.h"
#include "../player/simplay.h"
#include "../simtools.h"
#include "../simdebug.h"
#include "../simintr.h"
#include "../simskin.h"

#include "../boden/grund.h"
#include "../boden/wege/strasse.h"

#include "../besch/haus_besch.h"
#include "../besch/intro_dates.h"

#include "../besch/grund_besch.h"

#include "../utils/cbuffer_t.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"

#include "../gui/thing_info.h"

#include "gebaeude.h"


/**
 * Initializes all variables with save, usable values
 * @author Hj. Malthaner
 */
void gebaeude_t::init()
{
	tile = NULL;
	ptr.fab = 0;
	is_factory = false;
	anim_time = 0;
	sync = false;
	count = 0;
	zeige_baugrube = false;
	snow = false;
	remove_ground = true;
	passengers_generated_local = 0;
	passengers_succeeded_local = 0;
	passenger_success_percent_last_year_local = 0;
	passengers_generated_non_local = 0;
	passengers_succeeded_non_local = 0;
	passenger_success_percent_last_year_non_local = 0;
	available_jobs_by_time = -9223372036854775808ll;
}


#ifdef INLINE_DING_TYPE
gebaeude_t::gebaeude_t(karte_t *welt, ding_t::typ type) : ding_t(welt, type)
#else
gebaeude_t::gebaeude_t(karte_t *welt) : ding_t(welt)
#endif
{
	init();
}


#ifdef INLINE_DING_TYPE
gebaeude_t::gebaeude_t(karte_t *welt, loadsave_t *file) : ding_t(welt, ding_t::gebaeude)
#else
gebaeude_t::gebaeude_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
#endif
{
	init();
	rdwr(file);
	if(file->get_version()<88002) {
		set_yoff(0);
	}
	if(tile  &&  tile->get_phasen()>1) {
		welt->sync_eyecandy_add( this );
		sync = true;
	}
}



#ifdef INLINE_DING_TYPE
gebaeude_t::gebaeude_t(karte_t *welt, ding_t::typ type, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) :
    ding_t(welt, type, pos)
{
	init(sp, t);
}

gebaeude_t::gebaeude_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) :
    ding_t(welt, ding_t::gebaeude, pos)
{
	init(sp, t);
}

void gebaeude_t::init(spieler_t *sp, const haus_tile_besch_t *t)
#else
gebaeude_t::gebaeude_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) :
    ding_t(welt, pos)
#endif
{
	set_besitzer( sp );

	init();
	if(t) 
	{
		set_tile(t,true);	// this will set init time etc.
		sint64 maint;
		if(tile->get_besch()->get_base_maintenance() == COST_MAGIC)
		{
			maint = welt->get_settings().maint_building*tile->get_besch()->get_level();
		}
		else
		{
			maint = tile->get_besch()->get_maintenance();
		}
		spieler_t::add_maintenance(get_besitzer(), maint, tile->get_besch()->get_finance_waytype() );
	}

	if(tile->get_besch()->get_typ() == wohnung)
	{
		people.population = tile->get_besch()->get_population_and_visitor_demand_capacity() == 65535 ? tile->get_besch()->get_level() * welt->get_settings().get_population_per_level() : tile->get_besch()->get_population_and_visitor_demand_capacity();
		adjusted_people.population = welt->calc_adjusted_monthly_figure(people.population);
	}
	else
	{
		people.visitor_demand = tile->get_besch()->get_population_and_visitor_demand_capacity() == 65535 ? tile->get_besch()->get_level() * welt->get_settings().get_visitor_demand_per_level() : tile->get_besch()->get_population_and_visitor_demand_capacity();
		adjusted_people.visitor_demand = welt->calc_adjusted_monthly_figure(people.visitor_demand);
	}
	
	jobs = tile->get_besch()->get_employment_capacity() == 65535 ? (is_monument() || tile->get_besch()->get_typ() == wohnung) ? 0 : tile->get_besch()->get_level() * welt->get_settings().get_jobs_per_level() : tile->get_besch()->get_employment_capacity();
	mail_demand = tile->get_besch()->get_mail_demand_and_production_capacity() == 65535 ? is_monument() ? 0 : tile->get_besch()->get_level() * welt->get_settings().get_mail_per_level() : tile->get_besch()->get_mail_demand_and_production_capacity();

	adjusted_jobs = welt->calc_adjusted_monthly_figure(jobs);
	adjusted_mail_demand = welt->calc_adjusted_monthly_figure(mail_demand);

	grund_t *gr=welt->lookup(get_pos());
	if(gr  &&  gr->get_weg_hang()!=gr->get_grund_hang()) {
		set_yoff(-TILE_HEIGHT_STEP);
	}

	check_road_tiles(false);
}



/**
 * Destructor. Removes this from the list of sync objects if neccesary.
 *
 * @author Hj. Malthaner
 */
gebaeude_t::~gebaeude_t()
{
	stadt_t* our_city = get_stadt();
	if(our_city && !welt->get_is_shutting_down()) 
	{
		our_city->remove_gebaeude_from_stadt(this);
	}

	if(sync) {
		sync = false;
		welt->sync_eyecandy_remove(this);
	}

	
	// tiles might be invalid, if no description is found during loading
	if(tile && tile->get_besch()) 
	{
		check_road_tiles(true);
		if(tile->get_besch()->ist_ausflugsziel())
		{
			welt->remove_ausflugsziel(this);
		}
	}

	count = 0;
	anim_time = 0;
	if(tile) 
	{
		sint64 maint;
		if(tile->get_besch()->get_base_maintenance() == COST_MAGIC)
		{
			maint = welt->get_settings().maint_building * tile->get_besch()->get_level();
		}
		else
		{
			maint = tile->get_besch()->get_maintenance();
		}
		spieler_t::add_maintenance(get_besitzer(), -maint);
	}
	if(!welt->get_is_shutting_down())
	{
		const weighted_vector_tpl<stadt_t*>& staedte = welt->get_staedte();
		for(weighted_vector_tpl<stadt_t*>::const_iterator j = staedte.begin(), end = staedte.end(); j != end; ++j) 
		{
			(*j)->remove_connected_attraction(this);
		}
	}

	welt->remove_building_from_world_list(this);
}


void gebaeude_t::check_road_tiles(bool del)
{
	const haus_besch_t *hb = tile->get_besch();
	const koord3d pos = get_pos() - koord3d(tile->get_offset(), 0);
	koord size = hb->get_groesse(tile->get_layout());
	koord k;
	grund_t* gr_this;

	vector_tpl<gebaeude_t*> building_list;
	building_list.append(this);
	
	for(k.y = 0; k.y < size.y; k.y ++) 
	{
		for(k.x = 0; k.x < size.x; k.x ++) 
		{
			koord3d k_3d = koord3d(k, 0) + pos;
			grund_t *gr = welt->lookup(k_3d);
			if(gr) 
			{
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes
				if(gb_part && gb_part->get_tile()->get_besch() == hb) 
				{
					building_list.append_unique(gb_part);
				}
			}
		}
	}

	ITERATE(building_list, n)
	{
		const gebaeude_t* gb = building_list[n];

		for(uint8 i = 0; i < 8; i ++)
		{
			/* This is tricky: roads can change height, and we're currently
			 * not keeping track of when they do. We might show
			 * up as connecting to a road that's no longer at the right
			 * height. Therefore, iterate over all possible road levels when
			 * removing, but not when adding new connections. */
			koord pos_neighbour = gb->get_pos().get_2d() + (gb->get_pos().get_2d().neighbours[i]);
			if(del) {
				planquadrat_t *plan = welt->lookup(pos_neighbour);
				if(!plan)
				{
					continue;
				}
				for(int j=0; j<plan->get_boden_count(); j++) {
					grund_t *bd = plan->get_boden_bei(j);
					strasse_t *str = (strasse_t *)bd->get_weg(road_wt);

					if(str) {
						str->connected_buildings.remove(this);
					}
				}
			} else {
				koord3d pos3d(pos_neighbour, gb->get_pos().z);

				// Check for connected roads. Only roads in immediately neighbouring tiles
				// and only those on the same height will register a connexion.
				gr_this = welt->lookup(pos3d);

				if(!gr_this)
				{
					continue;
				}
				strasse_t* str = (strasse_t*)gr_this->get_weg(road_wt);
				if(str)
				{
					str->connected_buildings.append_unique(this);
				}
			}
		}
	}
}

void gebaeude_t::rotate90()
{
	ding_t::rotate90();

	// must or can rotate?
	const haus_besch_t* const haus_besch = tile->get_besch();
	if (haus_besch->get_all_layouts() > 1  ||  haus_besch->get_b() * haus_besch->get_h() > 1) {
		uint8 layout = tile->get_layout();
		koord new_offset = tile->get_offset();

		if(haus_besch->get_utyp() == haus_besch_t::unbekannt  ||  haus_besch->get_all_layouts()<=4) {
			layout = (layout & 4) + ((layout+3) % haus_besch->get_all_layouts() & 3);
		}
		else {
			static uint8 layout_rotate[16] = { 1, 8, 5, 10, 3, 12, 7, 14, 9, 0, 13, 2, 11, 4, 15, 6 };
			layout = layout_rotate[layout] % haus_besch->get_all_layouts();
		}
		// have to rotate the tiles :(
		if(  !haus_besch->can_rotate()  &&  haus_besch->get_all_layouts() == 1  ) {
			if ((welt->get_settings().get_rotation() & 1) == 0) {
				// rotate 180 degree
				new_offset = koord(haus_besch->get_b() - 1 - new_offset.x, haus_besch->get_h() - 1 - new_offset.y);
			}
			// do nothing here, since we cannot fix it porperly
		}
		else {
			// rotate on ...
			new_offset = koord(haus_besch->get_h(tile->get_layout()) - 1 - new_offset.y, new_offset.x);
		}

		// suche a tile exist?
		if(  haus_besch->get_b(layout) > new_offset.x  &&  haus_besch->get_h(layout) > new_offset.y  ) {
			const haus_tile_besch_t* const new_tile = haus_besch->get_tile(layout, new_offset.x, new_offset.y);
			// add new tile: but make them old (no construction)
			sint64 old_insta_zeit = insta_zeit;
			set_tile( new_tile, false  );
			insta_zeit = old_insta_zeit;
			if(  haus_besch->get_utyp() != haus_besch_t::hafen  &&  !tile->has_image()  ) {
				// may have a rotation, that is not recoverable
				if(  !is_factory  &&  new_offset!=koord(0,0)  ) {
					welt->set_nosave_warning();
				}
				if(  is_factory  ) {
					// there are factories with a broken tile
					// => this map rotation cannot be reloaded!
					welt->set_nosave();
				}
			}
		}
		else {
			welt->set_nosave();
		}
	}
}



/* sets the corresponding pointer to a factory
 * @author prissi
 */
void gebaeude_t::set_fab(fabrik_t *fb)
{
	// sets the pointer in non-zero
	if(fb) {
		if(!is_factory  &&  ptr.stadt!=NULL) {
			dbg->fatal("gebaeude_t::set_fab()","building already bound to city!");
		}
		is_factory = true;
		ptr.fab = fb;
	}
	else if(is_factory) {
		ptr.fab = NULL;
	}
}



/* sets the corresponding city
 * @author prissi
 */
void gebaeude_t::set_stadt(stadt_t *s)
{
	if(is_factory  &&  ptr.fab!=NULL) {
		dbg->fatal("gebaeude_t::set_stadt()","building at (%s) already bound to factory!", get_pos().get_str() );
	}
	// sets the pointer in non-zero
	is_factory = false;
	ptr.stadt = s;
}




/* make this building without construction */
void gebaeude_t::add_alter(sint64 a)
{
	insta_zeit -= min(a,insta_zeit);
}




void gebaeude_t::set_tile( const haus_tile_besch_t *new_tile, bool start_with_construction )
{
	insta_zeit = welt->get_zeit_ms();

	if(!zeige_baugrube  &&  tile!=NULL) {
		// mark old tile dirty
		sint16 ypos = 0;
		for(  int i=0;  i<256;  i++  ) {
			image_id bild = get_bild(i);
			if(bild==IMG_LEER) {
				break;
			}
			mark_image_dirty( bild, 0 );
			ypos -= get_tile_raster_width();
		}
	}

	zeige_baugrube = !new_tile->get_besch()->ist_ohne_grube()  &&  start_with_construction;
	if(sync) {
		if(new_tile->get_phasen()<=1  &&  !zeige_baugrube) {
			// need to stop animation
#if MULTI_THREAD>1
			pthread_mutex_lock( &sync_mutex );
#endif
			welt->sync_eyecandy_remove(this);
			sync = false;
			count = 0;
#if MULTI_THREAD>1
			pthread_mutex_unlock( &sync_mutex );
#endif
		}
	}
	else if(new_tile->get_phasen()>1  ||  zeige_baugrube) {
		// needs now animation
#if MULTI_THREAD>1
		pthread_mutex_lock( &sync_mutex );
#endif
		count = sim_async_rand(new_tile->get_phasen());
		anim_time = 0;
		welt->sync_eyecandy_add(this);
		sync = true;
#if MULTI_THREAD>1
		pthread_mutex_unlock( &sync_mutex );
#endif
	}
	tile = new_tile;
	remove_ground = tile->has_image()  &&  !tile->get_besch()->ist_mit_boden();
	set_flag(ding_t::dirty);
}


/**
 * Methode für Echtzeitfunktionen eines Objekts.
 * @return false wenn Objekt aus der Liste der synchronen
 * Objekte entfernt werden sol
 * @author Hj. Malthaner
 */
bool gebaeude_t::sync_step(long delta_t)
{
	if(insta_zeit > welt->get_zeit_ms())
	{
		// There were some integer overflow issues with 
		// this when some intermediate values were uint32.
		insta_zeit = welt->get_zeit_ms() - 5000ll;
	}

	if(zeige_baugrube) {
		// still under construction?
		if(welt->get_zeit_ms() - insta_zeit > 5000ll) {
			set_flag(ding_t::dirty);
			mark_image_dirty(get_bild(),0);
			zeige_baugrube = false;
			if(tile->get_phasen()<=1) {
				welt->sync_eyecandy_remove( this );
				sync = false;
			}
		}
	}
	else {
		// normal animated building
		anim_time += (uint16)delta_t;
		if(anim_time>tile->get_besch()->get_animation_time()) {
			if(!zeige_baugrube)  {

				// old positions need redraw
				if(  tile->is_hintergrund_phases( snow )  ) {
					// the background is animated
					set_flag(ding_t::dirty);
					// we must take care of the tiles above
					for(int i=1;  ;  i++) {
						image_id bild = get_bild(i);
						if(bild==IMG_LEER) {
							break;
						}
						mark_image_dirty(bild,-(i<<6));
					}
				}
				else {
					// try foreground
					image_id bild = tile->get_vordergrund(count, snow);
					mark_image_dirty(bild,0);
					// next phase must be marked dirty too ...
					bild = tile->get_vordergrund( count+1>=tile->get_phasen()?0:count+1, snow);
					mark_image_dirty(bild,0);
				}

				anim_time %= tile->get_besch()->get_animation_time();
				count ++;
				if(count >= tile->get_phasen()) {
					count = 0;
				}
				// winter for buildings only above snowline
			}
		}
	}
	return true;
}


void gebaeude_t::calc_bild()
{
	grund_t *gr = welt->lookup(get_pos());
	// need no ground?
	if( gr && remove_ground  &&  gr->get_typ()==grund_t::fundament  ) {
		gr->set_bild( IMG_LEER );
	}
	// snow image?
	snow = 0;
	if( gr && tile->get_seasons()>1  ) {
		snow = (!gr->ist_tunnel()  ||  gr->ist_karten_boden())  &&  (get_pos().z-(get_yoff()/TILE_HEIGHT_STEP)>= welt->get_snowline());
	}
}


image_id gebaeude_t::get_bild() const
{
	if(umgebung_t::hide_buildings!=0  &&  tile->has_image()) {
		// opaque houses
		if(get_haustyp()!=unbekannt) {
			return umgebung_t::hide_with_transparency ? skinverwaltung_t::fussweg->get_bild_nr(0) : skinverwaltung_t::construction_site->get_bild_nr(0);
		} else if(  (umgebung_t::hide_buildings == umgebung_t::ALL_HIDDEN_BUIDLING  &&  tile->get_besch()->get_utyp() < haus_besch_t::weitere)) {
			// hide with transparency or tile without information
			if(umgebung_t::hide_with_transparency) {
				if(tile->get_besch()->get_utyp() == haus_besch_t::fabrik  &&  ptr.fab->get_besch()->get_platzierung() == fabrik_besch_t::Wasser) {
					// no ground tiles for water thingies
					return IMG_LEER;
				}
				return skinverwaltung_t::fussweg->get_bild_nr(0);
			}
			else {
				int kind=skinverwaltung_t::construction_site->get_bild_anzahl()<=tile->get_besch()->get_utyp() ? skinverwaltung_t::construction_site->get_bild_anzahl()-1 : tile->get_besch()->get_utyp();
				return skinverwaltung_t::construction_site->get_bild_nr( kind );
			}
		}
	}

	// winter for buildings only above snowline
	if(zeige_baugrube)  {
		return skinverwaltung_t::construction_site->get_bild_nr(0);
	}
	else {
		return tile->get_hintergrund(count, 0, snow);
	}
}


image_id gebaeude_t::get_outline_bild() const
{
	if(umgebung_t::hide_buildings!=0  &&  umgebung_t::hide_with_transparency  &&  !zeige_baugrube) {
		// opaque houses
		return tile->get_hintergrund(count, 0, snow);
	}
	return IMG_LEER;
}


/* gives outline colour and plots background tile if needed for transparent view */
PLAYER_COLOR_VAL gebaeude_t::get_outline_colour() const
{
	COLOR_VAL colours[] = { COL_BLACK, COL_YELLOW, COL_YELLOW, COL_PURPLE, COL_RED, COL_GREEN };
	PLAYER_COLOR_VAL disp_colour = 0;
	if(umgebung_t::hide_buildings!=umgebung_t::NOT_HIDE) {
		if(get_haustyp()!=unbekannt) {
			disp_colour = colours[0] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		} else if (umgebung_t::hide_buildings == umgebung_t::ALL_HIDDEN_BUIDLING && tile->get_besch()->get_utyp() < haus_besch_t::weitere) {
			// special bilding
			disp_colour = colours[tile->get_besch()->get_utyp()] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		}
	}
	return disp_colour;
}


image_id gebaeude_t::get_bild(int nr) const
{
	if(zeige_baugrube || umgebung_t::hide_buildings) {
		return IMG_LEER;
	}
	else {
		// winter for buildings only above snowline
		return tile->get_hintergrund(count, nr, snow);
	}
}


image_id gebaeude_t::get_after_bild() const
{
	if(zeige_baugrube) {
		return IMG_LEER;
	}
	if (umgebung_t::hide_buildings != 0 && tile->get_besch()->get_utyp() < haus_besch_t::weitere) {
		return IMG_LEER;
	}
	else {
		// Show depots, station buildings etc.
		return tile->get_vordergrund(count, snow);
	}
}


/*
 * This and the next method once multiplied the level by the size
 *  of the building, but this is no longer relevant as of Experimental 12.0
 */
// TODO: Remove this deprecated code.
int gebaeude_t::get_passagier_level() const
{
	return tile->get_besch()->get_level();
}

// TODO: Remove this deprecated code.
int gebaeude_t::get_post_level() const
{
	return tile->get_besch()->get_post_level();	
}

/**
 * @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
 * @author Hj. Malthaner
 */
const char *gebaeude_t::get_name() const
{
	if(is_factory  &&  ptr.fab) {
		return ptr.fab->get_name();
	}
	switch(tile->get_besch()->get_typ()) {
		case wohnung:
			break;//return "Wohnhaus";
		case gewerbe:
			break;//return "Gewerbehaus";
		case industrie:
			break;//return "Industriegebäude";
		default:
			switch(tile->get_besch()->get_utyp()) {
				case haus_besch_t::attraction_city:   return "Besonderes Gebaeude";
				case haus_besch_t::attraction_land:   return "Sehenswuerdigkeit";
				case haus_besch_t::denkmal:           return "Denkmal";
				case haus_besch_t::rathaus:           return "Rathaus";
				case haus_besch_t::depot:			  return tile->get_besch()->get_name();
				default: break;
			}
			break;
	}
	return "Gebaeude";
}


/**
 * waytype associated with this object
 */
waytype_t gebaeude_t::get_waytype() const
{
	const haus_besch_t *besch = tile->get_besch();
	waytype_t wt = invalid_wt;
	if (besch->get_typ() == gebaeude_t::unbekannt) {
		const haus_besch_t::utyp utype = besch->get_utyp();
		if (utype == haus_besch_t::depot  ||  utype == haus_besch_t::generic_stop  ||  utype == haus_besch_t::generic_extension) {
			wt = (waytype_t)besch->get_extra();
		}
	}
	return wt;
}


bool gebaeude_t::ist_rathaus() const
{
	return tile->get_besch()->ist_rathaus();
}


bool gebaeude_t::is_monument() const
{
	return tile->get_besch()->get_utyp() == haus_besch_t::denkmal;
}


bool gebaeude_t::ist_firmensitz() const
{
	return tile->get_besch()->ist_firmensitz();
}

bool gebaeude_t::is_attraction() const
{
	return tile->get_besch()->ist_ausflugsziel();
}


gebaeude_t::typ gebaeude_t::get_haustyp() const
{
	return tile->get_besch()->get_typ();
}


void gebaeude_t::zeige_info()
{
	if(get_fabrik()) {
		ptr.fab->zeige_info();
		return;
	}
	int old_count = win_get_open_count();
	bool special = ist_firmensitz() || ist_rathaus();

	if(ist_firmensitz()) {
		create_win( new money_frame_t(get_besitzer()), w_info, magic_finances_t+get_besitzer()->get_player_nr() );
	}
	else if (ist_rathaus()) {
		welt->suche_naechste_stadt(get_pos().get_2d())->zeige_info();
	}

	if(!tile->get_besch()->ist_ohne_info()) {
		if(!special  ||  (umgebung_t::townhall_info  &&  old_count==win_get_open_count()) ) {
			// open info window for the first tile of our building (not relying on presence of (0,0) tile)
			get_first_tile()->ding_t::zeige_info();
		}
	}
}


gebaeude_t* gebaeude_t::get_first_tile()
{
	const haus_besch_t* const haus_besch = tile->get_besch();
	const uint8 layout = tile->get_layout();
	koord k;
	for(k.x=0; k.x<haus_besch->get_b(layout); k.x++) {
		for(k.y=0; k.y<haus_besch->get_h(layout); k.y++) {
			const haus_tile_besch_t *tile = haus_besch->get_tile(layout, k.x, k.y);
			if (tile==NULL  ||  !tile->has_image()) {
				continue;
			}
			if (grund_t *gr = welt->lookup( get_pos() - get_tile()->get_offset() + k)) {
				gebaeude_t *gb = gr->find<gebaeude_t>();
				if (gb  &&  gb->get_tile() == tile) {
					return gb;
				}
			}
		}
	}
	return this;
}

void gebaeude_t::get_description(cbuffer_t & buf) const
{
	if(is_factory && ptr.fab != NULL) 
	{
		buf.append(ptr.fab->get_name());
	}
	else if(zeige_baugrube) 
	{
		buf.append(translator::translate("Baustelle"));
		buf.append("\n");
	}
	else
	{
		const char *desc = tile->get_besch()->get_name();
		if(desc != NULL)
		{
			const char *trans_desc = translator::translate(desc);
			if(trans_desc == desc) 
			{
				// no description here
				switch(get_haustyp())
				{
					case wohnung:
						trans_desc = translator::translate("residential house");
						break;
					case industrie:
						trans_desc = translator::translate("industrial building");
						break;
					case gewerbe:
						trans_desc = translator::translate("shops and stores");
						break;
					default:
						// use file name
						break;
				}
				buf.append(trans_desc);
			}
			else 
			{
				// since the format changed, we remove all but double newlines
				char *text = new char[strlen(trans_desc)+1];
				char *dest = text;
				const char *src = trans_desc;
				while(  *src!=0  )
				{
					*dest = *src;
					if(src[0]=='\n')
					{
						if(src[1]=='\n')
						{
							src ++;
							dest++;
							*dest = '\n';
						}
						else
						{
							*dest = ' ';
						}
					}
					src ++;
					dest ++;
				}
				// remove double line breaks at the end
				*dest = 0;
				while( dest>text  &&  *--dest=='\n'  ) 
				{
					*dest = 0;
				}

				buf.append(text);
				delete [] text;
			}
		}
		else
		{
			buf.append("unknown");
		}
	}
}


void gebaeude_t::info(cbuffer_t & buf, bool dummy) const
{
	ding_t::info(buf);

	get_description(buf);

	if(!is_factory && !zeige_baugrube) 
	{		
		buf.append("\n\n");

		// belongs to which city?
		if (!is_factory && ptr.stadt != NULL) {
			buf.printf(translator::translate("Town: %s\n"), ptr.stadt->get_name());
		}

		if(  get_tile()->get_besch()->get_utyp() < haus_besch_t::bahnhof  ) {
			buf.printf("%s: %d\n", translator::translate("Passagierrate"), people.population); // TODO: Consider updating the names for this.
			buf.printf("%s: %d\n", translator::translate("Postrate"),      mail_demand);
		}

		haus_besch_t const& h = *tile->get_besch();
		buf.printf("%s%u", translator::translate("\nBauzeit von"), h.get_intro_year_month() / 12);
		if (h.get_retire_year_month() != DEFAULT_RETIRE_DATE * 12) {
			buf.printf("%s%u", translator::translate("\nBauzeit bis"), h.get_retire_year_month() / 12);
		}

		buf.printf("\nTEST total jobs: %i; remaining jobs: %i", get_adjusted_jobs(), check_remaining_available_jobs());
		
		buf.append("\n");
		if(get_besitzer()==NULL) {
			buf.append("\n");
			buf.append(translator::translate("Wert"));
			buf.append(": ");
			buf.append(-(welt->get_settings().cst_buy_land*(tile->get_besch()->get_level())/100) * 5);
			buf.append("$\n");
		}

		if (char const* const maker = tile->get_besch()->get_copyright()) {
			buf.append("\n");
			buf.printf(translator::translate("Constructed by %s"), maker);
		}

		// List of stops potentially within walking distance.
		const planquadrat_t* plan = welt->lookup(get_pos().get_2d());
		const nearby_halt_t *const halt_list = plan->get_haltlist();
		bool any_suitable_stops_passengers = false;
		bool any_suitable_stops_mail = false;
		buf.append("\n\n");

		if(plan->get_haltlist_count() > 0)
		{
			for (int h = plan->get_haltlist_count() - 1; h >= 0; h--) 
			{
				const halthandle_t halt = halt_list[h].halt;
				if (halt->is_enabled(warenbauer_t::passagiere))
				{
					if(!any_suitable_stops_passengers)
					{
						buf.append(translator::translate("Stops potentially within walking distance:"));
						buf.printf("\n(%s)\n\n", translator::translate("Passagiere"));
						any_suitable_stops_passengers = true;
					}
					const karte_t* const world = halt->get_welt();
					const uint16 walking_time = world->walking_time_tenths_from_distance(halt_list[h].distance);
					char walking_time_as_clock[32];
					world->sprintf_time_tenths(walking_time_as_clock, sizeof(walking_time_as_clock), walking_time);
					buf.printf("%s\n%s: %s\n", halt->get_name(), translator::translate("Walking time"), walking_time_as_clock);
				}
			}
			
			for (int h = plan->get_haltlist_count() - 1; h >= 0; h--) 
			{
				const halthandle_t halt = halt_list[h].halt;
				if (halt->is_enabled(warenbauer_t::post))
				{
					if(!any_suitable_stops_mail)
					{
						buf.printf("\n(%s)\n\n", translator::translate("Post"));
						any_suitable_stops_mail = true;
					}
					const karte_t* const world = halt->get_welt();
					const uint16 walking_time = world->walking_time_tenths_from_distance(halt_list[h].distance);
					char walking_time_as_clock[32];
					world->sprintf_time_tenths(walking_time_as_clock, sizeof(walking_time_as_clock), walking_time);
					buf.printf("%s\n%s: %s\n", halt->get_name(), translator::translate("Walking time"), walking_time_as_clock);
				}
			}

		}

		if(!any_suitable_stops_passengers)
		{
			buf.append(translator::translate("No passenger stops within walking distance\n"));
		}

		if(!any_suitable_stops_mail)
		{
			buf.append(translator::translate("\nNo postboxes within walking distance"));
		}
		
		buf.printf("\n\n%s %i%%\n", translator::translate("Passenger success rate this year (local):"), get_passenger_success_percent_this_year_local());
		buf.printf("%s %i%%\n", translator::translate("Passenger success rate last year (local):"), get_passenger_success_percent_last_year_local());
		buf.printf("%s %i%%\n", translator::translate("Passenger success rate this year (non-local):"), get_passenger_success_percent_this_year_non_local());
		buf.printf("%s %i%%\n", translator::translate("Passenger success rate last year (non-local):"), get_passenger_success_percent_last_year_non_local());
	}
}

void gebaeude_t::new_year()
{ 
		passenger_success_percent_last_year_local = get_passenger_success_percent_this_year_local();
		passenger_success_percent_last_year_non_local = get_passenger_success_percent_this_year_non_local(); 

		passengers_succeeded_local = passengers_generated_local = passengers_succeeded_non_local = passengers_generated_non_local = 0; 
}


void gebaeude_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "gebaeude_t" );

	ding_t::rdwr(file);

	char buf[128];
	short idx;

	if(file->is_saving()) {
		const char *s = tile->get_besch()->get_name();
		file->rdwr_str(s);
		idx = tile->get_index();
	}
	else {
		file->rdwr_str(buf, lengthof(buf));
	}
	file->rdwr_short(idx);
	if(file->get_experimental_version() <= 1)
	{
		uint32 old_insta_zeit = (uint32)insta_zeit;
		file->rdwr_long(old_insta_zeit);
		insta_zeit = old_insta_zeit;
	}
	else
	{
		file->rdwr_longlong(insta_zeit);
	}

	if(file->get_experimental_version() >= 12)
	{
		file->rdwr_longlong(available_jobs_by_time);
	}

	if(file->is_loading()) {
		tile = hausbauer_t::find_tile(buf, idx);
		if(tile==NULL) {
			// try with compatibility list first
			tile = hausbauer_t::find_tile(translator::compatibility_name(buf), idx);
			if(tile==NULL) {
				DBG_MESSAGE("gebaeude_t::rdwr()","neither %s nor %s, tile %i not found, try other replacement",translator::compatibility_name(buf),buf,idx);
			}
			else {
				DBG_MESSAGE("gebaeude_t::rdwr()","%s replaced by %s, tile %i",buf,translator::compatibility_name(buf),idx);
			}
		}
		if(tile==NULL) {
			// first check for special buildings
			if(strstr(buf,"TrainStop")!=NULL) {
				tile = hausbauer_t::find_tile("TrainStop", idx);
			} else if(strstr(buf,"BusStop")!=NULL) {
				tile = hausbauer_t::find_tile("BusStop", idx);
			} else if(strstr(buf,"ShipStop")!=NULL) {
				tile = hausbauer_t::find_tile("ShipStop", idx);
			} else if(strstr(buf,"PostOffice")!=NULL) {
				tile = hausbauer_t::find_tile("PostOffice", idx);
			} else if(strstr(buf,"StationBlg")!=NULL) {
				tile = hausbauer_t::find_tile("StationBlg", idx);
			}
			else {
				// try to find a fitting building
				int level=atoi(buf);
				gebaeude_t::typ type = gebaeude_t::unbekannt;

				if(level>0) {
					// May be an old 64er, so we can try some
					if(strncmp(buf+3,"WOHN",4)==0) {
						type = gebaeude_t::wohnung;
					} else if(strncmp(buf+3,"FAB",3)==0) {
						type = gebaeude_t::industrie;
					}
					else {
						type = gebaeude_t::gewerbe;
					}
					level --;
				}
				else if(buf[3]=='_') {
					/* should have the form of RES/IND/COM_xx_level
					 * xx is usually a number by can be anything without underscores
					 */
					level = atoi(strrchr( buf, '_' )+1);
					if(level>0) {
						switch(toupper(buf[0])) {
							case 'R': type = gebaeude_t::wohnung; break;
							case 'I': type = gebaeude_t::industrie; break;
							case 'C': type = gebaeude_t::gewerbe; break;
						}
					}
					level --;
				}
				// we try to replace citybuildings with their matching counterparts
				// if none are matching, we try again without climates and timeline!
				switch(type) {
					case gebaeude_t::wohnung:
						{
							const haus_besch_t *hb = hausbauer_t::get_residential(level,welt->get_timeline_year_month(),welt->get_climate(get_pos().z));
							if(hb==NULL) {
								hb = hausbauer_t::get_residential(level,0, MAX_CLIMATES );
							}
							if( hb) {
								dbg->message("gebaeude_t::rwdr", "replace unknown building %s with residence level %i by %s",buf,level,hb->get_name());
								tile = hb->get_tile(0);
							}
						}
						break;

					case gebaeude_t::gewerbe:
						{
							const haus_besch_t *hb = hausbauer_t::get_commercial(level,welt->get_timeline_year_month(),welt->get_climate(get_pos().z));
							if(hb==NULL) {
								hb = hausbauer_t::get_commercial(level,0, MAX_CLIMATES );
							}
							if(hb) {
								dbg->message("gebaeude_t::rwdr", "replace unknown building %s with commercial level %i by %s",buf,level,hb->get_name());
								tile = hb->get_tile(0);
							}
						}
						break;

					case gebaeude_t::industrie:
						{
							const haus_besch_t *hb = hausbauer_t::get_industrial(level,welt->get_timeline_year_month(),welt->get_climate(get_pos().z));
							if(hb==NULL) {
								hb = hausbauer_t::get_industrial(level,0, MAX_CLIMATES );
								if(hb==NULL) {
									hb = hausbauer_t::get_residential(level,0, MAX_CLIMATES );
								}
							}
							if (hb) {
								dbg->message("gebaeude_t::rwdr", "replace unknown building %s with industrie level %i by %s",buf,level,hb->get_name());
								tile = hb->get_tile(0);
							}
						}
						break;

					default:
						dbg->warning("gebaeude_t::rwdr", "description %s for building at %d,%d not found (will be removed)!", buf, get_pos().x, get_pos().y);
						welt->add_missing_paks( buf, karte_t::MISSING_BUILDING );
				}
			}
		}	// here we should have a valid tile pointer or nothing ...

		/* avoid double contruction of monuments:
		 * remove them from selection lists
		 */
		if (tile  &&  tile->get_besch()->get_utyp() == haus_besch_t::denkmal) {
			hausbauer_t::denkmal_gebaut(tile->get_besch());
		}
		if (tile) {
			remove_ground = tile->has_image()  &&  !tile->get_besch()->ist_mit_boden();
		}
	}

	if(file->get_version()<99006) {
		// ignore the sync flag
		uint8 dummy=sync;
		file->rdwr_byte(dummy);
	}

	if(file->get_experimental_version() >= 12)
	{
		bool f = is_factory;
		file->rdwr_bool(f);
		is_factory = f; 
	}

	// restore city pointer here
	if(file->get_version()>=99014 && !is_factory) 
	{
		sint32 city_index = -1;
		if(  file->is_saving()  &&  ptr.stadt!=NULL  ) {
			city_index = welt->get_staedte().index_of( ptr.stadt );
		}
		file->rdwr_long(city_index);
		if(  file->is_loading()  &&  city_index!=-1  &&  (tile==NULL  ||  tile->get_besch()==NULL  ||  tile->get_besch()->is_connected_with_town())  ) {
			ptr.stadt = welt->get_staedte()[city_index];
		}
	}

	if(file->get_experimental_version() >= 11)
	{
		file->rdwr_short(passengers_generated_local);
		file->rdwr_short(passengers_succeeded_local);
		file->rdwr_byte(passenger_success_percent_last_year_local);

		file->rdwr_short(passengers_generated_non_local);
		file->rdwr_short(passengers_succeeded_non_local);
		file->rdwr_byte(passenger_success_percent_last_year_non_local);
	}

	if(file->get_experimental_version() >= 12)
	{
		file->rdwr_short(people.population); // No need to distinguish the parts of the union here.
		file->rdwr_short(jobs);
		file->rdwr_short(mail_demand);
	}

	if(file->is_loading()) 
	{
		count = 0;
		anim_time = 0;
		sync = false;

		if(tile->get_besch()->get_typ() == wohnung)
		{
			people.population = tile->get_besch()->get_population_and_visitor_demand_capacity() == 65535 ? tile->get_besch()->get_level() * welt->get_settings().get_population_per_level() : tile->get_besch()->get_population_and_visitor_demand_capacity();
			adjusted_people.population = welt->calc_adjusted_monthly_figure(people.population);
		}
		else
		{
			people.visitor_demand = tile->get_besch()->get_population_and_visitor_demand_capacity() == 65535 ? tile->get_besch()->get_level() * welt->get_settings().get_visitor_demand_per_level() : tile->get_besch()->get_population_and_visitor_demand_capacity();
			adjusted_people.visitor_demand = welt->calc_adjusted_monthly_figure(people.visitor_demand);
		}
	
		jobs = tile->get_besch()->get_employment_capacity() == 65535 ? (is_monument() || tile->get_besch()->get_typ() == wohnung) ? 0 : tile->get_besch()->get_level() * welt->get_settings().get_jobs_per_level() : tile->get_besch()->get_employment_capacity();
		mail_demand = tile->get_besch()->get_mail_demand_and_production_capacity() == 65535 ? is_monument() ? 0 : tile->get_besch()->get_level() * welt->get_settings().get_mail_per_level() : tile->get_besch()->get_mail_demand_and_production_capacity();

		adjusted_jobs = welt->calc_adjusted_monthly_figure(jobs);
		adjusted_mail_demand = welt->calc_adjusted_monthly_figure(mail_demand);

		// Hajo: rebuild tourist attraction list
		if(tile && tile->get_besch()->ist_ausflugsziel()) 
		{
			welt->add_ausflugsziel(this);
		}
	}
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 * 
 * "After loading is called adapting to the world - normally used to the
 * look of the thing in the ground and surrounding area" (Google)
 *
 * @author Hj. Malthaner
 */
void gebaeude_t::laden_abschliessen()
{
	calc_bild();
	sint64 maint = tile->get_besch()->get_maintenance();
	if(maint == COST_MAGIC) 
	{
		maint = welt->get_settings().maint_building*tile->get_besch()->get_level();
	}
	spieler_t::add_maintenance(get_besitzer(), maint, tile->get_besch()->get_finance_waytype());

	// citybuilding, but no town?
	if(  tile->get_offset()==koord(0,0)  ) {
		if(  tile->get_besch()->is_connected_with_town()  ) {
			stadt_t *our_city = (ptr.stadt==NULL) ? welt->suche_naechste_stadt( get_pos().get_2d() ) : ptr.stadt;
			if(our_city) {
#if MULTI_THREAD>1
				pthread_mutex_lock( &add_to_city_mutex );
#endif
				our_city->add_gebaeude_to_stadt(this, umgebung_t::networkmode);
#if MULTI_THREAD>1
				pthread_mutex_unlock( &add_to_city_mutex );
#endif
			}
		}
		else if(  !is_factory  ) {
			ptr.stadt = NULL;
		}
	}
}


void gebaeude_t::entferne(spieler_t *sp) // "Remove" (Google)
{
//	DBG_MESSAGE("gebaeude_t::entferne()","gb %i");
	// remove costs

	const haus_besch_t* besch = tile->get_besch();
	sint64 cost = 0;

	if(besch->get_utyp()<haus_besch_t::bahnhof) 
	{
		const sint64 bulldoze_cost =welt->get_settings().cst_multiply_remove_haus * (besch->get_level());
		const sint64 purchase_cost =welt->get_settings().cst_buy_land * besch->get_level() * 5;
		cost = sp != get_besitzer() ? bulldoze_cost + purchase_cost : bulldoze_cost;
		spieler_t::book_construction_costs(sp, cost, get_pos().get_2d(), tile->get_besch()->get_finance_waytype());
	}
	else 
	{
		// tearing down halts is always single costs only
		cost = besch->get_price();
		// This check is necessary because the number of COST_MAGIC is used if no price is specified. 
		if(besch->get_base_price() == COST_MAGIC)
		{
			// TODO: find a way of checking what *kind* of stop that this is. This assumes railway.
			cost = welt->get_settings().cst_multiply_station * besch->get_level();
		}
		// Should be cheaper to bulldoze than build.
		// Currently you recover "scrap money"
		cost = - cost / 1.5F;
		spieler_t::book_construction_costs(sp, cost, get_pos().get_2d(), tile->get_besch()->get_finance_waytype());
	}

	// may need to update next buildings, in the case of start, middle, end buildings
	if(besch->get_all_layouts()>1  &&  get_haustyp()==unbekannt) {

		// realign surrounding buildings...
		uint32 layout = tile->get_layout();

		// detect if we are connected at far (north/west) end
		grund_t * gr = welt->lookup( get_pos() );
		if(gr) {
			sint8 offset = gr->get_weg_yoff()/TILE_HEIGHT_STEP;
			gr = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::ost : koord::sued), offset) );
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::ost : koord::sued),offset - 1) );
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if(gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->get_tile()->get_besch()->get_all_layouts()>4u) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if((layoutbase & 1u) == (layout & 1u)) {
						layoutbase |= 4u; // set far bit on neighbour
						gb->set_tile( gb->get_tile()->get_besch()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}

			// detect if near (south/east) end
			gr = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::west : koord::nord), offset) );
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::west : koord::nord),offset - 1) );
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if(gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->get_tile()->get_besch()->get_all_layouts()>4) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if((layoutbase & 1u) == (layout & 1u)) {
						layoutbase |= 2u; // set near bit on neighbour
						gb->set_tile( gb->get_tile()->get_besch()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}
		}
	}
	mark_images_dirty();
}


void gebaeude_t::mark_images_dirty() const
{
	// remove all traces from the screen
	image_id img;
	if(  zeige_baugrube  ||
			(!umgebung_t::hide_with_transparency  &&
				umgebung_t::hide_buildings>(get_haustyp()!=unbekannt ? umgebung_t::NOT_HIDE : umgebung_t::SOME_HIDDEN_BUIDLING))  ) {
		img = skinverwaltung_t::construction_site->get_bild_nr(0);
	}
	else {
		img = tile->get_hintergrund(count, 0, snow) ;
	}
	for(  int i=0;  img!=IMG_LEER;  img=get_bild(++i)  ) {
		mark_image_dirty( img, -(i*get_tile_raster_width()) );
	}
}

uint16 gebaeude_t::get_weight() const
{
	return tile->get_besch()->get_level();
}

sint64 gebaeude_t::calc_available_jobs_by_time() const
{
	// This assumes that the number of jobs for shops/offices scales with the level. This might not be the best way of doing it (shops have more visitors per job
	// than offices, for example), but to separate this would add great complexity, not least because of the conversion between "level" and actual jobs.
	return welt->get_zeit_ms() - welt->ticks_per_world_month;
}

void gebaeude_t::set_commute_trip(uint16 number)
{
	// Record the number of arriving workers by encoding the earliest time at which new workers can arrive.
	const sint64 total_jobs = (sint64)get_adjusted_jobs();
	const sint64 job_ticks = ((sint64)number * welt->ticks_per_world_month) / (total_jobs < 1 ? 1 : total_jobs);
	const sint64 new_jobs_by_time = calc_available_jobs_by_time();
	available_jobs_by_time = max(new_jobs_by_time + job_ticks, available_jobs_by_time + job_ticks);
}

uint16 gebaeude_t::get_population() const
{
	return get_haustyp() == wohnung ? people.population : 0;
}

uint16 gebaeude_t::get_adjusted_population() const
{
	return get_haustyp() == wohnung ? adjusted_people.population : 0;
}

uint16 gebaeude_t::get_visitor_demand() const
{
	if(get_haustyp() != wohnung)
	{
		return people.visitor_demand;
	}

	uint16 reduced_demand = people.population / 5;
	return reduced_demand > 0 ? reduced_demand : 1;
}

uint16 gebaeude_t::get_adjusted_visitor_demand() const
{
	if(get_haustyp() != wohnung)
	{
		return adjusted_people.visitor_demand;
	}

	uint16 reduced_demand = adjusted_people.population / 5;
	return reduced_demand > 0 ? reduced_demand : 1;
}

uint16 gebaeude_t::get_jobs() const
{
	if(!is_factory)
	{
		return jobs;
	}
	else
	{
		return (uint16)get_fabrik()->get_base_pax_demand();
	}
}

uint16 gebaeude_t::get_adjusted_jobs() const
{
	if(!is_factory)
	{
		return adjusted_jobs;
	}
	else
	{
		return (uint16)get_fabrik()->get_scaled_pax_demand();
	}
}

uint16 gebaeude_t::get_mail_demand() const
{
	if(!is_factory)
	{
		return mail_demand;
	}
	else
	{
		return (uint16)get_fabrik()->get_base_mail_demand();
	}
}

uint16 gebaeude_t::get_adjusted_mail_demand() const
{
	if(!is_factory)
	{
		return adjusted_mail_demand;
	}
	else
	{
		return (uint16)get_fabrik()->get_scaled_mail_demand();
	}
}


sint32 gebaeude_t::check_remaining_available_jobs() const
{
	// Commenting out the "if(!jobs_available())" code will allow jobs to be shown as negative.
	/*if(!jobs_available())
	{
		// All the jobs are taken for the time being.
		//return 0;
	}
	else
	{*/
		const uint32 total_jobs = get_adjusted_jobs();
		if(available_jobs_by_time < welt->get_zeit_ms() - welt->ticks_per_world_month)
		{
			// Uninitialised or stale - all jobs available
			return total_jobs;
		}
		const sint64 delta_t = welt->get_zeit_ms() - available_jobs_by_time;
		const sint64 remaining_jobs = delta_t * total_jobs / welt->ticks_per_world_month;
		return (sint32)remaining_jobs;
	//}
}

bool gebaeude_t::jobs_available() const
{ 
	const sint64 ticks = welt->get_zeit_ms();
	bool difference = available_jobs_by_time <= ticks;
	return difference;
}
