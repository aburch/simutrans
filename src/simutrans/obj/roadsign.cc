/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "../simunits.h"
#include "../simdebug.h"
#include "simobj.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../tool/simtool.h"
#include "../world/simworld.h"

#include "../descriptor/roadsign_desc.h"
#include "../descriptor/skin_desc.h"

#include "../ground/grund.h"
#include "../obj/way/strasse.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../gui/trafficlight_info.h"
#include "../gui/privatesign_info.h"
#include "../gui/tool_selector.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/cbuffer.h"
#include "../utils/simstring.h"

#include "roadsign.h"


const roadsign_desc_t *roadsign_t::default_signal=NULL;

stringhashtable_tpl<const roadsign_desc_t *> roadsign_t::table;


roadsign_t::roadsign_t(loadsave_t *file) : obj_t ()
{
	image = foreground_image = IMG_EMPTY;
	preview = false;
	rdwr(file);
	if(desc) {
		/* if more than one state, we will switch direction and phase for traffic lights
		 * however also gate signs need indications
		 */
		automatic = (desc->get_count()>4  &&  desc->get_wtyp()==road_wt)  ||  (desc->get_count()>2  &&  desc->is_private_way());
	}
	else {
		automatic = false;
	}
	// some sve had rather strange entries in state
	if(  !automatic  ||  desc==NULL  ) {
		state = 0;
	}
	// only traffic light need switches
	if(  automatic  ) {
		welt->sync.add(this);
	}
}


roadsign_t::roadsign_t(player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_desc_t *desc, bool preview) : obj_t(pos)
{
	this->desc = desc;
	this->dir = dir;
	this->preview = preview;
	image = foreground_image = IMG_EMPTY;
	state = 0;
	ticks_ns = ticks_ow = 16;
	ticks_offset = 0;
	ticks_yellow_ns = ticks_yellow_ow = 2;
	set_owner( player );
	if(  desc->is_private_way()  ) {
		// init ownership of private ways
		ticks_ns = ticks_ow = 0;
		if(  player->get_player_nr() >= 8  ) {
			ticks_ow = 1 << (player->get_player_nr()-8);
		}
		else {
			ticks_ns = 1 << player->get_player_nr();
		}
	}
	/* if more than one state, we will switch direction and phase for traffic lights
	 * however also gate signs need indications
	 */
	automatic = (desc->get_count()>4  &&  desc->get_wtyp()==road_wt)  ||  (desc->get_count()>2  &&  desc->is_private_way());
	// only traffic light need switches
	if(  automatic  ) {
		welt->sync.add(this);
	}
}


roadsign_t::~roadsign_t()
{
	if(  desc  ) {
		const grund_t *gr = welt->lookup(get_pos());
		if(gr) {
			weg_t *weg = gr->get_weg(desc->get_wtyp()!=tram_wt ? desc->get_wtyp() : track_wt);
			if(weg) {
				if (!preview) {
					if (desc->is_single_way()  ||  desc->is_signal_type()) {
						// signal removed, remove direction mask
						weg->set_ribi_maske(ribi_t::none);
					}
					weg->clear_sign_flag();
				}
			}
			else {
				dbg->error("roadsign_t::~roadsign_t()","roadsign_t %p was deleted but ground has no way of type %d!", this, desc->get_wtyp() );
			}
		}
	}
	if(automatic) {
		welt->sync.remove(this);
	}
}


