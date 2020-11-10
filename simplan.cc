/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simdebug.h"
#include "simobj.h"
#include "simfab.h"
#include "display/simgraph.h"
#include "simmenu.h"
#include "simplan.h"
#include "simworld.h"
#include "simhalt.h"
#include "player/simplay.h"
#include "simconst.h"
#include "macros.h"
#include "descriptor/ground_desc.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "boden/fundament.h"
#include "boden/wasser.h"
#include "boden/tunnelboden.h"
#include "boden/brueckenboden.h"
#include "boden/monorailboden.h"

#include "obj/gebaeude.h"

#include "dataobj/loadsave.h"
#include "dataobj/environment.h"

#include "gui/karte.h"


karte_ptr_t planquadrat_t::welt;


void swap(planquadrat_t& a, planquadrat_t& b)
{
	sim::swap(a.halt_list, b.halt_list);
	sim::swap(a.ground_size, b.ground_size);
	sim::swap(a.halt_list_count, b.halt_list_count);
	sim::swap(a.data, b.data);
	sim::swap(a.climate_data, b.climate_data);
}

// deletes also all grounds in this array!
planquadrat_t::~planquadrat_t()
{
	gebaeude_t *gb = NULL;
	if (!welt->is_destroying())
	{
		grund_t *gr = get_kartenboden();
		gb = gr ? gr->find<gebaeude_t>() : NULL;
	}

	if(ground_size==0) {
		// empty
	}
	else if(ground_size==1) {
		delete data.one;
	}
	else {
		while(ground_size>0) {
			ground_size --;
			delete data.some[ground_size];
			data.some[ground_size] = 0;
		}
		delete [] data.some;
	}
	delete [] halt_list;
	halt_list_count = 0;
	// to avoid access to this tile
	ground_size = 0;
	data.one = NULL;
	if (gb)
	{
		// If this is a building tile, make sure to delete the building's tile list.
		gb->set_building_tiles();
	}
}


grund_t *planquadrat_t::get_boden_von_obj(obj_t *obj) const
{
	if(ground_size==1) {
		if(data.one  &&  data.one->obj_ist_da(obj)) {
			return data.one;
		}
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]->obj_ist_da(obj)) {
				return data.some[i];
			}
		}
	}
	return NULL;
}


void planquadrat_t::boden_hinzufuegen(grund_t *bd)
{
	assert(!bd->ist_karten_boden());
	if(ground_size==0) {
		// completely empty
		data.one = bd;
		ground_size = 1;
		reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
		return;
	}
	else if(ground_size==1) {
		// needs to convert to array
//	assert(data.one->get_hoehe()!=bd->get_hoehe());
		if(data.one->get_hoehe()==bd->get_hoehe()) {
DBG_MESSAGE("planquadrat_t::boden_hinzufuegen()","addition ground %s at (%i,%i,%i) will be ignored!",bd->get_name(),bd->get_pos().x,bd->get_pos().y,bd->get_pos().z);
			return;
		}
		grund_t **tmp = new grund_t *[2];
		tmp[0] = data.one;
		tmp[1] = bd;
		data.some = tmp;
		ground_size = 2;
		reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
		return;
	}
	else {
		// insert into array
		uint8 i;
		for(i=1;  i<ground_size;  i++) {
			if(data.some[i]->get_hoehe()>=bd->get_hoehe()) {
				break;
			}
		}
		if(i<ground_size  &&  data.some[i]->get_hoehe()==bd->get_hoehe()) {
DBG_MESSAGE("planquadrat_t::boden_hinzufuegen()","addition ground %s at (%i,%i,%i) will be ignored!",bd->get_name(),bd->get_pos().x,bd->get_pos().y,bd->get_pos().z);
			return;
		}
		bd->set_kartenboden(false);
		// extend array if needed
		grund_t **tmp = new grund_t *[ground_size+1];
		for( uint8 j=0;  j<i;  j++  ) {
			tmp[j] = data.some[j];
		}
		tmp[i] = bd;
		while(i<ground_size) {
			tmp[i+1] = data.some[i];
			i++;
		}
		ground_size ++;
		delete [] data.some;
		data.some = tmp;
		reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
	}
}


