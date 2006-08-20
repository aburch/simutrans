/*
 * message_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "button.h"

#include "../simwin.h"

#include "../dataobj/translator.h"
#include "message_frame_t.h"
#include "message_stats_t.h"
#include "message_option_t.h"
#include "gui_scrollpane.h"

#include "help_frame.h"

#include "ifc/action_listener.h"

message_frame_t::message_frame_t(karte_t *welt) : gui_frame_t("Mailbox")
{
  message_frame_t::welt = welt;
  stats = new message_stats_t(welt);
  scrolly = new gui_scrollpane_t(stats);
  scrolly->setze_pos(koord(0, 0));
  add_komponente(scrolly);

  option_bt = new button_t();
  option_bt->pos = koord(8,0);
  option_bt->groesse = koord(80,14);
  option_bt->setze_typ(button_t::box);
  option_bt->text = translator::translate("Optionen");
  option_bt->add_listener(this);
  add_komponente(option_bt);

  setze_fenstergroesse(koord(320, 240));
  // a min-size for the window
  set_min_windowsize(koord(320, 80));

  set_resizemode(diagonal_resize);
  resize(koord(0,0));
  setze_opaque(true);
}



message_frame_t::~message_frame_t()
{
	delete option_bt;
	option_bt = 0;
	delete scrolly;
	scrolly = 0;
	delete stats;
	stats = 0;
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void message_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	koord groesse = gib_fenstergroesse()-koord(0,16);
	scrolly->setze_groesse(groesse);
}




 /* triggered, when button clicked */
bool
message_frame_t::action_triggered(gui_komponente_t *komp)
{
	create_win(320,200, new message_option_t(welt), w_info );
	return true;
}