void roadsign_t::set_dir(ribi_t::ribi dir)
{
	ribi_t::ribi olddir = this->dir;

	this->dir = dir;
	if (!preview) {
		weg_t *weg = welt->lookup(get_pos())->get_weg(desc->get_wtyp()!=tram_wt ? desc->get_wtyp() : track_wt);
		if(  desc->get_wtyp()!=track_wt  &&  desc->get_wtyp()!=monorail_wt  &&  desc->get_wtyp()!=maglev_wt  &&  desc->get_wtyp()!=narrowgauge_wt  ) {
			weg->count_sign();
		}
		if(desc->is_single_way()  ||  desc->is_signal_type()) {
			// set mask, if it is a single way ...
			weg->count_sign();
			weg->set_ribi_maske(calc_mask());
DBG_MESSAGE("roadsign_t::set_dir()","ribi %i",dir);
		}
	}

	// force redraw
	mark_image_dirty(get_image(),0);
	// some more magic to get left side images right ...
	sint8 old_x = get_xoff();
	set_xoff( after_xoffset );
	mark_image_dirty(foreground_image,after_yoffset-get_yoff());
	set_xoff( old_x );

	image = IMG_EMPTY;
	foreground_image = IMG_EMPTY;
	calc_image();

	if (preview)
		this->dir = olddir;
}


void roadsign_t::show_info()
{
	if(  desc->is_private_way()  ) {
		create_win(new privatesign_info_t(this), w_info, (ptrdiff_t)this );
	}
	else if(  automatic  ) {
		create_win(new trafficlight_info_t(this), w_info, (ptrdiff_t)this );
	}
	else {
		obj_t::show_info();
	}
}


void roadsign_t::info(cbuffer_t & buf) const
{
	obj_t::info( buf );

	if(  !desc->is_private_way()  ) {
		buf.append(translator::translate("Roadsign"));
		buf.append("\n");
		if(desc->is_single_way()) {
			buf.append(translator::translate("\nsingle way"));
		}
		if(desc->get_min_speed()!=0) {
			buf.printf("%s%d", translator::translate("\nminimum speed:"), speed_to_kmh(desc->get_min_speed()));
		}
		buf.printf("%s%u\n", translator::translate("\ndirection:"), dir);

		if(  automatic  ) {
			buf.append(translator::translate("\nSet phases:"));
		}

		if (!automatic) {
			if (char const* const maker = desc->get_copyright()) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
			}
			// extra treatment of trafficlights & private signs, author will be shown by those windows themselves
		}
	}
}