bool planquadrat_t::boden_entfernen(grund_t *bd)
{
	assert(!bd->ist_karten_boden()  &&  ground_size>0);
	if(ground_size==1) {
		ground_size = 0;
		data.one = NULL;
		return true;
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]==bd) {
				// found
				while(i<ground_size-1) {
					data.some[i] = data.some[i+1];
					i++;
				}
				ground_size --;
				// below 2?
				if(ground_size==1) {
					grund_t *tmp=data.some[0];
					delete [] data.some;
					data.one = tmp;
				}
				return true;
			}
		}
	}
	assert(0);
	return false;
}


void planquadrat_t::kartenboden_setzen(grund_t *bd)
{
	assert(bd);
	grund_t *tmp = get_kartenboden();
	if(tmp) {
		boden_ersetzen(tmp,bd);
	}
	else {
		data.one = bd;
		ground_size = 1;
		bd->set_kartenboden(true);
	}
	bd->calc_image();
	reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
}


/**
 * replaces the map solid ground (or water) and deletes the old one
 * @author Hansjörg Malthaner
 */
void planquadrat_t::boden_ersetzen(grund_t *alt, grund_t *neu)
{
	assert(alt!=NULL  &&  neu!=NULL  &&  !alt->is_halt()  );

	if(ground_size<=1) {
		assert(data.one==alt  ||  ground_size==0);
		data.one = neu;
		ground_size = 1;
		neu->set_kartenboden(true);
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]==alt) {
				data.some[i] = neu;
				neu->set_kartenboden(i==0);
				break;
			}
		}
	}
	// transfer text and delete
	if(alt) {
		if(alt->get_flag(grund_t::has_text)) {
			neu->set_flag(grund_t::has_text);
			alt->clear_flag(grund_t::has_text);
		}
		// transfer all objects
		while(  alt->get_top()>0  ) {
			neu->obj_add( alt->obj_remove_top() );
		}
		delete alt;
	}
}


void planquadrat_t::rdwr(loadsave_t *file, koord pos )
{
	xml_tag_t p( file, "planquadrat_t" );

	if(file->is_saving()) {
		if(ground_size==1) {
			file->wr_obj_id(data.one->get_typ());
			data.one->rdwr(file);
		}
		else {
			for(int i=0; i<ground_size; i++) {
				file->wr_obj_id(data.some[i]->get_typ());
				data.some[i]->rdwr(file);
			}
		}
		file->wr_obj_id(-1);
	}
	else {
		grund_t *gr;
		sint8 hgt = welt->get_groundwater();
		//DBG_DEBUG("planquadrat_t::rdwr()","Reading boden");
		do {
			short gtyp = file->rd_obj_id();

			switch(gtyp) {
				case -1: gr = NULL; break;
				case grund_t::boden:	    gr = new boden_t(file, pos);                 break;
				case grund_t::wasser:	    gr = new wasser_t(file, pos);                break;
				case grund_t::fundament:	    gr = new fundament_t(file, pos);	     break;
				case grund_t::tunnelboden:	    gr = new tunnelboden_t(file, pos);       break;
				case grund_t::brueckenboden:    gr = new brueckenboden_t(file, pos);     break;
				case grund_t::monorailboden:	    gr = new monorailboden_t(file, pos); break;
				default:
					gr = 0; // Hajo: keep compiler happy, fatal() never returns
					dbg->fatal("planquadrat_t::rdwr()","Error while loading game: Unknown ground type '%d'",gtyp);
			}
			// check if we have a matching building here, otherwise set to nothing
			if (gr  &&  gtyp == grund_t::fundament  &&  gr->find<gebaeude_t>() == NULL) {
				koord3d pos = gr->get_pos();
				// show normal ground here
				grund_t *neu = new boden_t(pos, 0);
				if(gr->get_flag(grund_t::has_text)) {
					neu->set_flag(grund_t::has_text);
					gr->clear_flag(grund_t::has_text);
				}
				// transfer all objects
				while(  gr->get_top()>0  ) {
					neu->obj_add( gr->obj_remove_top() );
				}
				delete gr;
				gr = neu;
//DBG_MESSAGE("planquadrat_t::rwdr", "unknown building (or prepare for factory) at %d,%d replaced by normal ground!", pos.x,pos.y);
			}
			// we should also check for ground below factories
			if(gr) {
				if(ground_size==0) {
					data.one = gr;
					ground_size = 1;
					gr->set_kartenboden(true);
					hgt = welt->lookup_hgt(pos);
				}
				else {
					// other ground must not reset the height
					boden_hinzufuegen(gr);
					welt->set_grid_hgt( pos, hgt );
				}
			}
		} while(gr != 0);
	}

	if (file->get_extended_version() >= 13 || file->get_extended_revision() >= 21)
	{
		// Cache the halt_list as this saves a huge amount of time re-calculating this on loading.

		if (file->is_saving())
		{
			file->rdwr_byte(halt_list_count);

			if (halt_list_count > 0)
			{
				for (uint8 i = 0; i < halt_list_count; i++)
				{
					nearby_halt_t nearby_halt = halt_list[i];

					file->rdwr_byte(nearby_halt.distance);
					uint16 halt_id = nearby_halt.halt.get_id();
					file->rdwr_short(halt_id);
				}
			}
		}

		else // Loading
		{
			uint8 halt_list_count_from_file;

			file->rdwr_byte(halt_list_count_from_file);

			for (uint8 i = 0; i < halt_list_count_from_file; i++)
			{
				uint8 distance;
				file->rdwr_byte(distance);

				uint16 halt_id;
				file->rdwr_short(halt_id);
				halthandle_t halt;
				halt.set_id(halt_id);

				halt_list_insert_at(halt, i, distance);
			}
		}

	}
}


