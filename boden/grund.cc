/* grund.cc
 *
 * Untergrund Basisklasse für Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <string.h>

#include "../simdebug.h"

#include "grund.h"
#include "fundament.h"
#include "tunnelboden.h"

#include "wege/weg.h"
#include "wege/monorail.h"
#include "wege/schiene.h"
#include "wege/strasse.h"
#include "wege/kanal.h"
#include "wege/runway.h"

#include "../gui/karte.h"
#include "../gui/ground_info.h"

#include "../simworld.h"
#include "../simvehikel.h"
#include "../simplay.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simgraph.h"
#include "../simdepot.h"
#include "../simfab.h"
#include "../simhalt.h"

#include "../dings/baum.h"
#include "../dings/bruecke.h"
#include "../dings/tunnel.h"
#include "../dings/gebaeude.h"
#include "../dings/label.h"
#include "../dings/crossing.h"
#include "../dings/signal.h"
#include "../dings/roadsign.h"
#include "../dings/wayobj.h"
#include "../simverkehr.h"
#include "../simpeople.h"

#include "../besch/haus_besch.h"
#include "../besch/kreuzung_besch.h"
#include "../besch/tunnel_besch.h"

#include "../bauer/wegbauer.h"
#include "../besch/weg_besch.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"

#include "../simcolor.h"
#include "../simconst.h"
#include "../simgraph.h"


#include "../tpl/ptrhashtable_tpl.h"
#include "../tpl/inthashtable_tpl.h"

#include "../dings/leitung2.h"	// for construction of new ways ...
#include "../dings/label.h"	// for construction of new ways ...

// klassenlose funktionen und daten


/**
 * Table of ground texts
 * @author Hj. Malthaner
 */
inthashtable_tpl <unsigned long, const char*> ground_texts;


/**
 * Pointer to the world of this ground. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_t * grund_t::welt = NULL;
bool grund_t::show_grid = false;
bool grund_t::underground_mode = false;

uint8 grund_t::offsets[4]={0,1,2/*illegal!*/,2};

ptrhashtable_tpl<grund_t*, grund_info_t*> grund_t::grund_infos;


void grund_t::entferne_grund_info()
{
	grund_infos.remove(this);
}



/**
 * Setzt den Beschreibungstext.
 * @param text Der neue Beschreibungstext.
 * @see grund_t::text
 * @author Hj. Malthaner
 */
void grund_t::setze_text(const char *text)
{
  // printf("Height %x\n", (pos.z - welt->gib_grundwasser())/Z_TILE_STEP);

  const unsigned long n =
    (pos.x << 19)
    + (pos.y << 6)
    + ((pos.z - welt->gib_grundwasser())/Z_TILE_STEP);

  const char * old = ground_texts.get(n);

  if(old != text) {
    ground_texts.remove(n);
    clear_flag(has_text);

    if(text) {
      ground_texts.put(n, text);
      set_flag(has_text);
    }

    set_flag(dirty);
    welt->setze_dirty();
  }
}



/**
 * Gibt eine Beschreibung des Untergrundes (informell) zurueck.
 * @return Einen Beschreibungstext zum Untergrund.
 * @author Hj. Malthaner
 */
const char* grund_t::gib_text() const
{
	const char * result = 0;
	if(flags & has_text) {
		const unsigned long n = (pos.x << 19) + (pos.y << 6) + ((pos.z - welt->gib_grundwasser())/Z_TILE_STEP);
		result = ground_texts.get(n);
	}
	return result;
}