// could be still better aligned for drive_left settings ...
void roadsign_t::calc_image()
{
	set_flag(obj_t::dirty);

	// vertical offset of the signal positions
	const grund_t *gr=welt->lookup(get_pos());
	if(gr==NULL) {
		return;
	}

	after_xoffset = 0;
	after_yoffset = 0;
	sint8 xoff = 0, yoff = 0;
	// left offsets defined, and image-on-the-left activated
	const bool left_offsets = desc->get_offset_left()  &&
	    (     (desc->get_wtyp()==road_wt  &&  welt->get_settings().is_drive_left()  )
	      ||  (desc->get_wtyp()!=air_wt  &&  desc->get_wtyp()!=road_wt  &&  welt->get_settings().is_signals_left())
	    );

	const slope_t::type full_hang = gr->get_weg_hang();
	const sint8 hang_diff = slope_t::max_diff(full_hang);
	const ribi_t::ribi hang_dir = ribi_t::backward( ribi_type(full_hang) );

	// private way have also closed/open states
	if(  desc->is_private_way()  ) {
		uint8 image = 1-(dir&1);
		if(  (1<<welt->get_active_player_nr()) & get_player_mask()  ) {
			// gate open
			image += 2;
		}
		set_image( desc->get_image_id(image) );
		set_yoff( 0 );
		if(  hang_diff  ) {
				set_yoff( -(TILE_HEIGHT_STEP*hang_diff)/2 );
		}
		else {
			set_yoff( -gr->get_weg_yoff() );
		}
		foreground_image = IMG_EMPTY;
		return;
	}

	if(  hang_diff == 0  ) {
		yoff = -gr->get_weg_yoff();
		after_yoffset = yoff;
	}
	else {
		// since the places were switched
		const ribi_t::ribi test_hang = left_offsets ? ribi_t::backward(hang_dir) : hang_dir;
		if(test_hang==ribi_t::east ||  test_hang==ribi_t::north) {
			yoff = -TILE_HEIGHT_STEP*hang_diff;
			after_yoffset = 0;
		}
		else {
			yoff = 0;
			after_yoffset = -TILE_HEIGHT_STEP*hang_diff;
		}
	}

	image_id tmp_image=IMG_EMPTY;
	if(!automatic) {
		assert( state==0 );

		foreground_image = IMG_EMPTY;
		ribi_t::ribi temp_dir = dir;

		if(  gr->get_typ()==grund_t::tunnelboden  &&  gr->ist_karten_boden()  &&
			(grund_t::underground_mode==grund_t::ugm_none  ||  (grund_t::underground_mode==grund_t::ugm_level  &&  gr->get_hoehe()<grund_t::underground_level))   ) {
			// entering tunnel here: hide the image further in if not undergroud/sliced
			const ribi_t::ribi tunnel_hang_dir = ribi_t::backward( ribi_type(gr->get_grund_hang()) );
			if(  tunnel_hang_dir==ribi_t::east ||  tunnel_hang_dir==ribi_t::north  ) {
				temp_dir &= ~ribi_t::southwest;
			}
			else {
				temp_dir &= ~ribi_t::northeast;
			}
		}

		// signs for left side need other offsets and other front/back order
		if(  left_offsets  ) {
			const sint16 XOFF = 2*desc->get_offset_left();
			const sint16 YOFF = desc->get_offset_left();

			if(temp_dir&ribi_t::east) {
				tmp_image = desc->get_image_id(3);
				xoff += XOFF;
				yoff += -YOFF;
			}

			if(temp_dir&ribi_t::north) {
				if(tmp_image!=IMG_EMPTY) {
					foreground_image = desc->get_image_id(0);
					after_xoffset += -XOFF;
					after_yoffset += -YOFF;
				}
				else {
					tmp_image = desc->get_image_id(0);
					xoff += -XOFF;
					yoff += -YOFF;
				}
			}

			if(temp_dir&ribi_t::west) {
				foreground_image = desc->get_image_id(2);
				after_xoffset += -XOFF;
				after_yoffset += YOFF;
			}

			if(temp_dir&ribi_t::south) {
				if(foreground_image!=IMG_EMPTY) {
					tmp_image = desc->get_image_id(1);
					xoff += XOFF;
					yoff += YOFF;
				}
				else {
					foreground_image = desc->get_image_id(1);
					after_xoffset += XOFF;
					after_yoffset += YOFF;
				}
			}
		}
		else {

			if(temp_dir&ribi_t::east) {
				foreground_image = desc->get_image_id(3);
			}

			if(temp_dir&ribi_t::north) {
				if(foreground_image!=IMG_EMPTY) {
					tmp_image = desc->get_image_id(0);
				}
				else {
					foreground_image = desc->get_image_id(0);
				}
			}

			if(temp_dir&ribi_t::west) {
				tmp_image = desc->get_image_id(2);
			}

			if(temp_dir&ribi_t::south) {
				if(tmp_image!=IMG_EMPTY) {
					foreground_image = desc->get_image_id(1);
				}
				else {
					tmp_image = desc->get_image_id(1);
				}
			}
		}

		// some signs on roads must not have a background (but then they have only two rotations)
		if(  desc->get_flags()&roadsign_desc_t::ONLY_BACKIMAGE  ) {
			if(foreground_image!=IMG_EMPTY) {
				tmp_image = foreground_image;
			}
			foreground_image = IMG_EMPTY;
		}
	}
	else {
		// traffic light
		weg_t *str=gr->get_weg(road_wt);
		if(str) {
			const uint8 weg_dir = str->get_ribi_unmasked();
			const uint8 direction = desc->get_count()>16 ? (state&2) + ((state+1) & 1) : (state+1) & 1;

			// other front/back images for left side ...
			if(  left_offsets  ) {
			const sint16 XOFF = 2*desc->get_offset_left();
			const sint16 YOFF = desc->get_offset_left();

				if(weg_dir&ribi_t::north) {
					if(weg_dir&ribi_t::east) {
						foreground_image = desc->get_image_id(6+direction*8);
						after_xoffset += 0;
						after_yoffset += 0;
					}
					else {
						foreground_image = desc->get_image_id(1+direction*8);
						after_xoffset += XOFF;
						after_yoffset += YOFF;
					}
				}
				else if(weg_dir&ribi_t::east) {
					foreground_image = desc->get_image_id(2+direction*8);
					after_xoffset += -XOFF;
					after_yoffset += YOFF;
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::south) {
						tmp_image = desc->get_image_id(7+direction*8);
						xoff += 0;
						yoff += 0;
					}
					else {
						tmp_image = desc->get_image_id(3+direction*8);
						xoff += XOFF;
						yoff += -YOFF;
					}
				}
				else if(weg_dir&ribi_t::south) {
					tmp_image = desc->get_image_id(0+direction*8);
					xoff += -XOFF;
					yoff += -YOFF;
				}
			}
			else {
				// drive right ...
				if(weg_dir&ribi_t::south) {
					if(weg_dir&ribi_t::east) {
						foreground_image = desc->get_image_id(4+direction*8);
					}
					else {
						foreground_image = desc->get_image_id(0+direction*8);
					}
				}
				else if(weg_dir&ribi_t::east) {
					foreground_image = desc->get_image_id(2+direction*8);
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::north) {
						tmp_image = desc->get_image_id(5+direction*8);
					}
					else {
						tmp_image = desc->get_image_id(3+direction*8);
					}
				}
				else if(weg_dir&ribi_t::north) {
					tmp_image = desc->get_image_id(1+direction*8);
				}
			}

		}
	}
	// set image and offsets
	set_image( tmp_image );
	set_xoff( xoff );
	set_yoff( yoff );

}