void planquadrat_t::check_season_snowline(const bool season_change, const bool snowline_change)
{
	if(  ground_size == 1  ) {
		data.one->check_season_snowline( season_change, snowline_change );
	}
	else if(  ground_size > 1  ) {
		for(  uint8 i = 0;  i < ground_size;  i++  ) {
			data.some[i]->check_season_snowline( season_change, snowline_change );
		}
	}
}



void planquadrat_t::correct_water()
{
	grund_t *gr = get_kartenboden();
	slope_t::type slope = gr->get_grund_hang();
	sint8 max_height = gr->get_hoehe() + slope_t::max_diff( slope );
	koord k = gr->get_pos().get_2d();
	sint8 water_hgt = welt->get_water_hgt(k);
	if(  gr  &&  gr->get_typ() != grund_t::wasser  &&  max_height <= water_hgt  ) {
		// below water but ground => convert
		kartenboden_setzen( new wasser_t(koord3d( k, water_hgt ) ) );
	}
	else if(  gr  &&  gr->get_typ() == grund_t::wasser  &&  max_height > water_hgt  ) {
		// water above ground => to ground
		kartenboden_setzen( new boden_t(gr->get_pos(), gr->get_disp_slope() ) );
	}
	else if(  gr  &&  gr->get_typ() == grund_t::wasser  &&  gr->get_hoehe() != water_hgt  ) {
		// water at wrong height
		gr->set_hoehe( water_hgt );
	}

	gr = get_kartenboden();
	if(  gr  &&  gr->get_typ() != grund_t::wasser  &&  gr->get_disp_height() < water_hgt  &&  welt->max_hgt(k) > water_hgt  ) {
		sint8 disp_hneu = water_hgt;
		sint8 disp_hn_sw = max( gr->get_hoehe() + corner_sw(slope), water_hgt );
		sint8 disp_hn_se = max( gr->get_hoehe() + corner_se(slope), water_hgt );
		sint8 disp_hn_ne = max( gr->get_hoehe() + corner_ne(slope), water_hgt );
		sint8 disp_hn_nw = max( gr->get_hoehe() + corner_nw(slope), water_hgt );
		const slope_t::type sneu = encode_corners(disp_hn_sw - disp_hneu, disp_hn_se - disp_hneu, disp_hn_ne - disp_hneu, disp_hn_nw - disp_hneu);
		gr->set_hoehe( disp_hneu );
		gr->set_grund_hang( (slope_t::type)sneu );
	}
}


void planquadrat_t::abgesenkt()
{
	grund_t *gr = get_kartenboden();
	if(gr) {
		const uint8 slope = gr->get_grund_hang();

		gr->obj_loesche_alle(NULL);
		sint8 max_hgt = gr->get_hoehe() + (slope ? 1 : 0);		// only matters that not flat

		koord k(gr->get_pos().get_2d());
		if(  max_hgt <= welt->get_water_hgt( k )  &&  gr->get_typ() != grund_t::wasser  ) {
			gr = new wasser_t(gr->get_pos());
			kartenboden_setzen( gr );
			// recalc water ribis of neighbors
			for(int r=0; r<4; r++) {
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::nsew[r]);
				if (gr2  &&  gr2->is_water()) {
					gr2->calc_image();
				}
			}
		}
		else {
			reliefkarte_t::get_karte()->calc_map_pixel(k);
		}
		gr->set_grund_hang( slope );
	}
}


