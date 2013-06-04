#include <string.h>

#include "tunnelboden.h"

#include "../simworld.h"
#include "../simskin.h"

#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../dings/tunnel.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"
#include "../besch/tunnel_besch.h"

#include "../utils/cbuffer_t.h"


tunnelboden_t::tunnelboden_t(karte_t *welt, loadsave_t *file, koord pos ) : boden_t(welt, koord3d(pos,0), 0)
{
	rdwr(file);

	// some versions had tunnel without tunnel objects
	if (!find<tunnel_t>()) {
		// then we must spawn it here (a way MUST be always present, or the savegame is completely broken!)
		weg_t *weg=(weg_t *)obj_bei(0);
		obj_add(new tunnel_t(welt, get_pos(), weg->get_besitzer(), tunnelbauer_t::find_tunnel(weg->get_besch()->get_wtyp(), 450, 0)));
		DBG_MESSAGE("tunnelboden_t::tunnelboden_t()","added tunnel to pos (%i,%i,%i)",get_pos().x, get_pos().y,get_pos().z);
	}
}


void tunnelboden_t::calc_bild_internal()
{
	// tunnel mouth
	if (ist_karten_boden()) {
		if (grund_t::underground_mode==grund_t::ugm_all || (grund_t::underground_mode==grund_t::ugm_level && pos.z==grund_t::underground_level)) {
			if (grund_t::underground_mode==grund_t::ugm_all) {
				clear_back_bild();
			}
			else {
				boden_t::calc_bild_internal();
			}
			// default tunnel ground images
			set_bild(skinverwaltung_t::tunnel_texture->get_bild_nr(0));
			clear_flag(draw_as_ding);
		}
		else {
			// calculate the ground
			boden_t::calc_bild_internal();
			set_flag(draw_as_ding);
		}

		if(  grund_t::underground_mode == grund_t::ugm_none  ) {
			if(  (get_grund_hang() == hang_t::west * umgebung_t::pak_height_conversion_factor  &&  abs(back_bild_nr) > 11)  ||  (get_grund_hang() == hang_t::nord * umgebung_t::pak_height_conversion_factor  &&  get_back_bild(0) != IMG_LEER)  ) {
				// must draw as ding, since there is a slope here nearby
				koord pos = get_pos().get_2d() + koord(get_grund_hang());
				grund_t *gr = welt->lookup_kartenboden(pos);
				gr->set_flag(grund_t::draw_as_ding);
			}
		}
	}
	// inside tunnel
	else {
		clear_back_bild();
		if (is_visible()) {
			// default tunnel ground images
			// single or double slope? (single slopes are not divisible by 8)
			const uint8 slope_this =  get_disp_slope();
			const uint8 bild_nr = (!slope_this  ||  (slope_this & 7)) ? grund_besch_t::slopetable[slope_this] : grund_besch_t::slopetable[slope_this >> 1] + 12;
			set_bild( skinverwaltung_t::tunnel_texture->get_bild_nr( bild_nr ) );
		}
		else {
			set_bild(IMG_LEER);
		}
	}
}


void tunnelboden_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "tunnelboden_t" );

	grund_t::rdwr(file);

	if(file->get_version()<88009) {
		uint32 int_hang = slope;
		file->rdwr_long(int_hang);
		slope = int_hang;
	}

	// only 99.03 version save the tunnel here
	if(file->get_version()==99003) {
		char  buf[256];
		const tunnel_besch_t *besch = NULL;
		file->rdwr_str(buf, lengthof(buf));
		if (find<tunnel_t>() == NULL) {
			besch = tunnelbauer_t::get_besch(buf);
			if(besch) {
				obj_add(new tunnel_t(welt, get_pos(), obj_bei(0)->get_besitzer(), besch));
			}
		}
	}
}


void tunnelboden_t::info(cbuffer_t & buf) const
{
	const tunnel_t *tunnel = find<tunnel_t>();
	if(tunnel  &&  tunnel->get_besch()) {
		const tunnel_besch_t *besch = tunnel->get_besch();
		buf.append(translator::translate(besch->get_name()));
		buf.append("\n");
	}
	boden_t::info(buf);
}