// only used for traffic light: change the current state
sync_result roadsign_t::sync_step(uint32 /*delta_t*/)
{
	if(  desc->is_private_way()  ) {
		uint8 image = 1-(dir&1);
		if(  (1<<welt->get_active_player_nr()) & get_player_mask()  ) {
			// gate open
			image += 2;
			// force redraw
			mark_image_dirty(get_image(),0);
		}
		set_image( desc->get_image_id(image) );
	}
	else {
		// Must not overflow if ticks_ns+ticks_ow+ticks_yellow_ns+ticks_yellow_ow=256
        uint32 ticks = ((welt->get_ticks()>>10)+ticks_offset) % ((uint32)ticks_ns+(uint32)ticks_ow+(uint32)ticks_yellow_ns+(uint32)ticks_yellow_ow);

		uint8 new_state = 0;
		//traffic light transition: e-w dir -> yellow e-w -> n-s dir -> yellow n-s -> ...
		if(  ticks < ticks_ow  ) {
		  new_state = 0;
		}
		else if(  ticks < ticks_ow+ticks_yellow_ow  ) {
		  new_state = 2;
		}
		else if(  ticks < (uint32)ticks_ow+ticks_yellow_ow+ticks_ns  ) {
		  new_state = 1;
		}
		else {
		  new_state = 3;
		}

		if(state!=new_state) {
			state = new_state;
			switch(new_state) {
			case 0:
			  dir = ribi_t::northsouth;
			  break;
			case 1:
			  dir = ribi_t::eastwest;
			  break;
			default: // yellow state
			  dir = ribi_t::none;
			  break;
			}
			calc_image();
		}
	}
	return SYNC_OK;
}


void roadsign_t::rotate90()
{
	// only meaningful for traffic lights
	obj_t::rotate90();
	if(automatic  &&  !desc->is_private_way()) {
	  state = (state&2/*whether yellow*/) + ((state+1)&1);
		if(  ticks_offset >= ticks_ns  ) {
			ticks_offset -= ticks_ns;
		}
		else {
			ticks_offset += ticks_ow;
		}
		uint8 temp = ticks_ns;
		ticks_ns = ticks_ow;
		ticks_ow = temp;
		temp = ticks_yellow_ns;
		ticks_yellow_ns = ticks_yellow_ow;
		ticks_yellow_ow = temp;

		trafficlight_info_t *const trafficlight_win = dynamic_cast<trafficlight_info_t *>( win_get_magic( (ptrdiff_t)this ) );
		if(  trafficlight_win  ) {
			trafficlight_win->update_data();
		}
	}
	dir = ribi_t::rotate90( dir );
}


