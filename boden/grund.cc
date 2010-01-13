/*
 * Base class for grounds in simutrans.
 * by Hj. Malthaner
 */

#include <string.h>

#include "../simcolor.h"
#include "../simconst.h"
#include "../simdebug.h"
#include "../simdepot.h"
#include "../simfab.h"
#include "../simgraph.h"
#include "../simgraph.h"
#include "../simhalt.h"
#include "../simimg.h"
#include "../simmem.h"
#include "../player/simplay.h"
#include "../simwin.h"
#include "../simworld.h"

#include "../bauer/wegbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/haus_besch.h"
#include "../besch/kreuzung_besch.h"
#include "../besch/tunnel_besch.h"
#include "../besch/weg_besch.h"

#include "../dataobj/freelist.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../dings/baum.h"
#include "../dings/crossing.h"
#include "../dings/groundobj.h"
#include "../dings/label.h"
#include "../dings/leitung2.h"	// for construction of new ways ...
#include "../dings/roadsign.h"
#include "../dings/signal.h"
#include "../dings/tunnel.h"
#include "../dings/wayobj.h"

#include "../gui/ground_info.h"
#include "../gui/karte.h"

#include "../tpl/inthashtable_tpl.h"

#include "../utils/cbuffer_t.h"

#include "wege/kanal.h"
#include "wege/maglev.h"
#include "wege/monorail.h"
#include "wege/narrowgauge.h"
#include "wege/runway.h"
#include "wege/schiene.h"
#include "wege/strasse.h"
#include "wege/weg.h"

#include "fundament.h"
#include "grund.h"
#include "tunnelboden.h"
#include "wasser.h"


// klassenlose funktionen und daten


/**
 * Table of ground texts
 * @author Hj. Malthaner
 */
static inthashtable_tpl<uint32, char*> ground_texts;


static inline uint32 get_ground_text_key(const koord3d& k)
{
	// text for all common heights
	return (k.x << 19) + (k.y << 6) + ((48-(k.z/Z_TILE_STEP))&0x3F);
// only kartenboden can have text!
//		(k.x << 19) + (k.y <<  6) + ((k.z - welt->get_grundwasser()) / Z_TILE_STEP);
}


/**
 * Pointer to the world of this ground. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_t * grund_t::welt = NULL;
bool grund_t::show_grid = false;

uint8 grund_t::offsets[4]={0,1,2/*illegal!*/,2};

sint8 grund_t::underground_level = 127;
uint8 grund_t::underground_mode = ugm_none;

void grund_t::set_underground_mode(const uint8 ugm, const sint8 level)
{
	underground_mode = ugm;
	switch(ugm) {
		case ugm_all:
			underground_level = -128;
			break;
		case ugm_level:
			underground_level = level;
			break;
		case ugm_none:
		default:
			underground_mode = ugm_none;
			underground_level = 127;
	}
}

void grund_t::set_text(const char *text)
{
	const uint32 n = get_ground_text_key(pos);
	if(text) {
		char *new_text = strdup(text);
		free(ground_texts.remove(n));
		ground_texts.put(n, new_text);
		set_flag(has_text);
		set_flag(dirty);
		welt->set_dirty();
	} else if(get_flag(has_text)) {
		char *txt=ground_texts.remove(n);
		free(txt);
		clear_flag(has_text);
		set_flag(dirty);
		welt->set_dirty();
	}
}



const char *grund_t::get_text() const
{
	const char *result = 0;
	if(flags&has_text) {
		result = ground_texts.get( get_ground_text_key(pos) );
		if(result==NULL) {
			return "undef";
		}
		assert(result);
	}
	return result;
}


grund_t::grund_t(karte_t *wl, loadsave_t *file)
{
	// only used for saving?
	welt = wl;
	flags = 0;
	back_bild_nr = 0;
	rdwr(file);
}


