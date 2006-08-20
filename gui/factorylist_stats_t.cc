/*
 * factorylist_stats_t.cc
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "factorylist_stats_t.h"

#include "../simgraph.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simfab.h"
#include "../simwin.h"
#include "../simworld.h"

#include "../bauer/warenbauer.h"
#include "../utils/cbuffer_t.h"

static char *sort_text[6] = {
"Fabrikname",
"Input",
"Output",
"Produktion",
"Rating",
"Power"
};


factorylist_header_t::factorylist_header_t(factorylist_stats_t *s) : stats(s)
{
  setze_groesse(koord(350, 38));

  // init buttons
  bt_filter.setze_groesse( koord(90, 14) );
  bt_filter.setze_typ(button_t::roundbox);
  bt_filter.setze_pos( koord(100,4) );
  stats->set_sortmode(FL_SM_NAME);
  bt_filter.text = translator::translate(sort_text[FL_SM_NAME]);
}


factorylist_header_t::~factorylist_header_t()
{
// DBG_DEBUG("factorylist_header_t()","destructor");
}

void factorylist_header_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTCLICK(ev)) {
		bt_filter.getroffen(ev->cx, ev->cy);
	}

		if(IS_LEFTRELEASE(ev)) {
		if(bt_filter.getroffen(ev->cx, ev->cy)) 	{
			// next selection
			bt_filter.pressed = false;
			const int mode = (stats->get_sortmode()+1) %  (FL_SM_POWER+1);
			stats->set_sortmode(mode);
			bt_filter.text = translator::translate(sort_text[mode]);
		}
	}
}


void factorylist_header_t::zeichnen(koord pos) const
{
  static cbuffer_t buf(64);

  display_proportional_clip(pos.x+10, pos.y+8, translator::translate("hl_txt_sort"), ALIGN_LEFT, SCHWARZ, true);
  bt_filter.zeichnen(pos);

  buf.clear();
  buf.append(translator::translate(sort_text[FL_SM_STATUS]));
  buf.append(" ");
  buf.append(translator::translate(sort_text[FL_SM_POWER]));
  buf.append(" : ");
  buf.append(translator::translate(sort_text[FL_SM_NAME]));
  buf.append(" (");
  buf.append(translator::translate(sort_text[FL_SM_INPUT]));
  buf.append(", ");
  buf.append(translator::translate(sort_text[FL_SM_OUTPUT]));
  buf.append(", ");
  buf.append(translator::translate(sort_text[FL_SM_MAXPROD]));
  buf.append(")");
  display_proportional_clip(pos.x+6, pos.y+25, buf, ALIGN_LEFT, WEISS, true);
}



factorylist_stats_t::factorylist_stats_t(karte_t * w) : welt(w), sortmode(FL_SM_NAME)
{
setze_groesse(koord(210, welt->gib_fab_list().count()*14 +14));
//DBG_DEBUG("factorylist_stats_t()","constructor");
fab_list = new vector_tpl<fabrik_t*>(1);
sort();
}

factorylist_stats_t::~factorylist_stats_t()
{
//DBG_DEBUG("factorylist_stats_t()","destructor");
}

void factorylist_stats_t::set_sortmode(unsigned int sortmode)
{
     if (sortmode == this->sortmode)
        return;

     if (sortmode <= FL_SM_POWER) {
        this->sortmode = sortmode;
        sort();
     }
 }

unsigned int factorylist_stats_t::get_sortmode(void)
{
 return this->sortmode;
}

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void factorylist_stats_t::infowin_event(const event_t * ev)
{

if (IS_LEFTRELEASE(ev))
{
	const unsigned int line = (ev->cy) / 14;

	if(line < fab_list->get_count())
	{
		fabrik_t * fab = fab_list->at(line);
		if(fab)
		{
			/* magic_none: otherwise only one dialog is allowed */
			//create_win(320, 0,-1,fab->gib_fabrik_info(),w_info,magic_none);
			const koord3d pos = fab->gib_pos();

			if(welt->ist_in_kartengrenzen(pos.gib_2d()))
			{
				welt->setze_ij_off(pos.gib_2d() + koord(-5,-5));
				grund_t *gr = welt->lookup(pos.gib_2d())->gib_kartenboden();
				if (gr)
				{
					gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
					if (gb)
					{
						create_win(320, 0,-1,new fabrik_info_t(fab,gb,welt),w_info,magic_none);

					}
				}
			}
		}
	}
}
} // end of function factorylist_stats_t::infowin_event(const event_t * ev)



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void factorylist_stats_t::zeichnen(koord offset) const
{
//DBG_DEBUG("factorylist_stats_t()","zeichnen()");

	static cbuffer_t buf(256);
	int xoff = offset.x;
	int yoff = offset.y;

	for (unsigned int i=0; i<fab_list->get_count(); i++) {

		const fabrik_t *fab = fab_list->at(i);
		if(fab) {

//DBG_DEBUG("factorylist_stats_t()","zeichnen() factory %i",i);
			unsigned long input, output;
			unsigned indikatorfarbe = fabrik_t::status_to_color[ fab->calc_factory_status( &input, &output ) ];

			buf.clear();
	//		buf.append(i+1);
	//		buf.append(".) ");
			buf.append(fab_list->at(i)->gib_name());
			buf.append(" (");

			if (fab->gib_eingang()->get_count() > 0) {
				buf.append(input);
			}
			else {
				buf.append("-");
			}
			buf.append(", ");

			if (fab->gib_ausgang()->get_count() > 0) {
				buf.append(output);
			}
			else {
				buf.append("-");
			}
			buf.append(", ");

			buf.append(fab->max_produktion());
			buf.append(") ");


			display_ddd_box_clip(xoff+7, yoff+6, 8, 8, MN_GREY0, MN_GREY4);
			display_fillbox_wh_clip(xoff+8, yoff+7, 6, 6, indikatorfarbe, true);

			if (fab->get_prodfaktor() > 16) {
			    display_color_img(skinverwaltung_t::electricity->gib_bild_nr(0), xoff+17, yoff+6, 0, false, false);
			}

			// show text
			display_proportional_clip(xoff+25,yoff+6,buf,ALIGN_LEFT,SCHWARZ,true);
		}
	    yoff+=14;
	}
//DBG_DEBUG("factorylist_stats_t()","zeichnen() ende");
}