void planquadrat_t::angehoben()
{
	grund_t *gr = get_kartenboden();
	if(gr) {
		const uint8 slope = gr->get_grund_hang();

		gr->obj_loesche_alle(NULL);
		sint8 max_hgt = gr->get_hoehe() + (slope ? 1 : 0);		// only matters that not flat

		koord k(gr->get_pos().get_2d());
		if(  max_hgt > welt->get_water_hgt( k )  &&  gr->get_typ() == grund_t::wasser  ) {
			gr = new boden_t(gr->get_pos(), slope );
			kartenboden_setzen( gr );
			// recalc water ribis
			for(int r=0; r<4; r++) {
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::nsew[r]);
				if(  gr2  &&  gr2->is_water()  ) {
					gr2->calc_image();
				}
			}
		}
		else if(  slope == 0  &&  gr->get_hoehe() == welt->get_water_hgt(k)  &&  gr->get_typ() == grund_t::wasser  ) {
			// water at zero level => make it land
			gr = new boden_t(gr->get_pos(), slope );
			kartenboden_setzen( gr );
			// recalc water ribis
			for(int r=0; r<4; r++) {
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::nsew[r]);
				if(  gr2  &&  gr2->is_water()  ) {
					gr2->calc_image();
				}
			}
		}
		else {
			reliefkarte_t::get_karte()->calc_map_pixel(k);
		}
	}
}


void planquadrat_t::display_obj(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, bool is_global, const sint8 hmin, const sint8 hmax  CLIP_NUM_DEF) const
{
	grund_t *gr0 = get_kartenboden();
	if(  gr0->get_flag( grund_t::dirty )  ) {
		gr0->set_all_obj_dirty(); // prevent artifacts with smart hide cursor
	}
	const sint8 h0 = gr0->get_disp_height();
	uint8 i = 1;
	// underground
	if(  hmin < h0  ) {
		for(  ;  i < ground_size;  i++  ) {
			const grund_t* gr = data.some[i];
			const sint8 h = gr->get_hoehe();
			const slope_t::type slope = gr->get_grund_hang();
			const sint8 htop = h + max(max(corner_sw(slope), corner_se(slope)),max(corner_ne(slope), corner_nw(slope)));
			// above ground
			if(  h > h0  ) {
				break;
			}
			// not too low?
			if(  htop >= hmin  ) {
				const sint16 yypos = ypos - tile_raster_scale_y( (h - h0) * TILE_HEIGHT_STEP, raster_tile_width );
				gr->display_boden( xpos, yypos, raster_tile_width  CLIP_NUM_PAR );
				gr->display_obj_all( xpos, yypos, raster_tile_width, is_global  CLIP_NUM_PAR );
			}
		}
	}
	//const bool kartenboden_dirty = gr->get_flag(grund_t::dirty);
	if(  gr0->get_flag( grund_t::draw_as_obj )  ||  !gr0->is_karten_boden_visible()  ) {
		gr0->display_boden( xpos, ypos, raster_tile_width  CLIP_NUM_PAR );
	}

	if(  env_t::simple_drawing  ) {
		// ignore trees going though bridges
		gr0->display_obj_all_quick_and_dirty( xpos, ypos, raster_tile_width, is_global  CLIP_NUM_PAR );
	}
	else {
		// clip everything at the next tile above
		if(  i < ground_size  ) {

			clip_dimension p_cr = display_get_clip_wh(CLIP_NUM_VAR);

			for(  uint8 j = i;  j < ground_size;  j++  ) {
				const sint8 h = data.some[j]->get_hoehe();
				const sint8 htop = h + slope_t::max_diff(data.some[j]->get_grund_hang());
				// too high?
				if(  h > hmax  ) {
					break;
				}
				// not too low?
				if(  htop >= hmin  ) {
					// something on top: clip horizontally to prevent trees etc shining trough bridges
					const sint16 yh = ypos - tile_raster_scale_y( (h - h0) * TILE_HEIGHT_STEP, raster_tile_width ) + ((3 * raster_tile_width) >> 2);
					if(  yh >= p_cr.y  ) {
						display_push_clip_wh(p_cr.x, yh, p_cr.w, p_cr.h + p_cr.y - yh  CLIP_NUM_PAR);
					}
					break;
				}
			}
			gr0->display_obj_all( xpos, ypos, raster_tile_width, is_global  CLIP_NUM_PAR );
			display_pop_clip_wh(CLIP_NUM_VAR);
		}
		else {
			gr0->display_obj_all( xpos, ypos, raster_tile_width, is_global  CLIP_NUM_PAR );
		}
	}
	// above ground
	for(  ;  i < ground_size;  i++  ) {
		const grund_t* gr = data.some[i];
		const sint8 h = gr->get_hoehe();
		const slope_t::type slope = gr->get_grund_hang();
		const sint8 htop = h + max(max(corner_sw(slope), corner_se(slope)),max(corner_ne(slope), corner_nw(slope)));
		// too high?
		if(  h > hmax  ) {
			break;
		}
		// not too low?
		if(  htop >= hmin  ) {
			const sint16 yypos = ypos - tile_raster_scale_y( (h - h0) * TILE_HEIGHT_STEP, raster_tile_width );
			gr->display_boden( xpos, yypos, raster_tile_width  CLIP_NUM_PAR );
			gr->display_obj_all( xpos, yypos, raster_tile_width, is_global  CLIP_NUM_PAR );
		}
	}
}


