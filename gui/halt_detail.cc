/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>

#include "../simworld.h"
#include "../simplay.h"
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


halt_detail_t::halt_detail_t(spieler_t * sp, karte_t *welt, halthandle_t halt) :
	gui_frame_t(translator::translate("Details"), sp),
	scrolly(&txt_info),
	txt_info(" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"),
	cb_info_buffer(8192)
{
	this->halt = halt;
	this->welt = welt;

	// fill buffer with halt detail
	cb_info_buffer.clear();
	halt_detail_info(cb_info_buffer);
	txt_info.setze_text(cb_info_buffer);

	txt_info.setze_pos(koord(10,10));

	// calc window size
	setze_opaque(true);
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

void halt_detail_t::halt_detail_info(cbuffer_t & buf)
{
	if (!halt.is_bound()) {
		return;
	}

	const slist_tpl<fabrik_t *> & fab_list = halt->gib_fab_list();
	slist_tpl<const ware_besch_t *> nimmt_an;

	buf.append(translator::translate("Fabrikanschluss"));
	buf.append(":\n");

	if (!fab_list.empty()) {

		slist_iterator_tpl<fabrik_t *> fab_iter(fab_list);

		while(fab_iter.next()) {
			const fabrik_t * fab = fab_iter.get_current();
			const koord pos = fab->gib_pos().gib_2d();

			buf.append(" ");
			buf.append(translator::translate(fab->gib_name()));
			buf.append(" (");
			buf.append(pos.x);
			buf.append(", ");
			buf.append(pos.y);
			buf.append(")\n");

			const vector_tpl<ware_production_t>& eingang = fab->gib_eingang();
			for (uint32 i = 0; i < eingang.get_count(); i++) {
				const ware_besch_t* ware = eingang[i].gib_typ();

				if(!nimmt_an.contains(ware)) {
					nimmt_an.append(ware);
				}
			}
		}
	} else {
		DBG_MESSAGE("else", "");
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
	}

	buf.append("\n");
	buf.append(translator::translate("Angenommene Waren"));
	buf.append(":\n");


	if (!nimmt_an.empty()) {
		for(uint32 i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
			const ware_besch_t *ware = warenbauer_t::gib_info(i);
			if(nimmt_an.contains(ware)) {
				buf.append(" ");
				buf.append(translator::translate(ware->gib_name()));
				buf.append("\n");
			}
		}
	} else {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
	}

	// add lines that serve this stop
	buf.append("\n");
	if (!halt->registered_lines.empty()) {
		buf.append(translator::translate("Lines serving this stop"));
		buf.append(":\n");

		for (unsigned int i = 0; i<halt->registered_lines.get_count(); i++) {
			buf.append(" ");
			buf.append(halt->registered_lines[i]->get_name());
			buf.append("\n");
		}
	}

	buf.append("\n");
	buf.append(translator::translate("Direkt erreichbare Haltestellen"));
	buf.append(":\n");

	const slist_tpl<warenziel_t> *ziele = halt->gib_warenziele();
	slist_iterator_tpl<warenziel_t> iter (ziele);

	while(iter.next()) {
		warenziel_t wz = iter.get_current();
		halthandle_t a_halt = halt->gib_halt(welt,wz.gib_ziel());
		if(a_halt.is_bound()) {

			buf.append(" ");
			buf.append(a_halt->gib_name());
			buf.append(":\n");

			bool first = true;

			for(uint32 i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
				const ware_besch_t *ware = warenbauer_t::gib_info(i);

				if(wz.gib_typ()->is_interchangeable(ware)) {

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
		}
	}

	if (ziele->empty()) {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
	}
	buf.append("\n\n");
}



void
halt_detail_t::zeichnen(koord pos, koord gr)
{
	// fill buffer with halt detail
	cb_info_buffer.clear();
	halt_detail_info(cb_info_buffer);
	txt_info.setze_text(cb_info_buffer);
	gui_frame_t::zeichnen(pos,gr);
}
