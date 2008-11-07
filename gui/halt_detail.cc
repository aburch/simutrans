/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simware.h"
#include "../simfab.h"
#include "../simhalt.h"
#include "../simline.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"

#include "../dataobj/warenziel.h"
#include "../dataobj/translator.h"

#include "halt_detail.h"


halt_detail_t::halt_detail_t(halthandle_t halt_) :
	gui_frame_t(translator::translate("Details"), halt_->gib_besitzer()),
	halt(halt_),
	scrolly(&cont),
	txt_info(" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"),
	cb_info_buffer(8192)
{
	cont.add_komponente(&txt_info);

	// fill buffer with halt detail
	halt_detail_info(cb_info_buffer);
	txt_info.setze_text(cb_info_buffer);
	txt_info.setze_pos(koord(10,10));

	// calc window size
	const koord size = txt_info.gib_groesse();
	if (size.y < 400) {
		setze_fenstergroesse(koord(300, size.y + 32));
	} else {
		setze_fenstergroesse(koord(300, 400));
	}

	// add scrollbar
	scrolly.setze_pos(koord(1, 1));
	scrolly.setze_groesse(gib_fenstergroesse() + koord(-1, -17));

	add_komponente(&scrolly);
}



halt_detail_t::~halt_detail_t()
{
	while(!posbuttons.empty()) {
		button_t *b = posbuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
}



void halt_detail_t::halt_detail_info(cbuffer_t & buf)
{
	if (!halt.is_bound()) {
		return;
	}

	while(!posbuttons.empty()) {
		button_t *b = posbuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	buf.clear();

	const slist_tpl<fabrik_t *> & fab_list = halt->gib_fab_list();
	slist_tpl<const ware_besch_t *> nimmt_an;

	sint16 offset_y = 20;
	buf.append(translator::translate("Fabrikanschluss"));
	buf.append(":\n");
	offset_y += LINESPACE;

	if (!fab_list.empty()) {

		slist_iterator_tpl<fabrik_t *> fab_iter(fab_list);

		while(fab_iter.next()) {
			const fabrik_t * fab = fab_iter.get_current();
			const koord pos = fab->gib_pos().gib_2d();

			// target button ...
			button_t *pb = new button_t();
			pb->init( button_t::posbutton, NULL, koord(10, offset_y) );
			pb->setze_targetpos( pos );
			pb->add_listener( this );
			posbuttons.append( pb );
			cont.add_komponente( pb );

			buf.append("   ");
			buf.append(translator::translate(fab->gib_name()));
			buf.append(" (");
			buf.append(pos.x);
			buf.append(", ");
			buf.append(pos.y);
			buf.append(")\n");
			offset_y += LINESPACE;

			const vector_tpl<ware_production_t>& eingang = fab->gib_eingang();
			for (uint32 i = 0; i < eingang.get_count(); i++) {
				const ware_besch_t* ware = eingang[i].gib_typ();

				if(!nimmt_an.contains(ware)) {
					nimmt_an.append(ware);
				}
			}
		}
	} else {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
		offset_y += LINESPACE;
	}

	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Angenommene Waren"));
	buf.append(":\n");
	offset_y += LINESPACE;

	if (!nimmt_an.empty()) {
		for(uint32 i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
			const ware_besch_t *ware = warenbauer_t::gib_info(i);
			if(nimmt_an.contains(ware)) {

				buf.append(" ");
				buf.append(translator::translate(ware->gib_name()));
				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	} else {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
		offset_y += LINESPACE;
	}

	// add lines that serve this stop
	buf.append("\n");
	offset_y += LINESPACE;

	if (!halt->registered_lines.empty()) {
		buf.append(translator::translate("Lines serving this stop"));
		buf.append(":\n");
		offset_y += LINESPACE;

		for (unsigned int i = 0; i<halt->registered_lines.get_count(); i++) {
			buf.append(" ");
			buf.append(halt->registered_lines[i]->get_name());
			buf.append("\n");
			offset_y += LINESPACE;
		}
	}

	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Direkt erreichbare Haltestellen"));
	buf.append("\n");
	offset_y += LINESPACE;

	bool has_stops = false;

	if(!halt->gib_warenziele_passenger()->empty()) {
		buf.append("\n");
		offset_y += LINESPACE;

		buf.append(" ·");
		buf.append(translator::translate(warenbauer_t::passagiere->gib_name()));
		buf.append(":\n");
		offset_y += LINESPACE;

		const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele_passenger();
		slist_iterator_tpl<warenziel_t> iter (ziele);
		while(iter.next()) {
			warenziel_t wz = iter.get_current();
			halthandle_t a_halt = wz.gib_zielhalt();
			if(a_halt.is_bound()) {

				has_stops = true;

				buf.append("   ");
				buf.append(a_halt->gib_name());

				// target button ...
				button_t *pb = new button_t();
				pb->init( button_t::posbutton, NULL, koord(10, offset_y) );
				pb->setze_targetpos( a_halt->gib_basis_pos() );
				pb->add_listener( this );
				posbuttons.append( pb );
				cont.add_komponente( pb );

				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	}

	if(!halt->gib_warenziele_mail()->empty()) {
		buf.append("\n");
		offset_y += LINESPACE;

		buf.append(" ·");
		buf.append(translator::translate(warenbauer_t::post->gib_name()));
		buf.append(":\n");
		offset_y += LINESPACE;

		const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele_mail();
		slist_iterator_tpl<warenziel_t> iter (ziele);
		while(iter.next()) {
			warenziel_t wz = iter.get_current();
			halthandle_t a_halt = wz.gib_zielhalt();
			if(a_halt.is_bound()) {

				has_stops = true;

				buf.append("   ");
				buf.append(a_halt->gib_name());

				// target button ...
				button_t *pb = new button_t();
				pb->init( button_t::posbutton, NULL, koord(10, offset_y) );
				pb->setze_targetpos( a_halt->gib_basis_pos() );
				pb->add_listener( this );
				posbuttons.append( pb );
				cont.add_komponente( pb );

				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	}

	buf.append("\n\n");
	offset_y += LINESPACE;
	offset_y += LINESPACE;
	if(!halt->gib_warenziele_freight()->empty()) {

		bool first = true;

		const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele_freight();
		slist_iterator_tpl<warenziel_t> iter (ziele);
		while(iter.next()) {
				warenziel_t wz = iter.get_current();
				halthandle_t a_halt = wz.gib_zielhalt();
				if(a_halt.is_bound()) {

					has_stops = true;

					buf.append("   ");
					buf.append(a_halt->gib_name());

					// target button ...
					button_t *pb = new button_t();
					pb->init( button_t::posbutton, NULL, koord(10, offset_y) );
					pb->setze_targetpos( a_halt->gib_basis_pos() );
					pb->add_listener( this );
					posbuttons.append( pb );
					cont.add_komponente( pb );

					buf.append("\n");
					offset_y += LINESPACE;

					first = true;
					for(uint32 i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
						const ware_besch_t *ware = warenbauer_t::gib_info(i);

						if(wz.gib_catg_index()==ware->gib_catg_index()) {

						if(first) {
							buf.append(" ·");
							buf.append(translator::translate(ware->gib_name()));
							first = false;

						} else {
							buf.append(", ");
							buf.append(translator::translate(ware->gib_name()));
						}
					}
				}

				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	}

	if (!has_stops) {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
	}
	buf.append("\n\n");

	txt_info.setze_text(buf);
	txt_info.recalc_size();
	cont.setze_groesse( txt_info.gib_groesse() );

	// ok, we have now this counter for pending updates
	destination_counter = halt->get_rebuild_destination_counter();
}



bool halt_detail_t::action_triggered(gui_komponente_t *, value_t extra)
{
	if(extra.i&~1) {
		koord k = *(const koord *)extra.p;
		halt->get_welt()->change_world_position( koord3d(k,halt->get_welt()->max_hgt(k)) );
	}
	return true;
}



void halt_detail_t::zeichnen(koord pos, koord gr)
{
	if(halt.is_bound()) {
		if(halt->get_rebuild_destination_counter()!=destination_counter) {
			// fill buffer with halt detail
			halt_detail_info(cb_info_buffer);
			txt_info.setze_text(cb_info_buffer);
		}
	}
	gui_frame_t::zeichnen( pos, gr );
}