// to correct offset on slopes
#ifdef MULTI_THREAD
void roadsign_t::display_after(int xpos, int ypos, const sint8 clip_num ) const
#else
void roadsign_t::display_after(int xpos, int ypos, bool ) const
#endif
{
	if(  foreground_image != IMG_EMPTY  ) {
		const int raster_width = get_current_tile_raster_width();
		xpos += tile_raster_scale_x( after_xoffset, raster_width );
		ypos += tile_raster_scale_y( after_yoffset, raster_width );
		// draw with owner
		if(  get_owner_nr() != PLAYER_UNOWNED  ) {
			if(  obj_t::show_owner  ) {
				display_blend( foreground_image, xpos, ypos, 0, color_idx_to_rgb(get_owner()->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, dirty  CLIP_NUM_PAR);
			}
			else {
				display_color( foreground_image, xpos, ypos, get_owner_nr(), true, get_flag(obj_t::dirty)  CLIP_NUM_PAR);
			}
		}
		else {
			display_normal( foreground_image, xpos, ypos, 0, true, get_flag(obj_t::dirty)  CLIP_NUM_PAR);
		}
	}
}



void roadsign_t::rdwr(loadsave_t *file)
{
	xml_tag_t r( file, "roadsign_t" );

	obj_t::rdwr(file);

	uint8 dummy=0;
	if(  file->is_version_less(102, 3)  ) {
		file->rdwr_byte(dummy);
		if(  file->is_loading()  ) {
			ticks_ns = ticks_ow = 16;
		}
	}
	else {
		file->rdwr_byte(ticks_ns);
		file->rdwr_byte(ticks_ow);
	}
	if(  file->is_version_atleast(110, 7)  ) {
		file->rdwr_byte(ticks_offset);
	}
	else {
		if(  file->is_loading()  ) {
			ticks_offset = 0;
		}
	}

	if( file->is_version_atleast(122,2) ) {
		file->rdwr_byte(ticks_yellow_ns);
		file->rdwr_byte(ticks_yellow_ow);
	}
	else if( file->is_loading() ){
		ticks_yellow_ns = ticks_yellow_ow = 2;
	}

	dummy = state;
	file->rdwr_byte(dummy);
	state = dummy;
	dummy = dir;
	file->rdwr_byte(dummy);
	dir = dummy;
	if(file->is_version_less(89, 0)) {
		dir = ribi_t::backward(dir);
	}

	if(file->is_saving()) {
		const char *s = desc->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		desc = roadsign_t::table.get(bname);
		if(desc==NULL) {
			desc = roadsign_t::table.get(translator::compatibility_name(bname));
			if(  desc==NULL  ) {
				dbg->warning("roadsign_t::rwdr", "description %s for roadsign/signal at %d,%d not found! (may be ignored)", bname, get_pos().x, get_pos().y);
				welt->add_missing_paks( bname, karte_t::MISSING_SIGN );
			}
			else {
				dbg->warning("roadsign_t::rwdr", "roadsign/signal %s at %d,%d replaced by %s", bname, get_pos().x, get_pos().y, desc->get_name() );
			}
		}
		// init ownership of private ways signs
		if(  file->is_version_less(110, 7)  &&  desc  &&  desc->is_private_way()  ) {
			ticks_ns = 0xFD;
			ticks_ow = 0xFF;
		}
	}
}


void roadsign_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, -desc->get_price(), get_pos().get_2d(), get_waytype());
}


void roadsign_t::finish_rd()
{
	grund_t *gr=welt->lookup(get_pos());
	if(  gr==NULL  ||  !gr->hat_weg(desc->get_wtyp()!=tram_wt ? desc->get_wtyp() : track_wt)  ) {
		dbg->error("roadsign_t::finish_rd","roadsing: way/ground missing at %i,%i => ignore", get_pos().x, get_pos().y );
	}
	else {
		// after loading restore directions
		set_dir(dir);
		gr->get_weg(desc->get_wtyp()!=tram_wt ? desc->get_wtyp() : track_wt)->count_sign();
	}
}