void grund_t::rdwr(loadsave_t *file)
{
	// water saves its correct height => no need to save grid heights anymore
	sint8 z = ist_wasser() ? welt->lookup_hgt(pos.get_2d()) : pos.z;

	xml_tag_t g( file, "grund_t" );
	if(file->get_version()<101000) {
		pos.rdwr(file);
	}
	else {
		file->rdwr_byte( z, "" );
		pos.z = get_typ()==grund_t::wasser ? welt->get_grundwasser() : z;
	}

	if(file->is_saving()) {
		const char *text = get_text();
		file->rdwr_str(text);
	}
	else {
		const char *text = 0;
		file->rdwr_str(text);
		if(text) {
			set_text(text);
			guarded_free((void *)text);
		}
	}

	if(file->get_version()<99007) {
		bool label;
		file->rdwr_bool(label, "\n");
		if(label) {
			dinge.add( new label_t(welt, pos, welt->get_spieler(0), get_text() ) );
		}
	}

	sint8 besitzer_n=-1;
	if(file->get_version()<99005) {
		file->rdwr_byte(besitzer_n, "\n");
	}

	if(file->get_version()>=88009) {
		uint8 sl = slope;
		file->rdwr_byte( sl, " " );
		slope = sl;
	}
	else {
		// safe init for old version
		slope = 0;
	}

	// restore grid
	if(  file->is_loading()  ) {
		if(  get_typ()==grund_t::wasser  &&  z>welt->get_grundwasser()  ) {
			z = welt->get_grundwasser();
		}
		else {
			z += corner4(slope);
		}
		welt->set_grid_hgt( pos.get_2d(), z );
	}

	// loading ways from here on
	if(file->is_loading()) {
		waytype_t wtyp;
		int i = -1;
		do {
			wtyp = (waytype_t)file->rd_obj_id();
			weg_t *weg = NULL;

			if(++i < 2) {
				switch(wtyp) {
					default:
						break;

					case road_wt:
						weg = new strasse_t (welt, file);
						break;

					case monorail_wt:
						weg = new monorail_t (welt, file);
						break;

					case maglev_wt:
						weg = new maglev_t (welt, file);
						break;

					case narrowgauge_wt:
						weg = new narrowgauge_t (welt, file);
						break;

					case track_wt: {
						schiene_t *sch = new schiene_t (welt, file);
						if(sch->get_besch()->get_wtyp()==monorail_wt) {
							dbg->warning("grund_t::rdwr()", "converting railroad to monorail at (%i,%i)",get_pos().x, get_pos().y);
							// compatibility code: Convert to monorail
							monorail_t *w= new monorail_t(welt);
							w->set_besch(sch->get_besch());
							w->set_max_speed(sch->get_max_speed());
							w->set_ribi(sch->get_ribi_unmasked());
							delete sch;
							weg = w;
						}
						else {
							weg = sch;
						}
					} break;

					case tram_wt:
						weg = new schiene_t (welt, file);
						if(weg->get_besch()->get_styp()!=weg_t::type_tram) {
							weg->set_besch(wegbauer_t::weg_search(tram_wt,weg->get_max_speed(),0,weg_t::type_tram));
						}
						break;

					case water_wt:
						// ignore old type dock ...
						if(file->get_version()>=87000) {
							weg = new kanal_t (welt, file);
						}
						else {
							uint8 d8;
							sint16 d16;
							sint32 d32;

							file->rdwr_byte(d8, "\n");
							file->rdwr_short(d16, "\n");
							file->rdwr_long(d32, "\n");
							file->rdwr_long(d32, "\n");
							file->rdwr_long(d32, "\n");
							file->rdwr_long(d32, "\n");
							DBG_MESSAGE("grund_t::rdwr()","at (%i,%i) dock ignored",get_pos().x, get_pos().y);
						}
						break;

					case air_wt:
						weg = new runway_t (welt, file);
						break;
				}

				if(weg) {
					if(get_typ()==fundament) {
						// remove this (but we can not correct the other wasy, since possibly not yet loaded)
						dbg->error("grund_t::rdwr()","removing way from foundation at %i,%i",pos.x,pos.y);
						// we do not delete them, to keep maitenance costs correct
					}
					else {
						assert((flags&has_way2)==0);	// maximum two ways on one tile ...
						weg->set_pos(pos);
						if(besitzer_n!=-1) {
							weg->set_besitzer(welt->get_spieler(besitzer_n));
						}
						dinge.add(weg);
						if(flags&has_way1) {
							flags |= has_way2;
						}
						flags |= has_way1;
					}
				}
			}
		} while(wtyp != invalid_wt);

		flags |= dirty;
	}
	else {
		// saving all ways ...
		if(flags&has_way1) {
			file->wr_obj_id( ((weg_t *)obj_bei(0))->get_waytype() );
			obj_bei(0)->rdwr(file);
		}
		if(flags&has_way2) {
			file->wr_obj_id( ((weg_t *)obj_bei(1))->get_waytype() );
			obj_bei(1)->rdwr(file);
		}
		file->wr_obj_id(-1);   // Ende der Wege
	}

	// all objects on this tile
	dinge.rdwr(welt, file, get_pos());

	// need to add a crossing for old games ...
	if (file->is_loading()  &&  ist_uebergang()  &&  !find<crossing_t>(2)) {
		const kreuzung_besch_t *cr_besch = crossing_logic_t::get_crossing( ((weg_t *)obj_bei(0))->get_waytype(), ((weg_t *)obj_bei(1))->get_waytype() );
		if(cr_besch==0) {
			dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", ((weg_t *)obj_bei(0))->get_waytype(), ((weg_t *)obj_bei(1))->get_waytype() );
		}
		crossing_t *cr = new crossing_t( welt, obj_bei(0)->get_besitzer(), pos, cr_besch, ribi_t::ist_gerade_ns(get_weg(cr_besch->get_waytype(1))->get_ribi_unmasked()) );
		dinge.add( cr );
		crossing_logic_t::add( welt, cr, crossing_logic_t::CROSSING_INVALID );
	}
}



grund_t::grund_t(karte_t *wl, koord3d pos)
{
	this->pos = pos;
	flags = 0;
	welt = wl;
	set_bild(IMG_LEER);    // setzt   flags = dirty;
	back_bild_nr = 0;
}



grund_t::~grund_t()
{
	destroy_win((long)this);

	// remove text from table
	set_text(NULL);

	dinge.loesche_alle(NULL,0);
	if(flags&is_halt_flag  &&  welt->ist_in_kartengrenzen(pos.get_2d())) {
		welt->lookup(pos.get_2d())->get_halt()->rem_grund(this);
	}
}




void grund_t::rotate90()
{
	const bool finish_rotate90 = (pos.x==welt->get_groesse_x()-1)  &&  (pos.y==welt->get_groesse_y()-1);
	static inthashtable_tpl<uint32, char*> ground_texts_rotating;
	const uint32 old_n = get_ground_text_key(pos);
	// first internal corrections
	// since the hash changes, we must put the text to the new position
	pos.rotate90( welt->get_groesse_y()-1 );
	slope = hang_t::rotate90( slope );
	// then rotate the things on this tile
	for(  int i=0;  i<dinge.get_top();  i++  ) {
		obj_bei(i)->rotate90();
	}
	// then the text ...
	if(flags&has_text) {
		const uint32 n = get_ground_text_key(pos);
		char *txt = ground_texts.get( old_n );
		ground_texts.put( old_n, NULL );
		ground_texts_rotating.put( n, txt );
	}
	// after processing the last tile, we have to transfer the entries into the original hashtable
	if(finish_rotate90) {
		// first of course remove the old positions
		ground_texts.clear();
		// then transfer all rotated texts
		inthashtable_iterator_tpl<uint32, char*> iter(ground_texts_rotating);
		while(iter.next()) {
			char *txt = iter.get_current_value();
			ground_texts.put( iter.get_current_key(), txt );
		}
		ground_texts_rotating.clear();
	}
}


// moves all objects from the old to the new grund_t
void grund_t::take_obj_from(grund_t* other_gr)
{
	// transfer all things
	while( other_gr->get_top() ) {
		dinge.add( other_gr->obj_remove_top() );
	}
	// transfer the way flags
	if(other_gr->get_flag(has_way1)) {
		flags |= has_way1;
		other_gr->clear_flag(has_way1);
	}
	if(other_gr->get_flag(has_way2)) {
		flags |= has_way2;
		other_gr->clear_flag(has_way2);
	}
}



