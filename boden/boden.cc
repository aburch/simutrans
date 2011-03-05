/*
 * Natur-Untergrund für Simutrans.
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simhalt.h"
#include "../simwin.h"
#include "../simskin.h"
#include "../simintr.h"

#include "../dings/baum.h"

#include "../gui/ground_info.h"
#include "../gui/karte.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/loadsave.h"

#include "boden.h"
#include "wege/strasse.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../simtools.h"
#include "../simimg.h"

#include "../tpl/ptrhashtable_tpl.h"


boden_t::boden_t(karte_t *welt, loadsave_t *file, koord pos ) : grund_t( welt, koord3d(pos,0) )
{
	grund_t::rdwr( file );

	// restoring trees (disadvantage: loosing offsets but much smaller savegame footprint)
	if(  file->get_version()>=110001  ) {
		sint16 id = file->rd_obj_id();
		while(  id!=-1  ) {
			sint32 age;
			file->rdwr_long( age );
			// check, if we still have this tree ... (if there are not trees, the first index is NULL!)
			if(  baum_t::get_anzahl_besch()>id  &&  (*(baum_t::get_all_besch()))[id]!=NULL  ) {
				baum_t *tree = new baum_t( welt, get_pos(), (uint16)id, age, slope );
				dinge.add( tree );
			}
			else {
				dbg->warning( "boden_t::boden_t()", "Could not restore tree type %i at (%s)", id, pos.get_str() );
			}
			// check for next tree
			id = file->rd_obj_id();
		}
	}
}


boden_t::boden_t(karte_t *welt, koord3d pos, hang_t::typ sl) : grund_t(welt, pos)
{
	slope = sl;
}


// only for more compact saving of trees
void boden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(  file->get_version()>=110001  ) {
		if(  umgebung_t::networkmode  &&  !hat_wege()  ) {
			for(  uint8 i=0;  i<dinge.get_top();  i++  ) {
				ding_t *d = dinge.bei(i);
				if(  d->get_typ()==ding_t::baum  ) {
					baum_t *tree = (baum_t *)d;
					file->wr_obj_id( tree->get_besch_id() );
					sint32 age = tree->get_age();
					file->rdwr_long( age );
				}
			}
		}
		file->wr_obj_id( -1 );
	}
}


const char *boden_t::get_name() const
{
	if(ist_uebergang()) {
		return "Kreuzung";
	} else if(hat_wege()) {
		return get_weg_nr(0)->get_name();
	}
	else {
		return "Boden";
	}
}


void boden_t::calc_bild_internal()
{
		uint8 slope_this =  get_disp_slope();
		weg_t *weg = get_weg(road_wt);

#ifndef DOUBLE_GROUNDS

		if (is_visible()) {
			if(weg  &&  weg->hat_gehweg()) {
			    if(get_hoehe() >= welt->get_snowline()  &&  skinverwaltung_t::fussweg->get_bild_nr(slope_this+1)!=IMG_LEER) {
			        // snow images
			        set_bild(skinverwaltung_t::fussweg->get_bild_nr(slope_this+1));
			    }
			    else if(slope_this!=0  &&  get_hoehe() == welt->get_snowline()-1  &&  skinverwaltung_t::fussweg->get_bild_nr(slope_this+2)!=IMG_LEER) {
			        // transition images
			        set_bild(skinverwaltung_t::fussweg->get_bild_nr(slope_this+2));
			    }
			    else {
			        set_bild(skinverwaltung_t::fussweg->get_bild_nr(slope_this));
			    }
			}
			else {
				set_bild(grund_besch_t::get_ground_tile(slope_this,get_disp_height()) );
			}
		}
		else
		{
			set_bild(IMG_LEER);
		}
#else
		if (weg && weg->hat_gehweg()) {
			set_bild(skinverwaltung_t::fussweg->get_bild_nr(grund_besch_t::slopetable[slope_this]));
		}
		else {
			set_bild( grund_besch_t::get_ground_tile(slope_this,get_hoehe() ) );
		}
#endif
		grund_t::calc_back_bild(get_disp_height()/Z_TILE_STEP,slope_this);
}
