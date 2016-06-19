/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Roadsigns functions and dialogs
 */

#include <stdio.h>

#include "../simunits.h"
#include "../simdebug.h"
#include "../simobj.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../simtool.h"
#include "../simworld.h"
#include "../simsignalbox.h"

#include "../besch/roadsign_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/haus_besch.h"

#include "../boden/grund.h"
#include "../boden/wege/strasse.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../gui/trafficlight_info.h"
#include "../gui/privatesign_info.h"
#include "../gui/tool_selector.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "roadsign.h"
#include "signal.h"


const roadsign_besch_t *roadsign_t::default_signal=NULL;

stringhashtable_tpl<const roadsign_besch_t *> roadsign_t::table;
vector_tpl<roadsign_besch_t*> roadsign_t::liste;


#ifdef INLINE_OBJ_TYPE
roadsign_t::roadsign_t(typ type, loadsave_t *file) : obj_t (type)
{
	init(file);
}

roadsign_t::roadsign_t(loadsave_t *file) : obj_t (obj_t::roadsign)
{
	init(file);
}

void roadsign_t::init(loadsave_t *file)
#else
roadsign_t::roadsign_t(loadsave_t *file) : obj_t ()
#endif
{
	image = after_bild = IMG_LEER;
	preview = false;
	rdwr(file);
	if(besch) {
		/* if more than one state, we will switch direction and phase for traffic lights
		 * however also gate signs need indications
		 */
		automatic = (besch->get_bild_anzahl()>4  &&  besch->get_wtyp()==road_wt)  ||  (besch->get_bild_anzahl()>2  &&  besch->is_private_way());
	}
	else {
		automatic = false;
	}
	// some sve had rather strange entries in state
	if(  !automatic  ||  besch==NULL  ) {
		state = 0;
	}
	// only traffic light need switches
	if(  automatic  ) {
		welt->sync_add(this);
	}
}

#ifdef INLINE_OBJ_TYPE

roadsign_t::roadsign_t(typ type, player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t* besch, bool preview) : obj_t(type, pos)
{
	init(player, dir, besch, preview);
}

roadsign_t::roadsign_t(player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t *besch, bool preview) : obj_t(obj_t::roadsign, pos)
{
	init(player, dir, besch, preview);
}

