/*
 * schedule_list.cc
 *
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <stdio.h>

#include "messagebox.h"
#include "schedule_list.h"
#include "scrolled_list.h"
#include "schedule_gui.h"
#include "line_management_gui.h"
#include "gui_convoiinfo.h"

#include "../simcolor.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simconvoi.h"
#include "../simvehikel.h"
#include "../simwin.h"
#include "../simlinemgmt.h"
#include "../utils/simstring.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/linie.h"
#include "components/gui_chart.h"
#include "halt_list_item.h"


const char schedule_list_gui_t::cost_type[MAX_LINE_COST][64] =
{
  "Free Capacity", "Transported Goods", "Convoys", "Revenue", "Operation", "Profit"
};

const int schedule_list_gui_t::cost_type_color[MAX_LINE_COST] =
{
  7, 35, 15, 132, 23, 27
};


// Hajo: 17-Jan-04: changed layout to make components fit into
// a width of 400 pixels -> original size was unuseable in 640x480

schedule_list_gui_t::schedule_list_gui_t(karte_t *welt)
 : gui_frame_t("Line Management"), scrolly(&cont), scrolly_haltestellen(&cont_haltestellen)
{
  SCL_HEIGHT = 170; // to enlarge scl window, increase this value!

  this->welt = welt;
  line = NULL;
  capacity = load = 0;
  selection = -1;
  loadfactor = 0;

  groesse = koord(400, 440);
  button_t button_def;

  // Hajo: width of a column - the window has two columns
  const int COLWIDTH = groesse.x/2 - 22;

  // Hajo: X Position of second column
  const int COL2XPOS = groesse.x/2 + 11;

  // editable line name
  inp_name.add_listener(this);
  inp_name.setze_pos(koord(groesse.x/2 + 11, 193));
  inp_name.setze_groesse(koord(COLWIDTH, 14));
  inp_name.set_visible(false);
  add_komponente(&inp_name);


  // load display
  filled_bar.add_color_value(&loadfactor, GREEN);
  filled_bar.set_visible(false);
  add_komponente(&filled_bar);

  // init scrolled list
  scl = new scrolled_list_gui_t(scrolled_list_gui_t::select);
  scl->setze_groesse(koord(COLWIDTH, SCL_HEIGHT));
  scl->setze_pos(koord(0,1));
  scl->setze_highlight_color(gib_besitzer()->kennfarbe+1);

  // tab panel
  tabs.setze_pos(koord(11,5));
  tabs.setze_groesse(koord(COLWIDTH, SCL_HEIGHT));

  tabs.add_tab(scl, translator::translate("All"));
  tabs.add_tab(scl, translator::translate("Truck"));
  tabs.add_tab(scl, translator::translate("Train"));
  tabs.add_tab(scl, translator::translate("Ship"));
  tabs.add_listener(this);
  add_komponente(&tabs);

  welt->simlinemgmt->sort_lines();
  build_line_list(0);

  cont.setze_groesse(koord(500, 40));

  scrolly.setze_groesse(koord(groesse.x/2,
			      gib_fenstergroesse().y - 240 ));
  scrolly.set_visible(false);
  add_komponente(&scrolly);
  setze_opaque(true);

  cont_haltestellen.setze_groesse(koord(500, 40));
  scrolly_haltestellen.setze_pos(koord(0, 50 + SCL_HEIGHT));
  scrolly_haltestellen.setze_groesse(koord(groesse.x/2,
					   gib_fenstergroesse().y - SCL_HEIGHT - 50 ));
  scrolly_haltestellen.set_visible(false);
  add_komponente(&scrolly_haltestellen);
  setze_opaque(true);


  // normal buttons edit new remove
  bt_new_line.setze_pos(koord(10, 18 + SCL_HEIGHT));
  bt_new_line.setze_groesse(koord(92,14));
  bt_new_line.setze_typ(button_t::roundbox);
  bt_new_line.text = translator::translate("New Line");
  add_komponente(&bt_new_line);
  bt_new_line.add_listener(this);

  bt_change_line.setze_pos(koord(102, 18 + SCL_HEIGHT));
  bt_change_line.setze_groesse(koord(92,14));
  bt_change_line.setze_typ(button_t::roundbox);
  bt_change_line.text = translator::translate("Update Line");
  add_komponente(&bt_change_line);
  bt_change_line.add_listener(this);
  bt_change_line.disable();

  bt_delete_line.setze_pos(koord(10, 32 + SCL_HEIGHT));
  bt_delete_line.setze_groesse(koord(92,14));
  bt_delete_line.setze_typ(button_t::roundbox);
  bt_delete_line.text = translator::translate("Delete Line");
  add_komponente(&bt_delete_line);
  bt_delete_line.add_listener(this);
  bt_delete_line.disable();

  //CHART
  chart = new gui_chart_t();
  chart->setze_pos(koord(COL2XPOS+26, 10));
  chart->set_dimension(12, 1000);
  chart->set_seed(0);
  chart->set_background(MN_GREY1);

  add_komponente(chart);
  //CHART END

  // add filter buttons
  for (int cost=0; cost<MAX_LINE_COST; cost++) {
    filterButtons[cost].init(button_t::box,
			     translator::translate(cost_type[cost]),
			     koord(COL2XPOS+(COLWIDTH/2+4) * (cost%2),
				   125+15*((int)cost/2+1)),
			     koord(COLWIDTH/2+3, 14));
    filterButtons[cost].add_listener(this);
    filterButtons[cost].background = cost_type_color[cost];
    add_komponente(filterButtons + cost);
  }

  scl->request_groesse(scl->gib_groesse());

  // resize button
  set_min_windowsize(koord(400, 400));
  set_resizemode(diagonal_resize);
  resize(koord(0,0));
}

schedule_list_gui_t::~schedule_list_gui_t()
{
	delete(scl);
	scl=0;
	delete(chart);
	chart=0;
}

/**
 * gibt den Besitzer zurueck
 *
 * @author Hj. Malthaner
 */
