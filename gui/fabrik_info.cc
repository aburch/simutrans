/*
 * fabrik_info.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "fabrik_info.h"

#include "components/gui_button.h"
#include "help_frame.h"

#include "../simfab.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simcity.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../dataobj/translator.h"


fabrik_info_t::fabrik_info_t(fabrik_t *fab, gebaeude_t *gb, karte_t *welt) :
  gui_frame_t(fab->gib_name(),fab->gib_besitzer()->get_player_color()),
  ding_info_t(gb),
  view(welt, gb->gib_pos()),
  scrolly(&cont),
  txt("\n"),
  info_buf(8192)
{
  unsigned int i;

  this->fab = fab;
  this->welt = welt;
  this->about = 0;

  txt.setze_pos(koord(16,-15));
  txt.setze_text(info_buf);
  cont.add_komponente(&txt);


  const vector_tpl <koord> & lieferziele =  fab->gib_lieferziele();
#ifdef _MSC_VER
  // V.Meyer: MFC has a bug with "new x[0]"
	lieferbuttons = new button_t [MAX(1, lieferziele.get_count())];
#else
  lieferbuttons = new button_t [lieferziele.get_count()];
#endif

  for(i=0; i<lieferziele.get_count(); i++) {
    lieferbuttons[i].setze_pos(koord(16, 50+i*11));
    lieferbuttons[i].setze_typ(button_t::arrowright);

    lieferbuttons[i].add_listener(this);

    cont.add_komponente(&lieferbuttons[i]);
  }

  int y_off = lieferziele.get_count() ? (int)lieferziele.get_count()*11-11 : -33;

  const vector_tpl <koord> & suppliers =  fab->get_suppliers();
#ifdef _MSC_VER
  // V.Meyer: MFC has a bug with "new x[0]"
  supplierbuttons = new button_t [MAX(1, suppliers.get_count())];
#else
  supplierbuttons = new button_t [suppliers.get_count()];
#endif
  for(i=0; i<suppliers.get_count(); i++) {
    supplierbuttons[i].setze_pos(koord(16, 83+y_off+i*11));
    supplierbuttons[i].setze_typ(button_t::arrowright);
    supplierbuttons[i].add_listener(this);
    cont.add_komponente(&supplierbuttons[i]);
  }

  int yy_off = suppliers.get_count() ? (int)suppliers.get_count()*11-11 : -33;
  y_off += yy_off;


  const slist_tpl <stadt_t *> & arbeiterziele = fab->gib_arbeiterziele();

#ifdef _MSC_VER
  // V.Meyer: MFC has a bug with "new x[0]"
  stadtbuttons = new button_t [MAX(1, arbeiterziele.count())];
#else
  stadtbuttons = new button_t [arbeiterziele.count()];
#endif

  for(i=0; i<arbeiterziele.count(); i++) {
    stadtbuttons[i].setze_pos(koord(16, 116+y_off+i*11));
    stadtbuttons[i].setze_typ(button_t::arrowright);

    stadtbuttons[i].add_listener(this);

    cont.add_komponente(&stadtbuttons[i]);
  }


  setze_opaque(true);

  // Hajo: "About" button only if translation is available

  char key[256];
  sprintf(key, "factory_%s_details", fab->gib_name());

  const char * value = translator::translate(key);

  if(value && *value != 'f') {
    about = new button_t();

    about->init(button_t::roundbox,
		translator::translate("About"),
		koord(266 - 64, 6+64+10),
		koord(64, 14));

    about->add_listener(this);
    add_komponente(about);
  }

  fab->info(info_buf);
  const short height = MAX(count_char(info_buf, '\n')*LINESPACE+40, 92);

  setze_fenstergroesse(koord((short)290, MIN(height, 408)));
  cont.setze_groesse(koord((short)290, height-20));

  scrolly.setze_groesse(gib_fenstergroesse()-koord(1, 8));
  scrolly.setze_pos(koord(1,1));
  scrolly.set_show_scroll_x(false);
  scrolly.set_read_only(true);
  add_komponente(&scrolly);

  view.setze_pos(koord(266 - 64, 8));
  add_komponente(&view);

}


fabrik_info_t::~fabrik_info_t()
{
  delete [] lieferbuttons;
  lieferbuttons = 0;

  delete [] stadtbuttons;
  stadtbuttons = 0;

  delete about;
  about = 0;
}


/*
 * Für den Aufruf der richtigen Methoden sorgen!
 */
const char * fabrik_info_t::gib_name() const {
  return gui_frame_t::gib_name();
}



/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 *
 * @author Hj. Malthaner
 */
void fabrik_info_t::zeichnen(koord pos, koord gr)
{
	info_buf.clear();
	fab->info(info_buf);

	gui_frame_t::zeichnen(pos, gr);

	unsigned indikatorfarbe = fabrik_t::status_to_color[ fab->calc_factory_status( NULL, NULL ) ];
	display_ddd_box_clip(pos.x + view.pos.x, pos.y + view.pos.y + 75, 64, 8, MN_GREY0, MN_GREY4);
	display_fillbox_wh_clip(pos.x + view.pos.x+1, pos.y + view.pos.y + 76, 62, 6, indikatorfarbe, true);
	if (fab->get_prodfaktor() > 16) {
		display_color_img(skinverwaltung_t::electricity->gib_bild_nr(0), pos.x + view.pos.x+4, pos.y + view.pos.y+18, 0, false, false);
	}
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool fabrik_info_t::action_triggered(gui_komponente_t *komp)
{
    unsigned int i;

    for(i=0; i<fab->gib_lieferziele().get_count(); i++) {
	if(komp == &lieferbuttons[i]) {
	    welt->zentriere_auf(fab->gib_lieferziele().get(i));
	}
    }

    for(i=0; i<fab->get_suppliers().get_count(); i++) {
	if(komp == &supplierbuttons[i]) {
	    welt->zentriere_auf(fab->get_suppliers().get(i));
	}
    }

    for(i=0; i<fab->gib_arbeiterziele().count(); i++) {
	if(komp == &stadtbuttons[i]) {
	    welt->zentriere_auf(fab->gib_arbeiterziele().at(i)->gib_pos());
	}
    }

    if(komp == about) {
      help_frame_t * frame = new help_frame_t();

      char key[256];
      sprintf(key, "factory_%s_details", fab->gib_name());

      frame->setze_text(translator::translate(key));

      create_win(frame, w_autodelete, magic_none);
    }

    return true;
}