bool grund_t::zeige_info()
{
	int old_count = win_get_open_count();
	bool success = false;
	if(get_halt().is_bound()) {
		get_halt()->zeige_info();
		if(umgebung_t::single_info  &&  old_count!=win_get_open_count()  ) {
			return true;
		}
		success = true;
	}
	if(umgebung_t::ground_info  ||  hat_wege()) {
		create_win(new grund_info_t(this), w_info, (long)this);
		return true;
	}
	return success;
}


void grund_t::info(cbuffer_t& buf) const
{
	if(flags&is_halt_flag) {
		welt->lookup(pos.get_2d())->get_halt()->info( buf );
	}

	if(!ist_wasser()) {
		if(flags&has_way1) {
			obj_bei(0)->info(buf);
		}
		if(flags&has_way2) {
			obj_bei(1)->info(buf);
		}
		if(ist_uebergang()) {
			find<crossing_t>(2)->get_logic()->info(buf);
		}
	}

#if 0
	if(buf.len() >= 0) {
		uint8 hang= get_grund_hang();
		buf.append(get_hoehe());
		buf.append("\nslope: ");
		buf.append(hang);
		buf.append("\nback0: ");
		buf.append(get_back_bild(0)-grund_besch_t::slopes->get_bild(0));
		buf.append("\nback1: ");
		buf.append(get_back_bild(1)-grund_besch_t::slopes->get_bild(0));
	}
	buf.append("\nway slope");
	buf.append((int)get_weg_hang());
	buf.append("\npos: ");
	buf.append(pos.x);
	buf.append(", ");
	buf.append(pos.y);
	buf.append(", ");
	buf.append(pos.z);

	buf.append("\n\n");
	buf.append(get_name());
	if(get_weg_ribi_unmasked(water_wt)) {
		buf.append("\nwater ribi: ");
		buf.append(get_weg_ribi_unmasked(water_wt));
	}
	buf.append("\ndraw_as_ding=");
	buf.append((flags&draw_as_ding)!=0);
#endif
}



void grund_t::set_halt(halthandle_t halt) {
	if(halt.is_bound()) {
		flags |= is_halt_flag|dirty;
	}
	else {
		flags &= ~is_halt_flag;
		flags |= dirty;
	}
}



halthandle_t grund_t::get_halt() const
{
	return (flags&is_halt_flag) ? welt->lookup(pos.get_2d())->get_halt() : halthandle_t();
}



void grund_t::calc_bild()
{
	// will automatically recalculate ways ...
	dinge.calc_bild();
	// since bridges may alter images of ways, this order is needed!
	calc_bild_internal();
}



ribi_t::ribi grund_t::get_weg_ribi(waytype_t typ) const
{
	weg_t *weg = get_weg(typ);
	return (weg) ? weg->get_ribi() : (ribi_t::ribi)ribi_t::keine;
}



ribi_t::ribi grund_t::get_weg_ribi_unmasked(waytype_t typ) const
{
	weg_t *weg = get_weg(typ);
	return (weg) ? weg->get_ribi_unmasked() : (ribi_t::ribi)ribi_t::keine;
}



image_id grund_t::get_back_bild(int leftback) const
{
	if(back_bild_nr==0) {
		return IMG_LEER;
	}
	sint8 back_bild = abs(back_bild_nr);
	back_bild = leftback ? (back_bild/11)+11 : back_bild%11;
	if(back_bild_nr<0) {
		return grund_besch_t::fundament->get_bild(back_bild);
	}
	else {
		return grund_besch_t::slopes->get_bild(back_bild);
	}
}



// with double height ground tiles!
// can also happen with single height tiles
static inline uint8 get_backbild_from_diff(sint8 h1, sint8 h2)
{
	if(h1==h2) {
		// vertical slope: which height?
		return h1*4;
	}
	else if(h1*h2<0) {
		// middle slop of double height
		return h1<0 ? 9 : 10;
	}
	else {
		return (h1>0?h1:0)+(h2>0?h2:0)*3;
	}
}