spieler_t* schedule_list_gui_t::gib_besitzer() const
{
    return welt->get_active_player();
}

char *schedule_list_gui_t::info(char *buf) const
{
    buf[0]=' ';
    buf[1]=0;
    return buf;
}

bool schedule_list_gui_t::action_triggered(gui_komponente_t *komp)           // 28-Dec-01    Markus Weber    Added
{
    if (komp == &bt_change_line)
    {
		if (line != NULL)
		{
			line_management_gui_t *line_gui = new line_management_gui_t(welt, line, welt->get_active_player());
			line_gui->zeige_info();
		}
    } else if (komp == &bt_new_line)
    {
    	if (tabs.get_active_tab_index() > 0) {
    	// create typed line
		simline_t * new_line = welt->simlinemgmt->create_line(tabs.get_active_tab_index());
		line_management_gui_t *line_gui = new line_management_gui_t(welt, new_line, welt->get_active_player());
		line_gui->zeige_info();

		// once a new line is added we need to renew the SCL and sort the lines
		// otherwise the selection of lines in the SCL gets messed up
		  welt->simlinemgmt->sort_lines();
		  scl->clear_elements();
		  build_line_list(tabs.get_active_tab_index());
		} else {
			create_win(-1, -1, 120, new nachrichtenfenster_t(welt, translator::translate("Cannot create generic line!\nSelect line type by\nusing filter tabs.")), w_autodelete);
		}
    } else if (komp == &bt_delete_line)
    {
		if (line != NULL)
		{
			// remove elements from lists
			lines.remove(line);
			welt->simlinemgmt->delete_line(line);
			scl->remove_element(scl->gib_selection());

			// clean up gui
			cont.remove_all();
			inp_name.set_visible(false);
			filled_bar.set_visible(false);
			build_line_list(tabs.get_active_tab_index());
			line = NULL;
			scl->setze_selection(-1);
			bt_delete_line.disable();
			bt_change_line.disable();
		}
    } else if (komp == &tabs)
    {
		scrolly.set_visible(false);
		scrolly_haltestellen.set_visible(false);
		inp_name.set_visible(false);
		filled_bar.set_visible(false);
    	build_line_list(tabs.get_active_tab_index());
    	scl->setze_selection(-1);
    	line = NULL;
    } else {
    	if (line != NULL)
    	{
    	    for ( int i = 0; i<MAX_LINE_COST; i++)
		    {
		    	if (komp == &filterButtons[i])
		    	{
		    		chart->is_visible(i) == true ? chart->hide_curve(i) : chart->show_curve(i);
		    		break;
		    	}
		    }
		}
    }

    return true;
}