grund_t::grund_t(karte_t *wl)
{
	welt = wl;
	flags = 0;
	bild_nr = IMG_LEER;
	back_bild_nr = 0;
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
	file->wr_obj_id(gib_typ());
	pos.rdwr(file);
//DBG_DEBUG("grund_t::rdwr()", "pos=%d,%d,%d", pos.x, pos.y, pos.z);

	if(file->is_saving()) {
		const char *text = gib_text();
		file->rdwr_str(text, "+");
	}
	else {
		const char *text = 0;
		file->rdwr_str(text, "+");
		setze_text(text);
	}

	if(file->get_version()<99007) {
		bool label;
		file->rdwr_bool(label, "\n");
		if(label) {
			dinge.add( new label_t(welt, pos, welt->gib_spieler(0), gib_text() ) );
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

						case track_wt: {
							schiene_t *sch = new schiene_t (welt, file);
							if(sch->gib_besch()->gib_wtyp()==monorail_wt) {
								dbg->warning("grund_t::rdwr()", "converting railroad to monorail at (%i,%i)",gib_pos().x, gib_pos().y);
								// compatibility code: Convert to monorail
								monorail_t *w= new monorail_t(welt);
								w->setze_besch(sch->gib_besch());
								w->setze_max_speed(sch->gib_max_speed());
								w->setze_ribi(sch->gib_ribi_unmasked());
								delete sch;
								weg = w;
							}
							else {
								weg = sch;
							}
						} break;

					case tram_wt:
						weg = new schiene_t (welt, file);
						if(weg->gib_besch()->gib_styp()!=weg_t::type_tram) {
							weg->setze_besch(wegbauer_t::weg_search(tram_wt,weg->gib_max_speed(),0,weg_t::type_tram));
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
							DBG_MESSAGE("grund_t::rdwr()","at (%i,%i) dock ignored",gib_pos().x, gib_pos().y);
						}
						break;

					case air_wt:
						weg = new runway_t (welt, file);
						break;
				}

				if(weg) {
					if(this->gib_typ()==fundament) {
						// remove this (but we can not correct the other wasy, since possibly not yet loaded)
						dbg->error("grund_t::rdwr()","removing way from foundation at %i,%i",pos.x,pos.y);
						// we do not delete them, to keep maitenance costs correct
					}
					else {
						assert((flags&has_way2)==0);	// maximum two ways on one tile ...
						weg->setze_pos(pos);
						if(besitzer_n!=-1) {
							weg->setze_besitzer(welt->gib_spieler(besitzer_n));
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
			obj_bei(0)->rdwr(file);
		}
		if(flags&has_way2) {
			obj_bei(1)->rdwr(file);
		}
		file->wr_obj_id(-1);   // Ende der Wege
	}

	// all objects on this tile
	dinge.rdwr(welt, file, gib_pos());

	// need to add a crossing for old games ...
	if(file->is_loading()  &&  ist_uebergang()  &&  !suche_obj_ab(ding_t::crossing,2)) {
		dinge.add( new crossing_t(welt, obj_bei(0)->gib_besitzer(), pos, ((weg_t *)obj_bei(0))->gib_waytype(), ((weg_t *)obj_bei(1))->gib_waytype() ) );
	}
}



grund_t::grund_t(karte_t *wl, koord3d pos)
{
	this->pos = pos;
	flags = 0;
	welt = wl;
	setze_bild(IMG_LEER);    // setzt   flags = dirty;
	back_bild_nr = 0;
}



grund_t::~grund_t()
{
//	dinge.dump();
	destroy_win(grund_infos.get(this));

	// remove text from table
	ground_texts.remove((pos.x << 16) + pos.y);

	dinge.loesche_alle(NULL,0);
	if(flags&is_halt_flag) {
		welt->lookup(pos.gib_2d())->gib_halt()->rem_grund(this);
	}
}



// moves all objects from the old to the new grund_t
void
grund_t::take_obj_from( grund_t *other_gr)
{
	// transfer all things
	while( other_gr->gib_top() ) {
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



void grund_t::info(cbuffer_t & buf) const
{
	if(flags&is_halt_flag) {
		welt->lookup(pos.gib_2d())->gib_halt()->info( buf );
	}

	if(!ist_wasser()) {
		if(flags&has_way1) {
			obj_bei(0)->info(buf);
		}
		if(flags&has_way2) {
			obj_bei(1)->info(buf);
		}
	}

#if 0
	if(buf.len() == 0) {
		uint8 hang= gib_grund_hang();
		buf.append(gib_hoehe());
		buf.append("\nslope: ");
		buf.append(hang);
		buf.append("\nback0: ");
		buf.append(gib_back_bild(0)-grund_besch_t::slopes->gib_bild(0));
		buf.append("\nback1: ");
		buf.append(gib_back_bild(1)-grund_besch_t::slopes->gib_bild(0));
		//    buf.append("\n");
		//    buf.append(translator::translate("Keine Info."));
	}
	buf.append("\nway slope");
	buf.append((int)gib_weg_hang());
	buf.append("\npos: ");
	buf.append(pos.x);
	buf.append(", ");
	buf.append(pos.y);
	buf.append(", ");
	buf.append(pos.z);

	buf.append("\n\n");
	buf.append(gib_name());
	if(gib_weg_ribi_unmasked(water_wt)) {
		buf.append("\nwater ribi: ");
		buf.append(gib_weg_ribi_unmasked(water_wt));
	}
#endif
}



/**
 * Manche Böden können zu Haltestellen gehören.
 * Der Zeiger auf die Haltestelle wird hiermit gesetzt.
 * @author Hj. Malthaner
 */
void grund_t::setze_halt(halthandle_t halt) {
//	this->halt = halt;
	if(halt.is_bound()) {
		flags |= is_halt_flag;
	}
	else {
		flags &= ~is_halt_flag;
	}
}



const halthandle_t
grund_t::gib_halt() const
{
	return (flags&is_halt_flag) ? welt->lookup(pos.gib_2d())->gib_halt() : halthandle_t();
}



/**
 * grund_t::calc_bild:
 *
 * Berechnet nur das Wegbild - den Rest bitte in der Ableitung setzen
 *
 * @author V. Meyer
 */
void grund_t::calc_bild()
{
	// will automatically recalculate ways ...
	dinge.calc_bild();
}



ribi_t::ribi grund_t::gib_weg_ribi(waytype_t typ) const
{
	weg_t *weg = gib_weg(typ);
	return (weg) ? weg->gib_ribi() : (ribi_t::ribi)ribi_t::keine;
}



ribi_t::ribi grund_t::gib_weg_ribi_unmasked(waytype_t typ) const
{
	weg_t *weg = gib_weg(typ);
	return (weg) ? weg->gib_ribi_unmasked() : (ribi_t::ribi)ribi_t::keine;
}



image_id
grund_t::gib_back_bild(int leftback) const
{
	if(back_bild_nr==0) {
		return IMG_LEER;
	}
	sint8 back_bild = abs(back_bild_nr);
	back_bild = leftback ? (back_bild/11)+11 : back_bild%11;
	if(back_bild_nr<0) {
		return grund_besch_t::fundament->gib_bild(back_bild);
	}
	else {
		return grund_besch_t::slopes->gib_bild(back_bild);
	}
}



// with double height ground tiles!
// can also happen with single height tiles
static inline uint8
get_backbild_from_diff( sint8 h1, sint8 h2 )
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
	sint8 back_bild_nr=0;
	sint8 is_building=0;
	bool fence_west=false, fence_north=false;
	const koord k = gib_pos().gib_2d();

	clear_flag(grund_t::draw_as_ding);
	if(((flags&has_way1)  &&  ((weg_t *)obj_bei(0))->gib_besch()->is_draw_as_ding())  ||  ((flags&has_way2)  &&  ((weg_t *)obj_bei(1))->gib_besch()->is_draw_as_ding())) {
		set_flag(grund_t::draw_as_ding);
	}
	bool	left_back_is_building = false;

	// check for foundation
	if(k.x>0  &&  k.y>0) {
		const grund_t *gr=welt->lookup(k+koord(-1,-1))->gib_kartenboden();
		if(gr) {
			const sint16 left_hgt=gr->gib_hoehe()/Z_TILE_STEP;
			const sint8 slope=gr->gib_grund_hang();

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
		const grund_t *gr=welt->lookup(k+koord(-1,0))->gib_kartenboden();
		if(gr) {
			const sint16 left_hgt=gr->gib_hoehe()/Z_TILE_STEP;
			const sint8 slope=gr->gib_grund_hang();

			const sint8 diff_from_ground_1 = left_hgt+corner2(slope)-hgt;
			const sint8 diff_from_ground_2 = left_hgt+corner3(slope)-hgt;
			// up slope hiding something ...
			if(diff_from_ground_1-corner1(slope_this)<0  ||  diff_from_ground_2-corner4(slope_this)<0)  {
				set_flag(grund_t::draw_as_ding);
				fence_west = true;
			}
			// any height difference AND something to see?
			if(  (diff_from_ground_1-corner1(slope_this)>0  ||  diff_from_ground_2-corner4(slope_this)>0)
				&&  (diff_from_ground_1>0  ||  diff_from_ground_2>0)  ) {
				back_bild_nr = get_backbild_from_diff( diff_from_ground_1, diff_from_ground_2 );
			}
			// avoid covering of slope by building ...
			if(left_back_is_building  &&  (back_bild_nr>0  ||  gr->gib_back_bild(1)!=IMG_LEER)) {
				set_flag(grund_t::draw_as_ding);
			}
			is_building = gr->gib_typ()==grund_t::fundament;
		}
	}

	// now enter the back two height differences
	if(k.y>0) {
		const grund_t *gr=welt->lookup(k+koord(0,-1))->gib_kartenboden();
		if(gr) {
			const sint16 back_hgt=gr->gib_hoehe()/Z_TILE_STEP;
			const sint8 slope=gr->gib_grund_hang();

			const sint8 diff_from_ground_1 = back_hgt+corner1(slope)-hgt;
			const sint8 diff_from_ground_2 = back_hgt+corner2(slope)-hgt;
			// up slope hiding something ...
			if(diff_from_ground_1-corner4(slope_this)<0  ||  diff_from_ground_2-corner3(slope_this)<0) {
				set_flag(grund_t::draw_as_ding);
				fence_north = true;
			}
			// any height difference AND something to see?
			if(  (diff_from_ground_1-corner4(slope_this)>0  ||  diff_from_ground_2-corner3(slope_this)>0)
				&&  (diff_from_ground_1>0  ||  diff_from_ground_2>0)  ) {
				back_bild_nr += get_backbild_from_diff( diff_from_ground_1, diff_from_ground_2 )*11;

			}
			is_building |= gr->gib_typ()==grund_t::fundament;
			// avoid covering of slope by building ...
			if(left_back_is_building  &&  (back_bild_nr>11  ||  gr->gib_back_bild(0)!=IMG_LEER)) {
				set_flag(grund_t::draw_as_ding);
			}
		}
	}

	// not ground -> then not draw first ...
	if(welt->lookup(k)->gib_kartenboden()!=this) {
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
		this->back_bild_nr = (gib_typ()==grund_t::fundament)? -back_bild_nr : back_bild_nr;
	}
}



PLAYER_COLOR_VAL
grund_t::text_farbe() const
{
	// if this gund belongs to a halt, the color should reflect the halt owner, not the grund owner!
	if(flags&is_halt_flag) {
		const spieler_t *sp=welt->lookup(pos.gib_2d())->gib_halt()->gib_besitzer();
		if(sp) {
			return PLAYER_FLAG|(sp->get_player_color1()+4);
		}
	}

	// else color according to current owner
	if(obj_bei(0)) {
		const spieler_t *sp = obj_bei(0)->gib_besitzer();
		if(sp) {
			return PLAYER_FLAG|(sp->get_player_color1()+4);
		}
	}

	return COL_WHITE;
}



void
grund_t::display_boden( const sint16 xpos, const sint16 ypos) const
{
	const bool dirty=get_flag(grund_t::dirty);
	const sint16 rasterweite=get_tile_raster_width();

	// walls
	if(back_bild_nr!=0) {
		if(abs(back_bild_nr)>121) {
			// fence before a drop
			if(back_bild_nr<0) {
				// behind a building
				display_img(grund_besch_t::fences->gib_bild(-back_bild_nr-122+3), xpos, ypos, dirty);
			}
			else {
				// on a normal tile
				display_img(grund_besch_t::fences->gib_bild(back_bild_nr-122), xpos, ypos, dirty);
			}
		}
		else {
			// artificial slope
			const sint8 back_bild2 = (abs(back_bild_nr)/11)+11;
			const sint8 back_bild1 = abs(back_bild_nr)%11;
			if(back_bild_nr<0) {
				// for a foundation
				display_img(grund_besch_t::fundament->gib_bild(back_bild1), xpos, ypos, dirty);
				display_img(grund_besch_t::fundament->gib_bild(back_bild2), xpos, ypos, dirty);
			}
			else {
				// natural
				display_img(grund_besch_t::slopes->gib_bild(back_bild1), xpos, ypos, dirty);
				display_img(grund_besch_t::slopes->gib_bild(back_bild2), xpos, ypos, dirty);
			}
		}
	}

	// ground
	image_id bild = gib_bild();
	if(bild==IMG_LEER) {
		// only check for forced redraw (of marked ... )
		if(dirty) {
			mark_rect_dirty_wc( xpos, ypos+rasterweite/2, xpos+rasterweite-1, ypos+rasterweite-1 );
		}
	}
	else {
		display_img(gib_bild(), xpos, ypos, dirty);

		// we show additionally a grid
		if((show_grid || underground_mode) &&  gib_typ()!=wasser) {
			uint8 hang = gib_grund_hang();
			uint8 back_hang = (hang&1) + ((hang>>1)&6);
			display_img(grund_besch_t::borders->gib_bild(back_hang), xpos, ypos, dirty);
		}
	}

	if(flags&has_way1) {
		sint16 ynpos = ypos-tile_raster_scale_y( gib_weg_yoff(), rasterweite );
		display_color_img(obj_bei(0)->gib_bild(), xpos, ynpos, obj_bei(0)->get_player_nr(), true, dirty);
		PLAYER_COLOR_VAL pc=obj_bei(0)->gib_outline_colour();
		if(pc) {
			display_img_blend(obj_bei(0)->gib_bild(), xpos, ynpos, pc, true, dirty);
		}
	}

	if(flags&has_way2){
		sint16 ynpos = ypos-tile_raster_scale_y( gib_weg_yoff(), rasterweite );
		display_color_img(obj_bei(1)->gib_bild(), xpos, ynpos, obj_bei(1)->get_player_nr(), true, dirty);
		PLAYER_COLOR_VAL pc=obj_bei(1)->gib_outline_colour();
		if(pc) {
			display_img_blend(obj_bei(1)->gib_bild(), xpos, ynpos, pc, true, dirty);
		}
	}
}



/* displays everything that is on a tile; is_global is true, if this is called during the whole screen update
 */
void
grund_t::display_dinge(const sint16 xpos, sint16 ypos, const bool is_global) const
{
	const bool dirty = get_flag(grund_t::dirty);
	const uint8 start_offset=offsets[flags/has_way1];

	if(!ist_im_tunnel()) {
		if(is_global  &&  get_flag(grund_t::marked)) {
			uint8 hang = gib_grund_hang();
			uint8 back_hang = (hang&1) + ((hang>>1)&6)+8;
			display_img(grund_besch_t::marker->gib_bild(back_hang), xpos, ypos, dirty);
			dinge.display_dinge( xpos, ypos, start_offset, is_global );
			display_img(grund_besch_t::marker->gib_bild(hang&7), xpos, ypos, dirty);
		}
		else {
			dinge.display_dinge( xpos, ypos, start_offset, is_global );
		}
	} else if(grund_t::underground_mode) {
		// only grid lines for underground mode ...
		uint8 hang = gib_grund_hang();
		uint8 back_hang = (hang&1) + ((hang>>1)&6);
		if(ist_karten_boden()) {
			display_img(grund_besch_t::borders->gib_bild(back_hang), xpos, ypos, dirty);
			if(gib_typ()==tunnelboden) {
				dinge.display_dinge( xpos, ypos, start_offset, is_global );
			}
		}
		if(get_flag(grund_t::marked)) {
			display_img(grund_besch_t::marker->gib_bild(back_hang)+8, xpos, ypos, dirty);
			display_img(grund_besch_t::marker->gib_bild(hang&7), xpos, ypos, dirty);
		}
	}

#ifdef SHOW_FORE_GRUND
	if(get_flag(grund_t::draw_as_ding)) {
		display_fillbox_wh_clip( xpos+raster_tile_width/2, ypos+(raster_tile_width*3)/4, 16, 16, 0, dirty);
	}
#endif
}



/* overlayer with signs, good levels and station coverage
 * reset_dirty clear the dirty bit (which marks the changed areas)
 * @author kierongreen
 */
void
grund_t::display_overlay(const sint16 xpos, const sint16 ypos, const bool reset_dirty)
{
	const bool dirty = get_flag(grund_t::dirty);

	// marker/station text
	const char * text = gib_text();
	if(text && (umgebung_t::show_names & 1)) {
		const sint16 raster_tile_width = get_tile_raster_width();
		const int width = proportional_string_width(text)+7;
		display_ddd_proportional_clip(xpos - (width - raster_tile_width)/2, ypos, width, 0, text_farbe(), COL_BLACK, text, dirty);
	}

	// display station waiting information/status
	if(flags&is_halt_flag  &&  (umgebung_t::show_names & 2)) {
		halthandle_t halt = welt->lookup(pos.gib_2d())->gib_halt();
		if(halt->gib_basis_pos3d()==pos) {
			halt->display_status(xpos, ypos);
		}
	}

#ifdef SHOW_FORE_GRUND
	if(get_flag(grund_t::draw_as_ding)) {
		display_fillbox_wh_clip( xpos+raster_tile_width/2, ypos+(raster_tile_width*3)/4, 16, 16, 0, dirty);
	}
#endif

	if(reset_dirty) {
		clear_flag(grund_t::dirty);
	}
}



/**
 * Bauhilfsfunktion - die ribis eines vorhandenen weges werden erweitert
 *
 * @return bool	    true, falls weg vorhanden
 * @param wegtyp    um welchen wegtyp geht es
 * @param ribi	    die neuen ribis
 *
 * @author V. Meyer
 */
bool grund_t::weg_erweitern(waytype_t wegtyp, ribi_t::ribi ribi)
{
	weg_t   *weg = gib_weg(wegtyp);
	if(weg) {
		weg->ribi_add(ribi);
		weg->count_sign();
		if(weg->is_electrified()) {
			for(uint8 i=0;  i<gib_top();  i++) {
				ding_t *d=obj_bei(i);
				if(d  &&  d->gib_typ()==ding_t::wayobj  &&  ((wayobj_t *)d)->gib_besch()->gib_wtyp()==wegtyp) {
					((wayobj_t *)d)->set_dir( ((wayobj_t *)d)->get_dir() | ribi );
				}
			}
		}
		calc_bild();
		return true;
	}
	return false;
}

/**
 * Bauhilfsfunktion - ein neuer weg wird mit den vorgegebenen ribis
 * eingetragen und der Grund dem Erbauer zugeordnet.
 *
 * @return bool	    true, falls weg nicht vorhanden war
 * @param weg	    der neue Weg
 * @param ribi	    die neuen ribis
 * @param sp	    Spieler, dem der Boden zugeordnet wird
 *
 * @author V. Meyer
 */
long grund_t::neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, spieler_t *sp)
{
	long cost=0;

	// not already there?
	const weg_t * alter_weg = gib_weg(weg->gib_waytype());
	if(alter_weg==NULL) {
		// ok, we are unique

		if((flags&has_way1)==0) {
			// new first way here

			// remove all trees ...
			while(1) {
				ding_t *d=dinge.suche(ding_t::baum,0);
				if(d==NULL) {
					break;
				}
				delete d;
				cost -= umgebung_t::cst_remove_tree;
			}

			// add
			weg->setze_ribi(ribi);
			weg->setze_pos(pos);
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
			weg->setze_ribi(ribi);
			weg->setze_pos(pos);
			flags |= has_way2;
			if(weg->gib_besch()->gib_styp()!=7) {
				// no tram => crossing needed!
				waytype_t w2 =  ((weg_t *)obj_bei( obj_bei(0)==weg ? 1 : 0 ))->gib_waytype();
				crossing_t *cr = new crossing_t(welt,sp,pos,w2,weg->gib_waytype());
				dinge.add( cr );
				cr->laden_abschliessen();
			}
		}

		// just add the cost
		if(sp && !ist_wasser()) {
			sp->add_maintenance(weg->gib_besch()->gib_wartung());
			weg->setze_besitzer( sp );
		}

		// may result in a crossing, but the wegebauer will recalc all images anyway
		weg->calc_bild();
//		dinge.dump();

		return cost;
	}
	return 0;
}


/**
 * Bauhilfsfunktion - einen Weg entfernen
 *
 * @return bool	    true, falls weg vorhanden war
 * @param wegtyp    um welchen wegtyp geht es
 * @param ribi_rem  sollen die ribis der nachbar zurückgesetzt werden?
 *
 * @author V. Meyer
 */
sint32 grund_t::weg_entfernen(waytype_t wegtyp, bool ribi_rem)
{
DBG_MESSAGE("grund_t::weg_entfernen()","this %p",this);
	weg_t *weg = gib_weg(wegtyp);
DBG_MESSAGE("grund_t::weg_entfernen()","weg %p",weg);
	if(weg!=NULL) {
		if(ribi_rem) {
			ribi_t::ribi ribi = weg->gib_ribi();
			grund_t *to;

			for(int r = 0; r < 4; r++) {
				if((ribi & ribi_t::nsow[r]) && get_neighbour(to, wegtyp, koord::nsow[r])) {
					weg_t *weg2 = to->gib_weg(wegtyp);
					if(weg2) {
						weg2->ribi_rem(ribi_t::rueckwaerts(ribi_t::nsow[r]));
						to->calc_bild();
					}
				}
			}
		}

		sint32 costs=weg->gib_besch()->gib_preis();	// costs for removal are construction costs
		weg->entferne( NULL );
		delete weg;

		// delete the second way ...
		if(flags&has_way2) {
			flags &= ~has_way2;

			// reset speed limit/crossing info (maybe altered by crossing)
			// Not all ways (i.e. with styp==7) will imply crossins, so wie hav to check
			crossing_t *cr = (crossing_t *)suche_obj_ab(ding_t::crossing,1);
			if(cr) {
				cr->entferne(0);
				delete cr;
				// restore speed limit
				((weg_t *)(obj_bei(0)))->setze_besch( ((weg_t *)(obj_bei(0)))->gib_besch() );
				((weg_t *)(obj_bei(0)))->count_sign();
			}
		}
		else {
			flags &= ~has_way1;
		}

		calc_bild();
		reliefkarte_t::gib_karte()->calc_map_pixel(gib_pos().gib_2d());

		return costs;
	}
	return 0;
}

bool grund_t::get_neighbour(grund_t *&to, waytype_t type, koord dir) const
{
	if(dir != koord::nord && dir != koord::sued && dir != koord::ost && dir != koord::west) {
		return false;
	}

	const planquadrat_t * plan = welt->lookup(pos.gib_2d() + dir);
	if(!plan) {
		return false;
	}

	// find ground in the same height: This work always!
	const sint16 this_height = get_vmove(dir);
	for( unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
		grund_t* gr = plan->gib_boden_bei(i);
		if(gr->get_vmove(-dir)==this_height) {
			// test, if connected
			if(!is_connected(gr, type, dir)) {
				continue;
			}
			to = gr;
			return true;
		}
	}
//	DBG_MESSAGE("grund_t::get_neighbour()","No ground connection from %i,%i,%i to dir %i,%i",pos.x,pos.y,pos.z,dir.x,dir.y);
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

	const int ribi1 = gib_weg_ribi_unmasked(wegtyp);
	const int ribi2 = gr->gib_weg_ribi_unmasked(wegtyp);
//	DBG_MESSAGE("grund_t::is_connected()","at (%i,%i,%i) ribi1=%i at (%i,%i,%i) ribi2=%i",pos.x,pos.y,pos.z,ribi1,gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z,ribi2);
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
int grund_t::get_vmove(koord dir) const
{
	const sint8 slope=gib_weg_hang();
	sint16 h=gib_hoehe();
	if(ist_bruecke()  &&  gib_grund_hang()!=0  &&  welt->lookup(pos)==this) {
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
	} else {
		// commented out: allow diagonal directions now (assume flat for these)
		//dbg->fatal("grund_t::get_vmove()","no valid direction given (%x)",ribi_typ(dir));	// error: not a direction ...
	}
	return h;
}



int
grund_t::get_max_speed() const
{
	int max = 0;
	if(flags&has_way1) {
		max = ((weg_t *)obj_bei(0))->gib_max_speed();
	}
	if(flags&has_way2) {
		max = min( max, ((weg_t *)obj_bei(0))->gib_max_speed() );
	}
	return max;
}



/* removes everything from a tile, including a halt but i.e. leave a powerline ond other stuff
 * @author prissi
 */
bool grund_t::remove_everything_from_way(spieler_t *sp,waytype_t wt,ribi_t::ribi rem)
{
//DBG_MESSAGE("grund_t::remove_everything()","at %d,%d,%d for waytype %i and ribi %i",pos.x,pos.y,pos.z,wt,rem);
	// check, if the way must be totally removed?
	weg_t *weg = gib_weg(wt);
	if(weg) {
		koord here = pos.gib_2d();

		// stopps
		if(flags&is_halt_flag  &&  gib_halt()->gib_besitzer()==sp) {
			const char *fail;
			if(!haltestelle_t::remove(welt, sp, pos, fail)) {
				return false;
			}
		}

		ribi_t::ribi add=(weg->gib_ribi_unmasked()&rem);
		sint32 costs = 0;

		for(uint8 i=0;  i<gib_top();  i++  ) {
			ding_t *d=obj_bei(i);
			// do not delete ways
			if(d->is_way()) {
				continue;
			}
			// roadsigns: check dir: dirs changed? delete
			if(d->gib_typ()==ding_t::roadsign  &&  ((roadsign_t *)d)->gib_besch()->gib_wtyp()==wt) {
				if((((roadsign_t *)d)->get_dir()&(~add))!=0) {
					costs -= ((roadsign_t *)d)->gib_besch()->gib_preis();
					delete d;
				}
			}
			// singal: not on crossings => remove all
			else if(d->gib_typ()==ding_t::signal  &&  ((roadsign_t *)d)->gib_besch()->gib_wtyp()==wt) {
				costs -= ((roadsign_t *)d)->gib_besch()->gib_preis();
				delete d;
			}
			// wayobj: check dir
			else if(d->gib_typ()==ding_t::wayobj  &&  ((wayobj_t *)d)->gib_besch()->gib_wtyp()==wt) {
				uint8 new_dir=((wayobj_t *)d)->get_dir()&add;
				if(new_dir) {
					// just change dir
					((wayobj_t *)d)->set_dir(new_dir);
				}
				else {
					costs -= ((wayobj_t *)d)->gib_besch()->gib_preis();
					delete d;
				}
			}
			// citycar or pedestrians: just delete
			else if(wt==road_wt  &&  (d->gib_typ()==ding_t::verkehr  ||  suche_obj(ding_t::fussgaenger))) {
				delete d;
			}
			// remove bridge
			else if(d->gib_typ()==ding_t::bruecke) {
				// last way was belonging to this bridge
				d->entferne(sp);
				delete d;
			}
			// remove tunnel portal, if not the last tile ...
			else if((add==ribi_t::keine  ||  ist_karten_boden())  &&  d->gib_typ()==ding_t::tunnel) {
				uint8 wt = ((tunnel_t *)d)->gib_besch()->gib_waytype();
				if((flags&has_way2)==0  &&  weg->gib_waytype()==wt) {
					// last way was belonging to this tunnel
					d->entferne(sp);
					delete d;
				}
				else {
					// we must leave the way to prevent destroying the other one
					add = gib_weg_nr(1)->gib_ribi_unmasked();
					weg->calc_bild();
				}
			}

		}


		// need to remove railblocks to recalcualte connections
		// remove all ways or just some?
		if(add==ribi_t::keine) {
			costs -= weg_entfernen(wt, true);
			if(add==ribi_t::keine  &&  (flags&is_kartenboden)  &&  gib_typ()==tunnelboden  &&  (flags&has_way1)==0) {
				// remove tunnelportal and set to normal ground
				obj_loesche_alle( sp );
				welt->access(pos.gib_2d())->kartenboden_setzen( new boden_t( welt, pos, slope ) );
			}
		}
		else {
	DBG_MESSAGE("wkz_wayremover()","change remaining way to ribi %d",add);
			// something will remain, we just change ribis
			weg_erweitern(wt, add);
			calc_bild();
		}
		// we have to pay?
		if(costs) {
			sp->buche(costs, here, COST_CONSTRUCTION);
		}
	}
	return true;
}