// artifical walls from here on ...
void grund_t::calc_back_bild(const sint8 hgt,const sint8 slope_this)
{
	if (underground_mode == ugm_all) {
		this->back_bild_nr = 0;
		return;
	}
	sint8 back_bild_nr=0;
	sint8 is_building=0;
	bool isvisible = is_visible();
	bool fence_west=false, fence_north=false;
	const koord k = get_pos().get_2d();

	clear_flag(grund_t::draw_as_ding);
	if(((flags&has_way1)  &&  ((weg_t *)obj_bei(0))->get_besch()->is_draw_as_ding())  ||  ((flags&has_way2)  &&  ((weg_t *)obj_bei(1))->get_besch()->is_draw_as_ding())) {
		set_flag(grund_t::draw_as_ding);
	}
	bool left_back_is_building = false;

	// check for foundation
	if(k.x>0  &&  k.y>0) {
		const grund_t *gr=welt->lookup(k+koord(-1,-1))->get_kartenboden();
		if(gr) {
			const sint16 left_hgt=gr->get_disp_height()/Z_TILE_STEP;
			const sint8 slope=gr->get_disp_slope();

			const sint8 diff_from_ground = left_hgt+corner2(slope)-hgt-corner4(slope_this);
			// up slope hiding something ...
			if(diff_from_ground<0)  {
				set_flag(grund_t::draw_as_ding);
			}
			else if(gr->get_flag(grund_t::draw_as_ding)  ||  gr->obj_count()>0) {
				left_back_is_building = true;
			}
		}
	}

	// now enter the left two height differences
	if(k.x>0) {
		const grund_t *gr=welt->lookup(k+koord(-1,0))->get_kartenboden();
		if(gr) {
			const sint16 left_hgt=gr->get_disp_height()/Z_TILE_STEP;
			const sint8 slope=gr->get_disp_slope();

			sint8 diff_from_ground_1 = left_hgt+corner2(slope)-hgt;
			sint8 diff_from_ground_2 = left_hgt+corner3(slope)-hgt;

			if (underground_mode==ugm_level) {
				// if exactly one of (this) and (gr) is visible, show full walls
				if ( isvisible && !gr->is_visible()){
					diff_from_ground_1 += 1;
					diff_from_ground_2 += 1;
					set_flag(grund_t::draw_as_ding);
					fence_west = corner1(slope_this)==corner4(slope_this);
				}
				else if ( !isvisible && gr->is_visible()){
					diff_from_ground_1 = 1;
					diff_from_ground_2 = 1;
				}
				// avoid walls that cover the tunnel mounds
				if ( gr->is_visible() && (gr->get_typ()==grund_t::tunnelboden) && ist_karten_boden() && gr->get_pos().z==underground_level && gr->get_grund_hang()==hang_t::west) {
					diff_from_ground_1 = 0;
					diff_from_ground_2 = 0;
				}
				if ( is_visible() && (get_typ()==grund_t::tunnelboden) && ist_karten_boden() && pos.z==underground_level && get_grund_hang()==hang_t::ost) {
					diff_from_ground_1 = 0;
					diff_from_ground_2 = 0;
				}
			}

			// up slope hiding something ...
			if(diff_from_ground_1-corner1(slope_this)<0  ||  diff_from_ground_2-corner4(slope_this)<0)  {
				set_flag(grund_t::draw_as_ding);
				if(  corner1(slope_this)==corner4(slope_this)  ) {
					// ok, we need a fence here, if there is not a vertical bridgehead
					fence_west = (flags&has_way1)==0  ||
								 ( ((static_cast<weg_t *>(obj_bei(0)))->get_ribi_unmasked()&ribi_t::west)==0 &&
									( (flags&has_way2)==0  ||  ((static_cast<weg_t *>(obj_bei(1)))->get_ribi_unmasked()&ribi_t::west)==0) );
				}
			}
			// no fences between water tiles or between invisible tiles
			if (fence_west && ( (ist_wasser() && gr->ist_wasser()) || (!isvisible && !gr->is_visible()) ) ) {
				fence_west = false;
			}
			// any height difference AND something to see?
			if(  (diff_from_ground_1-corner1(slope_this)>0  ||  diff_from_ground_2-corner4(slope_this)>0)
				&&  (diff_from_ground_1>0  ||  diff_from_ground_2>0)  ) {
				back_bild_nr = get_backbild_from_diff( diff_from_ground_1, diff_from_ground_2 );
			}
			// avoid covering of slope by building ...
			if(  (left_back_is_building  ||  gr->get_flag(draw_as_ding))  &&  (back_bild_nr>0  ||  gr->get_back_bild(1)!=IMG_LEER)) {
				set_flag(grund_t::draw_as_ding);
			}
			is_building = gr->get_typ()==grund_t::fundament;
		}
	}

	// now enter the back two height differences
	if(k.y>0) {
		const grund_t *gr=welt->lookup(k+koord(0,-1))->get_kartenboden();
		if(gr) {
			const sint16 back_hgt=gr->get_disp_height()/Z_TILE_STEP;
			const sint8 slope=gr->get_disp_slope();

			sint8 diff_from_ground_1 = back_hgt+corner1(slope)-hgt;
			sint8 diff_from_ground_2 = back_hgt+corner2(slope)-hgt;

			if (underground_mode==ugm_level) {
				// if exactly one of (this) and (gr) is visible, show full walls
				if ( isvisible && !gr->is_visible()){
					diff_from_ground_1 += 1;
					diff_from_ground_2 += 1;
					set_flag(grund_t::draw_as_ding);
					fence_north = corner4(slope_this)==corner3(slope_this);
				}
				else if ( !isvisible && gr->is_visible()){
					diff_from_ground_1 = 1;
					diff_from_ground_2 = 1;
				}
				// avoid walls that cover the tunnel mounds
				if ( gr->is_visible() && (gr->get_typ()==grund_t::tunnelboden) && ist_karten_boden() && gr->get_pos().z==underground_level && gr->get_grund_hang()==hang_t::nord) {
					diff_from_ground_1 = 0;
					diff_from_ground_2 = 0;
				}
				if ( is_visible() && (get_typ()==grund_t::tunnelboden) && ist_karten_boden() && pos.z==underground_level && get_grund_hang()==hang_t::sued) {
					diff_from_ground_1 = 0;
					diff_from_ground_2 = 0;
				}
			}

			// up slope hiding something ...
			if(diff_from_ground_1-corner4(slope_this)<0  ||  diff_from_ground_2-corner3(slope_this)<0) {
				set_flag(grund_t::draw_as_ding);
				if(  corner3(slope_this)==corner4(slope_this)  ) {
					// ok, we need a fence here, if there is not a vertical bridgehead
					fence_north = (flags&has_way1)==0  ||
								 ( ((static_cast<weg_t *>(obj_bei(0)))->get_ribi_unmasked()&ribi_t::nord)==0 &&
									( (flags&has_way2)==0  ||  ((static_cast<weg_t *>(obj_bei(1)))->get_ribi_unmasked()&ribi_t::nord)==0) );
				}
			}
			// no fences between water tiles or between invisible tiles
			if (fence_north && ( (ist_wasser() && gr->ist_wasser()) || (!isvisible && !gr->is_visible()) ) ) {
				fence_north = false;
			}
			// any height difference AND something to see?
			if(  (diff_from_ground_1-corner4(slope_this)>0  ||  diff_from_ground_2-corner3(slope_this)>0)
				&&  (diff_from_ground_1>0  ||  diff_from_ground_2>0)  ) {
				back_bild_nr += get_backbild_from_diff( diff_from_ground_1, diff_from_ground_2 )*11;

			}
			is_building |= gr->get_typ()==grund_t::fundament;
			// avoid covering of slope by building ...
			if(  (left_back_is_building  ||  gr->get_flag(draw_as_ding))  &&  (back_bild_nr>11  ||  gr->get_back_bild(0)!=IMG_LEER)) {
				set_flag(grund_t::draw_as_ding);
			}
		}
	}

	// not ground -> then not draw first ...
	if(welt->lookup(k)->get_kartenboden()!=this) {
		clear_flag(grund_t::draw_as_ding);
	}

	back_bild_nr %= 121;
	this->back_bild_nr = (is_building!=0)? -back_bild_nr : back_bild_nr;
	// needs a fence?
	if(back_bild_nr==0) {
		back_bild_nr = 121;
		if(fence_west) {
			back_bild_nr += 1;
		}
		if(fence_north) {
			back_bild_nr += 2;
		}
		this->back_bild_nr = (get_typ()==grund_t::fundament)? -back_bild_nr : back_bild_nr;
	}
}