void factorylist_stats_t::sort()
{
	slist_iterator_tpl<fabrik_t *> fab_iter (welt->gib_fab_list());

	fab_list->clear();
	fab_list->resize(welt->gib_fab_list().count());

	//DBG_DEBUG("factorylist_stats_t()","fabriken.count() == %d",fabriken->count());

	if(!fab_iter.next()) {
		return;
	}
	// ok, we have at least one factory
	fab_list->append(fab_iter.get_current());

	while(fab_iter.next()) {
		fabrik_t *fab = fab_iter.get_current();

		bool append = true;
		for (unsigned int j=0; j<fab_list->get_count(); j++) {
			const fabrik_t *check_fab=fab_list->at(j);

			unsigned long input, output, check_input, check_output;
			switch(sortmode) {
				case FL_SM_NAME:
					append = strcmp(fab->gib_name(),check_fab->gib_name())>=0;
					break;
				case FL_SM_INPUT:
					fab->calc_factory_status( &input, &output );
					check_fab->calc_factory_status( &check_input, &check_output );
					append = (input<=check_input  &&  check_fab->gib_eingang()->get_count()!=0);
					break;
				case FL_SM_OUTPUT:
					fab->calc_factory_status( &input, &output );
					check_fab->calc_factory_status( &check_input, &check_output );
					append = (output<=check_output  &&  check_fab->gib_ausgang()->get_count()!=0);
					break;
				case FL_SM_MAXPROD:
					append =  (fab->max_produktion() <= check_fab->max_produktion());
					break;
				case FL_SM_STATUS:
					append = fab->calc_factory_status( NULL, NULL )>=check_fab->calc_factory_status( NULL, NULL );
					break;
				case FL_SM_POWER:
					append = (fab->get_prodfaktor() <= check_fab->get_prodfaktor());
			}
			if(!append) {
				fab_list->insert_at(j,fab);
				break;
			}
		} // end of for (unsigned int j=0; j<fab_list->get_count(); j++)
		if(append) {
			fab_list->append(fab);
		}

	} // end of iterator

} // end of function factorylist_stats_t::sort()