image_id overlay_img(grund_t *gr)
{
	// only transparent outline
	image_id img;
	if(  gr->get_typ()==grund_t::wasser  ) {
		// water is always flat and does not return proper image_id
		img = ground_desc_t::outside->get_image(0);
	}
	else {
		img = gr->get_image();
		if(  img==IMG_EMPTY  ) {
			// foundations or underground mode
			img = ground_desc_t::get_ground_tile( gr );
		}
	}
	return img;
}


void planquadrat_t::display_overlay(const sint16 xpos, const sint16 ypos) const
{
	grund_t *gr=get_kartenboden();

	// building transformers - show outlines of factories

/*	alternative method of finding selected tool - may be more useful in future but use simpler method for now
	tool_t *tool = welt->get_tool(welt->get_active_player_nr());
	int tool_id = tool->get_id();

	if(tool_id==(TOOL_TRANSFORMER|GENERAL_TOOL)....	*/

	if ((grund_t::underground_mode == grund_t::ugm_all
		 || (grund_t::underground_mode == grund_t::ugm_level  &&  gr->get_hoehe() == grund_t::underground_level + welt->get_settings().get_way_height_clearance()))
		&&  gr->get_typ()==grund_t::fundament
		&&  tool_t::general_tool[TOOL_TRANSFORMER]->is_selected()) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb) {
			fabrik_t* fab=gb->get_fabrik();
			if(fab) {
				PLAYER_COLOR_VAL status = COL_RED;
				if(fab->get_desc()->is_electricity_producer()) {
					status = COL_LIGHT_BLUE;
					if(fab->is_transformer_connected()) {
						status = COL_LIGHT_TURQUOISE;
					}
				}
				else {
					if(fab->is_transformer_connected()) {
						status = COL_ORANGE;
					}
					if(fab->get_prodfactor_electric()>0) {
						status = COL_GREEN;
					}
				}
				display_img_blend( overlay_img(gr), xpos, ypos, status | OUTLINE_FLAG | TRANSPARENT50_FLAG, 0, true);
			}
		}
	}

	// display station owner boxes
	if(env_t::station_coverage_show  &&  halt_list_count>0) {

		if(env_t::use_transparency_station_coverage) {

			// only transparent outline
			image_id img = overlay_img(gr);

			for(int halt_count = 0; halt_count < halt_list_count; halt_count++) {
				const PLAYER_COLOR_VAL transparent = PLAYER_FLAG | OUTLINE_FLAG | (halt_list[halt_count].halt->get_owner()->get_player_color1() + 4);
				display_img_blend( img, xpos, ypos, transparent | TRANSPARENT25_FLAG, 0, 0);
			}
#ifdef PLOT_PLAYER_OUTLINE_COLOURS
			// Earlier code comments considered this was too slow for display, but this does not seem the case as of April 2013.
			// However, this does not work properly: colours are often displayed incorrectly.
			// The intention of this seems to be to overlay and blend different players' colours in the display of station coverage.
			// plot player outline colours - we always plot in order of players so that the order of the stations in halt_list
			// doesn't affect the colour displayed [since blend(col1,blend(col2,screen)) != blend(col2,blend(col1,screen))]
			for(int player_count = 0; player_count<MAX_PLAYER_COUNT; player_count++)
			{
				player_t *display_player = welt->get_player(player_count);
				if(display_player)
				{
					const PLAYER_COLOR_VAL transparent = PLAYER_FLAG | OUTLINE_FLAG | (display_player->get_player_color1() * 4 + 4);
					for(int halt_count = 0; halt_count < halt_list_count; halt_count++)
					{
						if(halt_list[halt_count].halt->get_owner() == display_player)
						{
							display_img_blend( img, xpos, ypos, transparent | TRANSPARENT25_FLAG, 0, 0);
						}
					}
				}
			}
#endif
		}
		else {
			const sint16 raster_tile_width = get_tile_raster_width();
			// opaque boxes (
			const sint16 r=raster_tile_width/8;
			const sint16 x=xpos+raster_tile_width/2-r;
			const sint16 y=ypos+(raster_tile_width*3)/4-r - (gr->get_grund_hang()? tile_raster_scale_y(8,raster_tile_width): 0);
			const bool kartenboden_dirty = gr->get_flag(grund_t::dirty);
			const sint16 off = (raster_tile_width>>5);
			// suitable start search
			for (size_t h = halt_list_count; h-- != 0;) {
				display_fillbox_wh_clip(x - h * off, y + h * off, r, r, PLAYER_FLAG | (halt_list[h].halt->get_owner()->get_player_color1() + 4), kartenboden_dirty);
			}
		}
	}

	if (welt->get_active_player()->get_selected_signalbox() != NULL)
	{
		gebaeude_t* gb = (gebaeude_t*)welt->get_active_player()->get_selected_signalbox();
		if (gb)
		{
			uint32 const radius = gb->get_tile()->get_desc()->get_radius();
			uint16 const cov = radius / welt->get_settings().get_meters_per_tile();
			if (shortest_distance(gb->get_pos().get_2d(), gr->get_pos().get_2d()) <= cov)
			{
				// Mark a 5x5 cross at center of circle
				if ((gr->get_pos().x == gb->get_pos().x && (gr->get_pos().y >= gb->get_pos().y - 2 && gr->get_pos().y <= gb->get_pos().y + 2)) || (gr->get_pos().y == gb->get_pos().y && (gr->get_pos().x >= gb->get_pos().x - 2 && gr->get_pos().x <= gb->get_pos().x + 2)))
				{
					welt->mark_area(gr->get_pos(), koord(1, 1), env_t::signalbox_coverage_show);
				}
				// Mark the circle
				if (shortest_distance(gb->get_pos().get_2d(), gr->get_pos().get_2d()) >= cov)
				{
					welt->mark_area(gr->get_pos(), koord(1, 1), env_t::signalbox_coverage_show);
				}
			}
		}
	}



	gr->display_overlay(xpos, ypos);
	if (ground_size > 1) {
		const sint16 raster_tile_width = get_tile_raster_width();
		const sint8 h0 = gr->get_disp_height();
		for(  uint8 i = 1;  i < ground_size;  i++  ) {
			grund_t* gr = data.some[i];
			const sint8 h = gr->get_disp_height();
			const sint16 yypos = ypos - tile_raster_scale_y((h - h0) * TILE_HEIGHT_STEP, raster_tile_width);
			gr->display_overlay( xpos, yypos );
		}
	}
}