PLAYER_COLOR_VAL grund_t::text_farbe() const
{
	// if this gund belongs to a halt, the color should reflect the halt owner, not the grund owner!
	// Now, we use the color of label_t owner
	if(is_halt()  &&  find<label_t>()==NULL) {
		// only halt label
		const halthandle_t halt = welt->lookup(pos.get_2d())->get_halt();
		const spieler_t *sp=halt->get_besitzer();
		if(sp) {
			return PLAYER_FLAG|(sp->get_player_color1()+4);
		}
	}
	// else color according to current owner
	else if(obj_bei(0)) {
		const spieler_t *sp = obj_bei(0)->get_besitzer(); // for cityhall
		const label_t* l = find<label_t>();
		if(l) {
			sp = l->get_besitzer();
		}
		if(sp) {
			return PLAYER_FLAG|(sp->get_player_color1()+4);
		}
	}

	return COL_WHITE;
}



void grund_t::display_boden(const sint16 xpos, const sint16 ypos) const
{
	const bool dirty=get_flag(grund_t::dirty);
	const sint16 rasterweite=get_tile_raster_width();

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	// walls, fences, foundations etc
	if(back_bild_nr!=0) {
		if(abs(back_bild_nr)>121) {
			// fence before a drop
			const sint16 offset = visible && corner4(get_grund_hang()) ? -tile_raster_scale_y( TILE_HEIGHT_STEP/Z_TILE_STEP, get_tile_raster_width()) : 0;
			if(back_bild_nr<0) {
				// behind a building
				display_img(grund_besch_t::fences->get_bild(-back_bild_nr-122+3), xpos, ypos+offset, dirty);
			}
			else {
				// on a normal tile
				display_img(grund_besch_t::fences->get_bild(back_bild_nr-122), xpos, ypos+offset, dirty);
			}
		}
		else {
			// artificial slope
			const sint8 back_bild2 = (abs(back_bild_nr)/11)+11;
			const sint8 back_bild1 = abs(back_bild_nr)%11;
			if(back_bild_nr<0) {
				// for a foundation
				display_img(grund_besch_t::fundament->get_bild(back_bild1), xpos, ypos, dirty);
				display_img(grund_besch_t::fundament->get_bild(back_bild2), xpos, ypos, dirty);
			}
			else {
				// natural
				display_img(grund_besch_t::slopes->get_bild(back_bild1), xpos, ypos, dirty);
				display_img(grund_besch_t::slopes->get_bild(back_bild2), xpos, ypos, dirty);
			}
		}
	}

	// ground
	image_id bild = get_bild();
	if(bild==IMG_LEER) {
		// only check for forced redraw (of marked ... )
		if(dirty) {
			mark_rect_dirty_wc( xpos, ypos+rasterweite/2, xpos+rasterweite-1, ypos+rasterweite-1 );
		}
	}
	else {
		if(get_typ()!=wasser) {
			// show image if tile is visible
			if (visible)  {
				display_img(get_bild(), xpos, ypos, dirty);
				// we show additionally a grid
				// for undergroundmode = ugm_all the grid is plotted in display_dinge
				if(show_grid){
					const uint8 hang = get_grund_hang();
					const uint8 back_hang = (hang&1) + ((hang>>1)&6);
					display_img(grund_besch_t::borders->get_bild(back_hang), xpos, ypos, dirty);
				}
			}
		}
		else {
			// take animation into account
			display_img(get_bild()+wasser_t::stage, xpos, ypos, dirty|wasser_t::change_stage);
		}
	}

	// display ways
	if(visible){
		if(  flags&has_way1  ) {
			sint16 ynpos = ypos-tile_raster_scale_y( get_weg_yoff(), rasterweite );
			ding_t* d = obj_bei(0);
			display_color_img( d->get_bild(), xpos, ynpos, d->get_player_nr(), true, dirty|d->get_flag(ding_t::dirty) );
			PLAYER_COLOR_VAL pc = d->get_outline_colour();
			if(pc) {
				display_img_blend( d->get_bild(), xpos, ynpos, pc, true, dirty|d->get_flag(ding_t::dirty) );
			}
			d->clear_flag( ding_t::dirty );
		}

		if(  flags&has_way2  ){
			sint16 ynpos = ypos-tile_raster_scale_y( get_weg_yoff(), rasterweite );
			ding_t* d = obj_bei(1);
			display_color_img( d->get_bild(), xpos, ynpos, d->get_player_nr(), true, dirty|d->get_flag(ding_t::dirty) );
			PLAYER_COLOR_VAL pc = d->get_outline_colour();
			if(pc) {
				display_img_blend( d->get_bild(), xpos, ynpos, pc, true, dirty|d->get_flag(ding_t::dirty) );
			}
			d->clear_flag( ding_t::dirty );
		}
	}
}