void
schedule_list_gui_t::infowin_event(const event_t *ev)
{
  int x = ev->cx;
  int y = ev->cy;

    if (ev->ev_class == EVENT_CLICK) {
      if (line == NULL) {
  	  	bt_change_line.disable();
  	  	bt_delete_line.disable();
      } else {
  	  	bt_change_line.enable();
  	  	bt_delete_line.enable();
  	  }
		  if (scl->getroffen(x, y-40)) {
	    event_t ev2 = *ev;
			translate_event(&ev2, -tabs.pos.x, -tabs.pos.y-35);
	    scl->infowin_event(&ev2);

      selection = scl->gib_selection();
      // get selected line
      if ((selection >= 0) && (selection < (int)lines.count())) {
  	  	line = lines.at(selection);
  	  	bt_change_line.enable();
  	  	bt_delete_line.enable();
  	  } else {
  	  	line = NULL;
  	  }
      if (line == NULL) {
				return;
      }
      scrolly.set_visible(true);
      scrolly_haltestellen.set_visible(true);
      inp_name.setze_text(line->get_name(), 128);
      inp_name.set_visible(true);
      filled_bar.set_visible(true);

      // fill container with info of line's convoys
      // we do it here, since this list needs only to be
      // refreshed when the user selects a new line
      int icnv = 0;
      icnv = line->count_convoys();
      // display convoys of line
      cont.remove_all();
      int ypos = 5;
      int i;
      for (i = 0; i < icnv; i++) {
	gui_convoiinfo_t *cinfo = new gui_convoiinfo_t(line->get_convoy(i)->self, i + 1);
		cinfo->setze_pos(koord(0, ypos));
		cinfo->setze_groesse(koord(400, 40));
		cont.add_komponente(cinfo);
		ypos += 40;
      }

      cont.setze_groesse(koord(500, ypos));

      // fill haltestellen container with info of line's haltestellen
      cont_haltestellen.remove_all();
      ypos = 5;
      slist_tpl<koord> tmp; // stores koords of stops that are allready displayed
      for (i = 0; i<line->get_fahrplan()->maxi(); i++) {
	const koord fahrplan_koord = line->get_fahrplan()->eintrag.get(i).pos.gib_2d();
	halthandle_t halt = haltestelle_t::gib_halt(welt, fahrplan_koord);
	if (halt.is_bound()) {
		// only add a haltestelle to the list, if it
		// is not in the list allready
		if (!tmp.contains(fahrplan_koord)) {
		  halt_list_item_t *cinfo = new halt_list_item_t(halt, i + 1);
		  cinfo->setze_pos(koord(0, ypos));
		  cinfo->setze_groesse(koord(500, 40));
		  cont_haltestellen.add_komponente(cinfo);
		  ypos += 40;
		  tmp.append(fahrplan_koord);
		}
	}
      }
      cont_haltestellen.setze_groesse(koord(500, ypos));

      // chart
      chart->remove_curves();
      for (i = 0; i<MAX_LINE_COST; i++)  {
	chart->add_curve(cost_type_color[i], (sint64 *)line->get_finance_history(), MAX_LINE_COST, i, MAX_MONTHS, i < MAX_NON_MONEY_TYPES ? STANDARD : MONEY, filterButtons[i].pressed, true);
      }
      chart->set_visible(true);
    }
  }
  gui_frame_t::infowin_event(ev);
}


void schedule_list_gui_t::zeichnen(koord pos, koord gr)
{
  for (int i = 0;i<MAX_LINE_COST;i++) {
    filterButtons[i].pressed = chart->is_visible(i);
  }

  gui_frame_t::zeichnen(pos, gr);
  if (line != NULL) {
    display(pos);
  }
//  scl->zeichnen(pos);
}

void
schedule_list_gui_t::display(koord pos)
{
  int icnv = 0;
  icnv = line->get_finance_history(0,LINE_CONVOIS);

  char buffer[8192];
  char ctmp[128];

  capacity = load = loadfactor = 0; // total capacity and load of line (=sum of all conv's cap/load)

  long profit = line->get_finance_history(0,LINE_PROFIT);

  buffer[0] ='\0';

  if (line != NULL) {

    if (icnv > 0) {

      for (int i = 0; i<icnv; i++) {

	convoihandle_t cnv = line->get_convoy(i)->self;

	for (unsigned j = 0; j<cnv->gib_vehikel_anzahl(); j++) {

	  capacity += cnv->gib_vehikel(j)->gib_fracht_max();
	  load += cnv->gib_vehikel(j)->gib_fracht_menge();
	}
      }
    }
  }

  // we check if cap is zero, since theoretically a
  // conv can consist of only 1 vehicle, which has no cap (eg. locomotive)
  // and we do not like to divide by zero, do we?
  if (capacity > 0) {
    loadfactor = (load * 100) / capacity;
  }

  money_to_string(ctmp, profit / 100.0);
  sprintf(buffer,
	  translator::translate("Convois: %d\nProfit: %s"),
	  icnv,
	  ctmp);

  display_multiline_text(pos.x + groesse.x/2 + 2,
			 pos.y + 229,
			 buffer,
			 BLACK);

  number_to_string(ctmp, capacity);
  sprintf(buffer,
	  translator::translate("Capacity: %s\nLoad: %d (%d%%)"),
	  ctmp,
	  load,
	  loadfactor);

  display_multiline_text(pos.x + groesse.x/2 + 106,
			 pos.y + 229,
			 buffer,
			 BLACK);
}

void schedule_list_gui_t::resize(const koord delta)
{
  gui_frame_t::resize(delta);
  this->groesse = get_client_windowsize() + koord(0, 16);

  scrolly.setze_pos(koord(groesse.x/2, 240));
  scrolly.setze_groesse(koord(groesse.x/2,
			      get_client_windowsize().y-scrolly.gib_pos().y));

  scrolly_haltestellen.setze_groesse(koord(groesse.x/2,
					   get_client_windowsize().y-scrolly_haltestellen.gib_pos().y));

  chart->setze_groesse(koord(groesse.x - 242, 110));

  filled_bar.setze_groesse(koord(groesse.x/2-32, 4));
  filled_bar.setze_pos(koord(groesse.x/2+11, 236));
}

void schedule_list_gui_t::build_line_list(int filter)
{
  if (welt->simlinemgmt->count_lines() > 0)
  {
  	scl->clear_elements();
  	lines.clear();
    slist_iterator_tpl<simline_t *> iter( simlinemgmt_t::all_managed_lines );
    while( iter.next() ) {
      simline_t *line = iter.get_current();
      if ((line->get_linetype() == filter) || (filter == 0)) {
			scl->append_element( line->get_name() );
			lines.append(line);
	    }
    }
  }
}