/**
 * Finds halt belonging to a player
 * @param player owner of the halts we are interested in.
 */
// (somewhat random, although ground level will be preferred -- don't call this with NULL)
halthandle_t planquadrat_t::get_halt(player_t *player) const
{
	for(  uint8 i=0;  i < get_boden_count();  i++  ) {
		halthandle_t my_halt = get_boden_bei(i)->get_halt();
		if(  my_halt.is_bound()  &&  (player == NULL  ||  player == my_halt->get_owner())  ) {
			return my_halt;
		}
	}
	return halthandle_t();
}



// these functions are private helper functions for halt_list
void planquadrat_t::halt_list_remove( halthandle_t halt )
{
	for( uint8 i=0;  i<halt_list_count;  i++ ) {
		if(halt_list[i].halt == halt) {
			for( uint8 j=i+1;  j<halt_list_count;  j++  ) {
				halt_list[j-1] = halt_list[j];
			}
			halt_list_count--;
			break;
		}
	}
}


void planquadrat_t::halt_list_insert_at(halthandle_t halt, uint8 pos, uint8 distance)
{
	// extend list?
	if((halt_list_count%4)==0) {
		nearby_halt_t *tmp = new nearby_halt_t[halt_list_count+4];
		if(halt_list!=NULL) {
			// now insert
			for( uint8 i=0;  i<halt_list_count;  i++ ) {
				tmp[i] = halt_list[i];
			}
			delete [] halt_list;
		}
		halt_list = tmp;
	}
	// now insert
	for( uint8 i=halt_list_count;  i>pos;  i-- ) {
		halt_list[i] = halt_list[i-1];
	}
	halt_list[pos].halt = halt;
	halt_list[pos].distance = distance;
	halt_list_count ++;
}