void grund_t::display_dinge(const sint16 xpos, sint16 ypos, const bool is_global) const
{
	const bool dirty = get_flag(grund_t::dirty);
	const uint8 start_offset=offsets[flags/has_way1];

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	if(visible) {
		if(is_global  &&  get_flag(grund_t::marked)) {
			const uint8 hang = get_grund_hang();
			const uint8 back_hang = (hang&1) + ((hang>>1)&6)+8;
			display_img(grund_besch_t::marker->get_bild(back_hang), xpos, ypos, dirty);
			dinge.display_dinge( xpos, ypos, start_offset, is_global );
			display_img(grund_besch_t::marker->get_bild(get_grund_hang()&7), xpos, ypos, dirty);
			//display_img(grund_besch_t::marker->get_bild(get_weg_hang()&7), xpos, ypos, dirty);

			if (!ist_karten_boden()) {
				const grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
				const sint16 raster_tile_width = get_tile_raster_width();
				if (pos.z > gr->get_hoehe()) {
					//display front part of marker for grunds in between
					for(sint8 z = pos.z-Z_TILE_STEP; z>gr->get_hoehe(); z-=Z_TILE_STEP) {
						display_img(grund_besch_t::marker->get_bild(0), xpos, ypos - tile_raster_scale_y( (z-pos.z)*TILE_HEIGHT_STEP/Z_TILE_STEP, raster_tile_width), true);
					}
					//display front part of marker for ground
					display_img(grund_besch_t::marker->get_bild(gr->get_grund_hang()&7), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe()-pos.z)*TILE_HEIGHT_STEP/Z_TILE_STEP, raster_tile_width), true);
				}
				else if (pos.z < gr->get_disp_height()) {
					//display back part of marker for grunds in between
					for(sint8 z = pos.z+Z_TILE_STEP; z<gr->get_disp_height(); z+=Z_TILE_STEP) {
						display_img(grund_besch_t::borders->get_bild(0), xpos, ypos - tile_raster_scale_y( (z-pos.z)*TILE_HEIGHT_STEP/Z_TILE_STEP, raster_tile_width), true);
					}
					//display back part of marker for ground
					const uint8 hang = gr->get_grund_hang() | gr->get_weg_hang();
					const uint8 back_hang = (hang&1) + ((hang>>1)&6);
					display_img(grund_besch_t::borders->get_bild(back_hang), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe()-pos.z)*TILE_HEIGHT_STEP/Z_TILE_STEP, raster_tile_width), true);
				}
			}
		}
		else {
			dinge.display_dinge( xpos, ypos, start_offset, is_global );
		}
	}
	else { // must be karten_boden
		// in undergroundmode: draw ground grid
		const uint8 hang = underground_mode==ugm_all ? get_grund_hang() : hang_t::flach;
		const uint8 back_hang = (hang&1) + ((hang>>1)&6);
		display_img(grund_besch_t::borders->get_bild(back_hang), xpos, ypos, dirty);
		// show marker for marked but invisible tiles
		if(is_global  &&  get_flag(grund_t::marked)) {
			display_img(grund_besch_t::marker->get_bild(back_hang+8), xpos, ypos, dirty);
			display_img(grund_besch_t::marker->get_bild(hang&7), xpos, ypos, dirty);
		}
	}

#ifdef SHOW_FORE_GRUND
	if(get_flag(grund_t::draw_as_ding)) {
		display_fillbox_wh_clip( xpos+raster_tile_width/2, ypos+(raster_tile_width*3)/4, 16, 16, 0, dirty);
	}
#endif
}



void grund_t::display_overlay(const sint16 xpos, const sint16 ypos)
{
	const bool dirty = get_flag(grund_t::dirty);

	// marker/station text
	if(get_flag(has_text)  &&  umgebung_t::show_names) {
		const char *text = get_text();
		if(umgebung_t::show_names & 1) {
			const sint16 raster_tile_width = get_tile_raster_width();
			const int width = proportional_string_width(text)+7;
			display_ddd_proportional_clip(xpos - (width - raster_tile_width)/2, ypos, width, 0, text_farbe(), COL_BLACK, text, dirty);
		}

		// display station waiting information/status
		if(umgebung_t::show_names & 2) {
			const halthandle_t halt = welt->lookup(pos.get_2d())->get_halt();
			if(halt.is_bound()  &&  halt->get_basis_pos3d()==pos) {
				halt->display_status(xpos, ypos);
			}
		}
	}

#ifdef SHOW_FORE_GRUND
	if(get_flag(grund_t::draw_as_ding)) {
		display_fillbox_wh_clip( xpos+raster_tile_width/2, ypos+(raster_tile_width*3)/4, 16, 16, 0, dirty);
	}
#endif

	clear_flag(grund_t::dirty);
}



bool grund_t::weg_erweitern(waytype_t wegtyp, ribi_t::ribi ribi)
{
	weg_t   *weg = get_weg(wegtyp);
	if(weg) {
		weg->ribi_add(ribi);
		weg->count_sign();
		if(weg->is_electrified()) {
			wayobj_t *wo = get_wayobj( wegtyp );
			if( (ribi & wo->get_dir()) == 0 ) {
				// ribi isn't set at wayobj;
				for( uint8 i = 0; i < 4; i++ ) {
					// Add ribis to adjacent wayobj.
					if( ribi_t::nsow[i] & ribi ) {
						grund_t *next_gr;
						if( get_neighbour( next_gr, wegtyp, ribi_t::nsow[i] ) ) {
							wayobj_t *wo2 = next_gr->get_wayobj( wegtyp );
							if( wo2 ) {
								wo->set_dir( wo->get_dir() | ribi_t::nsow[i] );
								wo2->set_dir( wo2->get_dir() | ribi_t::rueckwaerts(ribi_t::nsow[i]) );
							}
						}
					}
				}
			}
		}
		calc_bild();
		set_flag(dirty);
		return true;
	}
	return false;
}