// to sort compare_roadsign_desc for always the same menu order
static bool compare_roadsign_desc(const roadsign_desc_t* a, const roadsign_desc_t* b)
{
	int diff = a->get_wtyp() - b->get_wtyp();
	if (diff == 0) {
		if(a->is_choose_sign()) {
			diff += 120;
		}
		if(b->is_choose_sign()) {
			diff -= 120;
		}
		diff += (int)(a->get_flags() & ~roadsign_desc_t::SIGN_SIGNAL) - (int)(b->get_flags()  & ~roadsign_desc_t::SIGN_SIGNAL);
	}
	if (diff == 0) {
		/* Some type: sort by name */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


/* static stuff from here on ... */
bool roadsign_t::successfully_loaded()
{
	if(table.empty()) {
		DBG_MESSAGE("roadsign_t", "No signs found - feature disabled");
	}
	return true;
}


bool roadsign_t::register_desc(roadsign_desc_t *desc)
{
	// avoid duplicates with same name
	if(const roadsign_desc_t *old_desc = table.remove(desc->get_name())) {
		dbg->doubled( "roadsign", desc->get_name() );
		tool_t::general_tool.remove( old_desc->get_builder() );
		delete old_desc->get_builder();
		delete old_desc;
	}

	if(  desc->get_cursor()->get_image_id(1)!=IMG_EMPTY  ) {
		// add the tool
		tool_build_roadsign_t *tool = new tool_build_roadsign_t();
		tool->set_icon( desc->get_cursor()->get_image_id(1) );
		tool->cursor = desc->get_cursor()->get_image_id(0);
		tool->set_default_param(desc->get_name());
		tool_t::general_tool.append( tool );
		desc->set_builder( tool );
	}
	else {
		desc->set_builder( NULL );
	}

	roadsign_t::table.put(desc->get_name(), desc);

	if(  desc->get_wtyp()==track_wt  &&  desc->get_flags()==roadsign_desc_t::SIGN_SIGNAL  ) {
		default_signal = desc;
	}

	return true;
}


/**
 * Fill menu with icons of given signals/roadsings from the list
 */
void roadsign_t::fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_ROADSIGN | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();

	vector_tpl<const roadsign_desc_t *>matching;

	FOR(stringhashtable_tpl<roadsign_desc_t const*>, const& i, table) {
		roadsign_desc_t const* const desc = i.value;
		if(  desc->is_available(time)  &&  desc->get_wtyp()==wtyp  &&  desc->get_builder()  ) {
			// only add items with a cursor
			matching.insert_ordered( desc, compare_roadsign_desc );
		}
	}
	FOR(vector_tpl<roadsign_desc_t const*>, const i, matching) {
		tool_selector->add_tool_selector(i->get_builder());
	}
}


/**
 * Finds a matching roadsign
 */
const roadsign_desc_t *roadsign_t::roadsign_search(roadsign_desc_t::types const flag, waytype_t const wt, uint16 const time)
{
	FOR(stringhashtable_tpl<roadsign_desc_t const*>, const& i, table) {
		roadsign_desc_t const* const desc = i.value;
		if(  desc->is_available(time)  &&  desc->get_wtyp()==wt  &&  desc->get_flags()==flag  ) {
			return desc;
		}
	}
	return NULL;
}


const vector_tpl<const roadsign_desc_t*>& roadsign_t::get_available_signs(const waytype_t wt)
{
	static vector_tpl<const roadsign_desc_t*> dummy;
	dummy.clear();
	const uint16 time = welt->get_timeline_year_month();
	FOR(stringhashtable_tpl<roadsign_desc_t const*>, const& i, table) {
		roadsign_desc_t const* const desc = i.value;
		if(  desc->is_available(time)  &&  desc->get_wtyp()==wt) {
			dummy.append(desc);
		}
	}
	return dummy;
}