/* The following functions takes at least 9 bytes of memory per tile but speed up passenger generation *
 * @author prissi
 * Modified: jamespetts, May 2013
 */
void planquadrat_t::add_to_haltlist(halthandle_t halt)
{
	if(halt.is_bound())
	{
		// Quick and dirty way to our 2d co-ordinates
		const koord pos = get_kartenboden()->get_pos().get_2d();
		const koord halt_next_pos = halt->get_next_pos(pos, true);
		// Must be koord_distance not shortest_distance as the coverage radii are square, not circular
		const uint8 distance = (uint8)koord_distance(halt_next_pos, pos);
		if(halt_list_count > 0)
		{
			// Since only the first one gets all, we want the closest halt one to be first
			halt_list_remove(halt);

			for(unsigned insert_pos = 0; insert_pos < halt_list_count; insert_pos++)
			{
				if(halt_list[insert_pos].halt.is_bound() && koord_distance(halt_list[insert_pos].halt->get_next_pos(pos), pos) > distance)
				{
					halt_list_insert_at(halt, insert_pos, distance);
					return;
				}
			}
			// not found
		}
		// first just or just append to the end ...
		halt_list_insert_at(halt, halt_list_count, distance);
	}
}


/**
 * removes the halt from a ground
 * however this function check, whether there is really no other part still reachable
 * @author prissi, neroden
 */
void planquadrat_t::remove_from_haltlist(halthandle_t halt)
{
	halt_list_remove(halt);

	// We might still be connected (to a different tile on the halt, in which case reconnect.
	// NOTE: Coverage may be changed when the handling item is changed. e.g.) pax & goods -> only goods

	// quick and dirty way to our 2d koodinates ...
	const koord pos = get_kartenboden()->get_pos().get_2d();
	int const cov = welt->get_settings().get_station_coverage();
	halt->recalc_station_type();
	uint16 new_cov = 0;
	if (halt->get_pax_enabled() || halt->get_mail_enabled()) {
		new_cov = welt->get_settings().get_station_coverage();
	}
	else if (halt->get_ware_enabled()) {
		new_cov = welt->get_settings().get_station_coverage_factories();
	}
	for (int y = -cov; y <= cov; y++) {
		for (int x = -cov; x <= cov; x++) {
			koord test_pos = pos+koord(x,y);
			const planquadrat_t *pl = welt->access(test_pos);
			if (pl) {
				for(  uint i = 0;  i < pl->get_boden_count();  i++  ) {
					if (  pl->get_boden_bei(i)->get_halt() == halt && (abs(x)<=new_cov &&  abs(y)<=new_cov)) {
						// still connected
						// Reset distance computation
						add_to_haltlist(halt);
					}
				}
			}
		}
	}
}


/**
 * true, if this halt is reachable from here
 * @author prissi
 */
uint8 planquadrat_t::get_connected(halthandle_t halt) const
{
	for(uint8 i = 0; i < halt_list_count; i++)
	{
		if(halt_list[i].halt == halt)
		{
			return halt_list[i].distance;
		}
	}
	return 255;
}

void planquadrat_t::update_underground() const
{
	get_kartenboden()->check_update_underground();
	// update tunnel tiles
	for(unsigned int i=1; i<get_boden_count(); i++) {
		grund_t *const gr = get_boden_bei(i);
		if (gr->ist_tunnel()) {
			gr->check_update_underground();
		}
	}
}