sint64 grund_t::neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, spieler_t *sp)
{
	sint64 cost=0;

	// not already there?
	const weg_t * alter_weg = get_weg(weg->get_waytype());
	if(alter_weg==NULL) {
		// ok, we are unique

		if((flags&has_way1)==0) {
			// new first way here

			// remove all trees ...
			while(1) {
				baum_t *d=find<baum_t>(0);
				if(d==NULL) {
					break;
				}
				// we must mark it by hand, sinc ewe want to join costs
				d->mark_image_dirty( get_bild(), 0 );
				delete d;
				cost -= welt->get_einstellungen()->cst_remove_tree;
			}
			// remove all groundobjs ...
			while(1) {
				groundobj_t *d=find<groundobj_t>(0);
				if(d==NULL) {
					break;
				}
				cost += d->get_besch()->get_preis();
				delete d;
			}


			// add
			weg->set_ribi(ribi);
			weg->set_pos(pos);
			dinge.add( weg );
			flags |= has_way1;
		}
		else {
			// another way will be added
			if(flags&has_way2) {
				dbg->fatal("grund_t::neuen_weg_bauen()","cannot built more than two ways on %i,%i,%i!",pos.x,pos.y,pos.z);
				return 0;
			}
			// add the way
			dinge.add( weg );
			weg->set_ribi(ribi);
			weg->set_pos(pos);
			flags |= has_way2;
			if(ist_uebergang()) {
				// no tram => crossing needed!
				waytype_t w2 =  ((weg_t *)obj_bei( obj_bei(0)==weg ? 1 : 0 ))->get_waytype();
				const kreuzung_besch_t *cr_besch = crossing_logic_t::get_crossing( weg->get_waytype(), w2 );
				if(cr_besch==0) {
					dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", weg->get_waytype(), w2 );
				}
				crossing_t *cr = new crossing_t( welt, obj_bei(0)->get_besitzer(), pos, cr_besch, ribi_t::ist_gerade_ns(get_weg(cr_besch->get_waytype(1))->get_ribi_unmasked()) );
				dinge.add( cr );
				cr->laden_abschliessen();
			}
		}

		// just add the cost
		if(sp && !ist_wasser()) {
			spieler_t::add_maintenance( sp, weg->get_besch()->get_wartung());
			weg->set_besitzer( sp );
		}

		// may result in a crossing, but the wegebauer will recalc all images anyway
		weg->calc_bild();
//		dinge.dump();

		return cost;
	}
	return 0;
}


sint32 grund_t::weg_entfernen(waytype_t wegtyp, bool ribi_rem)
{
DBG_MESSAGE("grund_t::weg_entfernen()","this %p",this);
	weg_t *weg = get_weg(wegtyp);
DBG_MESSAGE("grund_t::weg_entfernen()","weg %p",weg);
	if(weg!=NULL) {

		if(ribi_rem) {
			ribi_t::ribi ribi = weg->get_ribi();
			grund_t *to;

			for(int r = 0; r < 4; r++) {
				if((ribi & ribi_t::nsow[r]) && get_neighbour(to, wegtyp, koord::nsow[r])) {
					weg_t *weg2 = to->get_weg(wegtyp);
					if(weg2) {
						weg2->ribi_rem(ribi_t::rueckwaerts(ribi_t::nsow[r]));
						to->calc_bild();
					}
				}
			}
		}

		sint32 costs=weg->get_besch()->get_preis();	// costs for removal are construction costs
		weg->entferne( NULL );
		delete weg;

		// delete the second way ...
		if(flags&has_way2) {
			flags &= ~has_way2;

			// reset speed limit/crossing info (maybe altered by crossing)
			// Not all ways (i.e. with styp==7) will imply crossins, so wie hav to check
			crossing_t* cr = find<crossing_t>(1);
			if(cr) {
				cr->entferne(0);
				delete cr;
				// restore speed limit
				weg_t* w = (weg_t*)obj_bei(0);
				w->set_besch(w->get_besch());
				w->count_sign();
			}
		}
		else {
			flags &= ~has_way1;
		}

		calc_bild();
		reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());

		return costs;
	}
	return 0;
}


bool grund_t::get_neighbour(grund_t *&to, waytype_t type, koord dir) const
{
	// must be a single direction
	if(  (abs(dir.x)^abs(dir.y))!=1  ) {
		// shorter for:
		// if(dir != koord::nord  &&  dir != koord::sued  &&  dir != koord::ost  &&  dir != koord::west) {
		return false;
	}

	const planquadrat_t * plan = welt->lookup(pos.get_2d() + dir);
	if(!plan) {
		return false;
	}

	// find ground in the same height: This work always!
	const sint16 this_height = get_vmove(dir);
	for( unsigned i=0;  i<plan->get_boden_count();  i++  ) {
		grund_t* gr = plan->get_boden_bei(i);
		if(gr->get_vmove(-dir)==this_height) {
			// test, if connected
			if(!is_connected(gr, type, dir)) {
				continue;
			}
			to = gr;
			return true;
		}
	}
	return false;
}



bool grund_t::is_connected(const grund_t *gr, waytype_t wegtyp, koord dv) const
{
	if(!gr) {
		return false;
	}
	if(wegtyp==invalid_wt) {
		return true;
	}

	const int ribi1 = get_weg_ribi_unmasked(wegtyp);
	const int ribi2 = gr->get_weg_ribi_unmasked(wegtyp);
	if(dv == koord::nord) {
		return (ribi1 & ribi_t::nord) && (ribi2 & ribi_t::sued);
	} else if(dv == koord::sued) {
		return (ribi1 & ribi_t::sued) && (ribi2 & ribi_t::nord);
	} else if(dv == koord::west) {
		return (ribi1 & ribi_t::west) && (ribi2 & ribi_t::ost );
	} else if(dv == koord::ost) {
		return (ribi1 & ribi_t::ost ) && (ribi2 & ribi_t::west);
	}
	return false;
}



// now we need a more sophisticated calculations ...
sint8 grund_t::get_vmove(koord dir) const
{
	const sint8 slope=get_weg_hang();
	sint8 h=get_hoehe();
	if(ist_bruecke()  &&  get_grund_hang()!=0  &&  welt->lookup(pos)==this) {
		h += Z_TILE_STEP;	// end or start of a bridge
	}

	if(dir == koord::ost) {
		h += corner3(slope)*Z_TILE_STEP;
	} else if(dir == koord::west) {
		h += corner1(slope)*Z_TILE_STEP;
	} else if(dir == koord::sued) {
		h += corner1(slope)*Z_TILE_STEP;
	} else if(dir == koord::nord) {
		h += corner3(slope)*Z_TILE_STEP;
	}
	else {
		// commented out: allow diagonal directions now (assume flat for these)
		//dbg->fatal("grund_t::get_vmove()","no valid direction given (%x)",ribi_typ(dir));	// error: not a direction ...
	}
	return h;
}