void roadsign_t::init(player_t *player, ribi_t::ribi dir, const roadsign_besch_t *besch, bool preview)
#else
roadsign_t::roadsign_t(player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t *besch, bool preview) : obj_t(pos)
#endif
{
	this->besch = besch;
	this->dir = dir;
	this->preview = preview;
	image = after_bild = IMG_LEER;
	state = 0;
	ticks_ns = ticks_ow = 16;
	ticks_offset = 0;
	set_owner( player );
	if(  besch->is_private_way()  ) {
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
	automatic = (besch->get_bild_anzahl()>4  &&  besch->get_wtyp()==road_wt)  ||  (besch->get_bild_anzahl()>2  &&  besch->is_private_way());
	// only traffic light need switches
	if(  automatic  ) {
		welt->sync_add(this);
	}
}


roadsign_t::~roadsign_t()
{
	if(  besch  ) {
		const grund_t *gr = welt->lookup(get_pos());
		if(gr) {
			weg_t* weg = gr->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
			if(weg) {
				player_t* owner = get_owner();
				if(owner)
				{
					// Remove maintenance cost
					sint32 maint = get_besch()->get_maintenance();
					player_t::add_maintenance(owner, -maint, weg->get_waytype()); 
				}
				if (!preview) {
					if (besch->is_single_way() || besch->is_signal_type()) {
						// signal removed, remove direction mask
						weg->set_ribi_maske(ribi_t::keine);
					}
				weg->clear_sign_flag();
				}
			}
			else {
				dbg->error("roadsign_t::~roadsign_t()","roadsign_t %p was deleted but ground has no way of type %d!", besch->get_wtyp() );
			}
		}
	}
	if(automatic) {
		welt->sync_remove(this);
	}
}


void roadsign_t::set_dir(ribi_t::ribi dir)
{
	ribi_t::ribi olddir = this->dir;

	this->dir = dir;
	if (!preview) {
		weg_t *weg = welt->lookup(get_pos())->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		if(  besch->get_wtyp()!=track_wt  &&  besch->get_wtyp()!=monorail_wt  &&  besch->get_wtyp()!=maglev_wt  &&  besch->get_wtyp()!=narrowgauge_wt  ) {
			weg->count_sign();
		}
		if(  besch->is_single_way()  ||  besch->is_signal()  ||  besch->is_pre_signal()  ||  besch->is_longblock_signal()  ) {
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
	mark_image_dirty(after_bild,after_yoffset-get_yoff());
	set_xoff( old_x );

	image = IMG_LEER;
	after_bild = IMG_LEER;
	calc_image();

	if (preview)
	{
		this->dir = olddir;
	}
}


void roadsign_t::show_info()
{
	if(  besch->is_private_way()  ) {
		create_win(new privatesign_info_t(this), w_info, (ptrdiff_t)this );
	}
	else if(  automatic  ) {
		create_win(new trafficlight_info_t(this), w_info, (ptrdiff_t)this );
	}
	else {
		obj_t::show_info();
	}
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void roadsign_t::info(cbuffer_t & buf, bool dummy) const
{
	obj_t::info( buf );
	if(  besch->is_private_way()  ) {
		buf.append( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
	}
	else {
		buf.append(translator::translate("Roadsign"));
		buf.append("\n");
		if(besch->is_single_way()) {
			buf.append(translator::translate("\nsingle way"));
		}
		if(besch->get_min_speed()!=0) {
			buf.printf("%s%d", translator::translate("\nminimum speed:"), speed_to_kmh(besch->get_min_speed()));
		}
#ifdef DEBUG
		buf.append(translator::translate("\ndirection:"));
		buf.append(dir);
		buf.append("\n");
#endif
		if(  automatic  ) {
			buf.append(translator::translate("\nSet phases:"));
			buf.append("\n\n");;
		}
	}
	// Signal specific information is dealt with in void signal_t::info(cbuffer_t & buf, bool dummy) const (in signal.cc)
}


// could be still better aligned for drive_left settings ...
// now only an offset in besch could improve it ...
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
	const bool left_offsets = besch->get_offset_left()  &&
	    (     (besch->get_wtyp()==road_wt  &&  welt->get_settings().is_drive_left()  )
	      ||  (besch->get_wtyp()!=air_wt  &&  besch->get_wtyp()!=road_wt  &&  welt->get_settings().is_signals_left())
	    );

	const sint8 height_step = TILE_HEIGHT_STEP << hang_t::ist_doppel(gr->get_weg_hang());

	const hang_t::typ full_hang = gr->get_weg_hang();
	const sint8 hang_diff = hang_t::max_diff(full_hang);
	const ribi_t::ribi hang_dir = ribi_t::rueckwaerts( ribi_typ(full_hang) );

	// private way have also closed/open states
	if(  besch->is_private_way()  ) {
		uint8 image = 1-(dir&1);
		if(  (1<<welt->get_active_player_nr()) & get_player_mask()  ) {
			// gate open
			image += 2;
			// force redraw
		    mark_image_dirty(get_image(),0);
		}
		set_bild( besch->get_bild_nr(image) );
		set_yoff( 0 );
		if(  hang_diff  ) {
			if(hang_dir==ribi_t::west ||  hang_dir==ribi_t::nord) {
				set_yoff( -(height_step>>1) );
			}
		}
		else {
			set_yoff( -gr->get_weg_yoff() );
		}
		after_bild = IMG_LEER;
		return;
	}

	if(  hang_diff == 0  ) {
		yoff = -gr->get_weg_yoff();
		after_yoffset = yoff;
	}
	else {
		// since the places were switched
		const ribi_t::ribi test_hang = left_offsets ? ribi_t::rueckwaerts(hang_dir) : hang_dir;
		if(test_hang==ribi_t::ost ||  test_hang==ribi_t::nord) {
			yoff = -TILE_HEIGHT_STEP*hang_diff;
			after_yoffset = 0;
		}
		else {
			yoff = 0;
			after_yoffset = -TILE_HEIGHT_STEP*hang_diff;
		}
	}

	image_id tmp_bild=IMG_LEER;
	if(!automatic) {
		assert( state==0 );

		after_bild = IMG_LEER;
		ribi_t::ribi temp_dir = dir;

		if(  gr->get_typ()==grund_t::tunnelboden  &&  gr->ist_karten_boden()  &&
			(grund_t::underground_mode==grund_t::ugm_none  ||  (grund_t::underground_mode==grund_t::ugm_level  &&  gr->get_hoehe()<grund_t::underground_level))   ) {
			// entering tunnel here: hide the image further in if not undergroud/sliced
			const ribi_t::ribi tunnel_hang_dir = ribi_t::rueckwaerts( ribi_typ(gr->get_grund_hang()) );
			if(  tunnel_hang_dir==ribi_t::ost ||  tunnel_hang_dir==ribi_t::nord  ) {
				temp_dir &= ~ribi_t::suedwest;
			}
			else {
				temp_dir &= ~ribi_t::nordost;
			}
		}

		// signs for left side need other offsets and other front/back order
		if(  left_offsets  ) {
			const sint16 XOFF = 2*besch->get_offset_left();
			const sint16 YOFF = besch->get_offset_left();

			if(temp_dir&ribi_t::ost) {
				tmp_bild = besch->get_bild_nr(3);
				xoff += XOFF;
				yoff += -YOFF;
			}

			if(temp_dir&ribi_t::nord) {
				if(tmp_bild!=IMG_LEER) {
					after_bild = besch->get_bild_nr(0);
					after_xoffset += -XOFF;
					after_yoffset += -YOFF;
				}
				else {
					tmp_bild = besch->get_bild_nr(0);
					xoff += -XOFF;
					yoff += -YOFF;
				}
			}

			if(temp_dir&ribi_t::west) {
				after_bild = besch->get_bild_nr(2);
				after_xoffset += -XOFF;
				after_yoffset += YOFF;
			}

			if(temp_dir&ribi_t::sued) {
				if(after_bild!=IMG_LEER) {
					tmp_bild = besch->get_bild_nr(1);
					xoff += XOFF;
					yoff += YOFF;
				}
				else {
					after_bild = besch->get_bild_nr(1);
					after_xoffset += XOFF;
					after_yoffset += YOFF;
				}
			}
		}
		else {

			if(temp_dir&ribi_t::ost) {
				after_bild = besch->get_bild_nr(3);
			}

			if(temp_dir&ribi_t::nord) {
				if(after_bild!=IMG_LEER) {
					tmp_bild = besch->get_bild_nr(0);
				}
				else {
					after_bild = besch->get_bild_nr(0);
				}
			}

			if(temp_dir&ribi_t::west) {
				tmp_bild = besch->get_bild_nr(2);
			}

			if(temp_dir&ribi_t::sued) {
				if(tmp_bild!=IMG_LEER) {
					after_bild = besch->get_bild_nr(1);
				}
				else {
					tmp_bild = besch->get_bild_nr(1);
				}
			}
		}

		// some signs on roads must not have a background (but then they have only two rotations)
		if(  besch->get_flags()&roadsign_besch_t::ONLY_BACKIMAGE  ) {
			if(after_bild!=IMG_LEER) {
				tmp_bild = after_bild;
			}
			after_bild = IMG_LEER;
		}
	}
	else {
		// traffic light
		weg_t *str=gr->get_weg(road_wt);
		if(str) {
			const uint8 weg_dir = str->get_ribi_unmasked();
			const uint8 direction = (dir&ribi_t::nord)!=0;

			// other front/back images for left side ...
			if(  left_offsets  ) {
				const sint16 XOFF = 2*besch->get_offset_left();
				const sint16 YOFF = besch->get_offset_left();

				if(weg_dir&ribi_t::nord) {
					if(weg_dir&ribi_t::ost) {
						after_bild = besch->get_bild_nr(6+direction*8);
						after_xoffset += 0;
						after_yoffset += 0;
					}
					else {
						after_bild = besch->get_bild_nr(1+direction*8);
						after_xoffset += XOFF;
						after_yoffset += YOFF;
					}
				}
				else if(weg_dir&ribi_t::ost) {
					after_bild = besch->get_bild_nr(2+direction*8);
					after_xoffset += -XOFF;
					after_yoffset += YOFF;
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::sued) {
						tmp_bild = besch->get_bild_nr(7+direction*8);
						xoff += 0;
						yoff += 0;
					}
					else {
						tmp_bild = besch->get_bild_nr(3+direction*8);
						xoff += XOFF;
						yoff += -YOFF;
					}
				}
				else if(weg_dir&ribi_t::sued) {
					tmp_bild = besch->get_bild_nr(0+direction*8);
					xoff += -XOFF;
					yoff += -YOFF;
				}
			}
			else {
				// drive right ...
				if(weg_dir&ribi_t::sued) {
					if(weg_dir&ribi_t::ost) {
						after_bild = besch->get_bild_nr(4+direction*8);
					}
					else {
						after_bild = besch->get_bild_nr(0+direction*8);
					}
				}
				else if(weg_dir&ribi_t::ost) {
					after_bild = besch->get_bild_nr(2+direction*8);
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::nord) {
						tmp_bild = besch->get_bild_nr(5+direction*8);
					}
					else {
						tmp_bild = besch->get_bild_nr(3+direction*8);
					}
				}
				else if(weg_dir&ribi_t::nord) {
					tmp_bild = besch->get_bild_nr(1+direction*8);
				}
			}

		}
	}
	// set image and offsets
	set_bild( tmp_bild );
	set_xoff( xoff );
	set_yoff( yoff );

}


// only used for traffic light: change the current state
bool roadsign_t::sync_step(long /*delta_t*/)
{
	if(  besch->is_private_way()  ) {
		uint8 image = 1-(dir&1);
		if(  (1<<welt->get_active_player_nr()) & get_player_mask()  ) {
			// gate open
			image += 2;
			// force redraw
			mark_image_dirty(get_image(),0);
		}
		set_bild( besch->get_bild_nr(image) );
	}
	else {
		// change every ~32s
		uint32 ticks = ((welt->get_zeit_ms()>>10)+ticks_offset) % (ticks_ns+ticks_ow);

		uint8 new_state = (ticks >= ticks_ns) ^ (welt->get_settings().get_rotation() & 1);
		if(state!=new_state) {
			state = new_state;
			dir = (new_state==0) ? ribi_t::nordsued : ribi_t::ostwest;
			calc_image();
		}
	}
	return true;
}


void roadsign_t::rotate90()
{
	// only meaningful for traffic lights
	obj_t::rotate90();
	if(automatic  &&  !besch->is_private_way()) {
		state = (state+1)&1;
		uint8 temp = ticks_ns;
		ticks_ns = ticks_ow;
		ticks_ow = temp;

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
	if(  after_bild != IMG_LEER  ) {
		const int raster_width = get_current_tile_raster_width();
		xpos += tile_raster_scale_x( after_xoffset, raster_width );
		ypos += tile_raster_scale_y( after_yoffset, raster_width );
		// draw with owner
		if(  get_player_nr() != PLAYER_UNOWNED  ) {
			if(  obj_t::show_owner  ) {
				display_blend( after_bild, xpos, ypos, 0, (get_owner()->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, dirty  CLIP_NUM_PAR);
			}
			else {
				display_color( after_bild, xpos, ypos, get_player_nr(), true, get_flag(obj_t::dirty)  CLIP_NUM_PAR);
			}
		}
		else {
			display_normal( after_bild, xpos, ypos, 0, true, get_flag(obj_t::dirty)  CLIP_NUM_PAR);
		}
	}
}


void roadsign_t::rdwr(loadsave_t *file)
{
	xml_tag_t r( file, "roadsign_t" );

	obj_t::rdwr(file);

	uint8 dummy=0;
	if(  file->get_version()<=102002  ) {
		file->rdwr_byte(dummy);
		if(  file->is_loading()  ) {
			ticks_ns = ticks_ow = 16;
		}
	}
	else {
		file->rdwr_byte(ticks_ns);
		file->rdwr_byte(ticks_ow);
	}
	if(  file->get_version()>=110007 || file->get_experimental_version() >= 10 ) {
		file->rdwr_byte(ticks_offset);
	}
	else {
		if(  file->is_loading()  ) {
			ticks_offset = 0;
		}
	}

	dummy = state;
	file->rdwr_byte(dummy);
	state = dummy;
	dummy = dir;
	file->rdwr_byte(dummy);
	dir = dummy;
	if(file->get_version()<89000) {
		dir = ribi_t::rueckwaerts(dir);
	}

	if(file->is_saving()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		besch = roadsign_t::table.get(bname);
		if(besch==NULL) {
			besch = roadsign_t::table.get(translator::compatibility_name(bname));
			if(  besch==NULL  ) {
				dbg->warning("roadsign_t::rwdr", "description %s for roadsign/signal at %d,%d not found! (may be ignored)", bname, get_pos().x, get_pos().y);
				welt->add_missing_paks( bname, karte_t::MISSING_SIGN );
			}
			else {
				dbg->warning("roadsign_t::rwdr", "roadsign/signal %s at %d,%d replaced by %s", bname, get_pos().x, get_pos().y, besch->get_name() );
			}
		}
		// init ownership of private ways signs
		if(  file->get_version()<110007  &&  besch  &&  besch->is_private_way()  ) {
			ticks_ns = 0xFD;
			ticks_ow = 0xFF;
		}
	}

	if(besch && besch->is_signal_type() && file->is_saving())
	{
		signal_t* sig = (signal_t*)this;
		sig->rdwr_signal(file);
	}
}


void roadsign_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, -besch->get_preis(), get_pos().get_2d(), get_waytype());
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void roadsign_t::finish_rd()
{
	grund_t *gr=welt->lookup(get_pos());
	if(  gr==NULL  ||  !gr->hat_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt)  ) {
		dbg->error("roadsign_t::finish_rd","roadsing: way/ground missing at %i,%i => ignore", get_pos().x, get_pos().y );
	}
	else {
		// after loading restore directions
		set_dir(dir);
		gr->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt)->count_sign();
	}
}


// to sort compare_roadsign_besch for always the same menu order
static bool compare_roadsign_besch(const roadsign_besch_t* a, const roadsign_besch_t* b)
{
	int diff = a->get_wtyp() - b->get_wtyp();
	if (diff == 0) {
		if(a->is_choose_sign()) {
			diff += 120;
		}
		if(b->is_choose_sign()) {
			diff -= 120;
		}
		diff += (int)(a->get_flags() & ~roadsign_besch_t::SIGN_SIGNAL) - (int)(b->get_flags()  & ~roadsign_besch_t::SIGN_SIGNAL);
	}
	if (diff == 0) {
		/* Some type: sort by name */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


/* static stuff from here on ... */
bool roadsign_t::alles_geladen()
{
	if(table.empty()) {
		DBG_MESSAGE("roadsign_t", "No signs found - feature disabled");
	}
	return true;
}


bool roadsign_t::register_besch(roadsign_besch_t *besch)
{
	// avoid duplicates with same name
	const roadsign_besch_t *old_besch = table.get(besch->get_name());
	if(old_besch) {
		dbg->warning( "roadsign_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		table.remove(besch->get_name());
		tool_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}

	if(  besch->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
		// add the tool
		tool_build_roadsign_t *tool = new tool_build_roadsign_t();
		tool->set_icon( besch->get_cursor()->get_bild_nr(1) );
		tool->cursor = besch->get_cursor()->get_bild_nr(0);
		tool->set_default_param(besch->get_name());
		tool_t::general_tool.append( tool );
		besch->set_builder( tool );
	}
	else {
		besch->set_builder( NULL );
	}

	roadsign_t::table.put(besch->get_name(), besch);
	roadsign_t::liste.append(besch);

	if(  besch->get_wtyp()==track_wt  &&  besch->get_flags()==roadsign_besch_t::SIGN_SIGNAL  ) {
		default_signal = besch;
	}

	return true;
}


/**
 * Fill menu with icons of given signals/roadsings from the list
 * @author Hj. Malthaner
 */
void roadsign_t::fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_ROADSIGN | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();

	vector_tpl<const roadsign_besch_t *>matching;

	FOR(stringhashtable_tpl<roadsign_besch_t const*>, const& i, table) {
		roadsign_besch_t const* const besch = i.value;

		bool allowed_given_current_signalbox; 
		uint32 signal_group = besch->get_signal_group();
		
		if(signal_group)
		{
			player_t* player = welt->get_active_player();
			if(player)
			{
				signalbox_t* sb = player->get_selected_signalbox();
				if(!sb)
				{
					allowed_given_current_signalbox = false;
				}
				else
				{
					allowed_given_current_signalbox = sb->can_add_signal(besch); 
				}
			}
			else
			{
				allowed_given_current_signalbox = false;
			}
		}
		else
		{
			allowed_given_current_signalbox = true;
		}

		bool allowed_given_underground_state = true;
		if(grund_t::underground_mode == grund_t::ugm_none)
		{
			allowed_given_underground_state = besch->get_allow_underground() == 0 || besch->get_allow_underground() == 2;
		}

		if(grund_t::underground_mode == grund_t::ugm_all)
		{
			allowed_given_underground_state = besch->get_allow_underground() == 1 || besch->get_allow_underground() == 2;
		}
		
		if(besch->is_available(time) && besch->get_wtyp() == wtyp && besch->get_builder() && allowed_given_current_signalbox && allowed_given_underground_state)
		{
			// only add items with a cursor
			matching.insert_ordered( besch, compare_roadsign_besch );
		}
	}
	FOR(vector_tpl<roadsign_besch_t const*>, const i, matching) {
		tool_selector->add_tool_selector(i->get_builder());
	}
}


/**
 * Finds a matching roadsign
 * @author prissi
 */
const roadsign_besch_t *roadsign_t::roadsign_search(roadsign_besch_t::types const flag, waytype_t const wt, uint16 const time)
{
	FOR(stringhashtable_tpl<roadsign_besch_t const*>, const& i, table) {
		roadsign_besch_t const* const besch = i.value;
		if(  besch->is_available(time)  &&  besch->get_wtyp()==wt  &&  besch->get_flags()==flag  ) {
			return besch;
		}
	}
	return NULL;
}

const roadsign_besch_t* roadsign_t::find_best_upgrade(bool underground)
{
	const uint16 time = welt->get_timeline_year_month();
	const roadsign_besch_t* best_candidate = NULL;

	FOR(stringhashtable_tpl<roadsign_besch_t const*>, const& i, table)
	{
		roadsign_besch_t const* const new_roadsign_type = i.value;
		if(new_roadsign_type->is_available(time)
			&& new_roadsign_type->get_upgrade_group() == besch->get_upgrade_group()
			&& new_roadsign_type->get_wtyp() == besch->get_wtyp()
			&& new_roadsign_type->get_flags() == besch->get_flags() 
			&& new_roadsign_type->get_working_method() == besch->get_working_method()
			&& (new_roadsign_type->get_allow_underground() == 2
			|| (underground && new_roadsign_type->get_allow_underground() == 1)
			|| (!underground && new_roadsign_type->get_allow_underground() == 0)))
		{
			if(best_candidate)
			{
				// Find the most recent current replacement signal/sign
				if(new_roadsign_type->get_intro_year_month() > best_candidate->get_intro_year_month())
				{
					best_candidate = new_roadsign_type;
				}
			}
			else
			{
				best_candidate = new_roadsign_type;
			}
		}
	}

	return best_candidate; 
}

 void roadsign_t::set_scale(uint16 scale_factor)
{
	// Called from the world's set_scale method so as to avoid having to export the internal data structures of this class.
	FOR(vector_tpl<roadsign_besch_t *>, sign, liste)
	{
		sign->set_scale(scale_factor); 
	}
}

 bool roadsign_t::upgrade(const roadsign_besch_t* new_besch)
 {
	 if(!new_besch)
	 {
		 return false;
	 }

	 if(besch->get_upgrade_group() == 0)
	 { 
		 // Can only upgrade if an upgrade group is defined.
		 return false;
	 }

	 const roadsign_besch_t* old_besch = besch;
	 if(old_besch->get_upgrade_group() != new_besch->get_upgrade_group())
	 {
		 return false;
	 }

	 if(!get_owner()->can_afford(new_besch->get_way_only_cost()))
	 {
		 return false;
	 }

	 sint32 diff = new_besch->get_maintenance() - old_besch->get_maintenance();
	 player_t::add_maintenance(get_owner(), diff, get_waytype()); 
	 player_t::book_construction_costs(get_owner(), new_besch->get_way_only_cost(), get_pos().get_2d(), get_waytype()); 

	 besch = new_besch;
	 welt->set_dirty(); 
	 return true;
 }