int grund_t::get_max_speed() const
{
	int max = 0;
	if(flags&has_way1) {
		max = ((weg_t *)obj_bei(0))->get_max_speed();
	}
	if(flags&has_way2) {
		max = min( max, ((weg_t *)obj_bei(1))->get_max_speed() );
	}
	return max;
}



bool grund_t::remove_everything_from_way(spieler_t* sp, waytype_t wt, ribi_t::ribi rem)
{
	// check, if the way must be totally removed?
	weg_t *weg = get_weg(wt);
	if(weg) {
		koord here = pos.get_2d();

		// stopps
		if(flags&is_halt_flag  &&  (get_halt()->get_besitzer()==sp  || sp==welt->get_spieler(1))) {
			bool remove_halt = get_typ()!=boden;
			// remove only if there is no other way
			if(get_weg_nr(1)==NULL) {
				remove_halt = true;
			}
			else {
#ifdef delete_matching_stops
				// delete halts with the same waytype ... may lead to confusing / unexpected but completely logical results ;)
				gebaeude_t *gb = find<gebaeude_t>();
				if (gb) {
					waytype_t halt_wt = (waytype_t)gb->get_tile()->get_besch()->get_extra();
					if (halt_wt == wt || (wt==track_wt && halt_wt==tram_wt) || (wt==tram_wt && halt_wt==track_wt)) {
						remove_halt = true;
					}
				}
#else
				remove_halt = false;
#endif
			}
			if (remove_halt) {
				const char *fail;
				if (!haltestelle_t::remove(welt, sp, pos, fail)) {
					return false;
				}
			}
		}

		ribi_t::ribi add=(weg->get_ribi_unmasked()&rem);
		sint32 costs = 0;

		for(  sint16 i=get_top();  i>=0;  i--  ) {
			// we need to delete backwards, since we might miss things otherwise
			if(  i>=get_top()  ) {
				continue;
			}

			ding_t *d=obj_bei((uint8)i);
			// do not delete ways
			if(d->is_way()) {
				continue;
			}
			// roadsigns: check dir: dirs changed? delete
			if(d->get_typ()==ding_t::roadsign  &&  ((roadsign_t *)d)->get_besch()->get_wtyp()==wt) {
				if((((roadsign_t *)d)->get_dir()&(~add))!=0) {
					costs -= ((roadsign_t *)d)->get_besch()->get_preis();
					delete d;
				}
			}
			// singal: not on crossings => remove all
			else if(d->get_typ()==ding_t::signal  &&  ((roadsign_t *)d)->get_besch()->get_wtyp()==wt) {
				costs -= ((roadsign_t *)d)->get_besch()->get_preis();
				delete d;
			}
			// wayobj: check dir
			else if(add==ribi_t::keine  &&  d->get_typ()==ding_t::wayobj  &&  ((wayobj_t *)d)->get_besch()->get_wtyp()==wt) {
				uint8 new_dir=((wayobj_t *)d)->get_dir()&add;
				if(new_dir) {
					// just change dir
					((wayobj_t *)d)->set_dir(new_dir);
				}
				else {
					costs -= ((wayobj_t *)d)->get_besch()->get_preis();
					delete d;
				}
			}
			// citycar or pedestrians: just delete
			else if (wt==road_wt  &&  (d->get_typ()==ding_t::verkehr || d->get_typ()==ding_t::fussgaenger)) {
				delete d;
			}
			// remove tunnel portal, if not the last tile ...
			// must be done before weg_entfernen() to get maintenance right
			else if(d->get_typ()==ding_t::tunnel) {
				uint8 wt = ((tunnel_t *)d)->get_besch()->get_waytype();
				if (weg->get_waytype()==wt) {
					if((flags&has_way2)==0) {
						if (add==ribi_t::keine) {
							// last way was belonging to this tunnel
							d->entferne(sp);
							delete d;
						}
					}
					else {
						// we must leave the way to prevent destroying the other one
						add |= get_weg_nr(1)->get_ribi_unmasked();
						weg->calc_bild();
					}
				}
			}
		}

		// need to remove railblocks to recalcualte connections
		// remove all ways or just some?
		if(add==ribi_t::keine) {
			costs -= weg_entfernen(wt, true);
			// make tunnel portals to normal ground
			if((flags&is_kartenboden)  &&  get_typ()==tunnelboden  &&  (flags&has_way1)==0) {
				// remove remaining dings
				obj_loesche_alle( sp );
				// set to normal ground
				welt->access(pos.get_2d())->kartenboden_setzen( new boden_t( welt, pos, slope ) );
			}
		}
		else {
DBG_MESSAGE("wkz_wayremover()","change remaining way to ribi %d",add);
			// something will remain, we just change ribis
			weg->set_ribi(add);
			calc_bild();
		}
		// we have to pay?
		if(costs) {
			spieler_t::accounting(sp, costs, here, COST_CONSTRUCTION);
		}
	}
	return true;
}


void* grund_t::operator new(size_t s)
{
	return freelist_t::gimme_node(s);
}


void grund_t::operator delete(void* p, size_t s)
{
	return freelist_t::putback_node(s, p);
}

wayobj_t *grund_t::get_wayobj( waytype_t wt ) const
{
	waytype_t wt1 = ( wt == tram_wt ) ? track_wt : wt;

	wayobj_t *wayobj;
	if(  find<wayobj_t>()  ) {
		// since there might be more than one, we have to iterate through all of them
		for(  uint8 i = 0;  i < get_top();  i++  ) {
			ding_t *d = obj_bei(i);
			if(  d  &&  d->get_typ() == ding_t::wayobj  ) {
				wayobj = (wayobj_t *)d;
				waytype_t wt2 = wayobj->get_besch()->get_wtyp();
				if(  wt2 == tram_wt  ) {
					wt2 = track_wt;
				}
				if(  wt1 == wt2  ) {
					return wayobj;
				}
			}
		}
	}
	return NULL;
}
