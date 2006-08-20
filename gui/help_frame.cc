/*
 * help_frame.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <stdio.h>
#include "help_frame.h"
#include "../simwin.h"
#include "../utils/cstring_t.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"


void help_frame_t::setze_text(const char * buf)
{
    flow.set_text(buf);

    flow.setze_pos(koord(10, 10));
    flow.setze_groesse(koord(220, 0));

    int i = 0;
    int last_y = 0;
    koord curr;

    while(curr=flow.get_preferred_size(),
          (curr.y > 400 && curr.y != last_y && i < 9)) {
	flow.setze_groesse(koord(260+i*40, 0));
	last_y = curr.y;
	i++;
    }

    // the second line isn't redundant!!!
    flow.setze_groesse(flow.get_preferred_size());
    flow.setze_groesse(flow.get_preferred_size());

    setze_fenstergroesse(flow.gib_groesse()+koord(20, 36));
    setze_name(flow.get_title());
}


help_frame_t::help_frame_t() : gui_frame_t("Help")
{
    char buf[64];

    tstrncpy(buf, "<title>Unnamed</title><p>No text set</p>", 64);

    setze_text(buf);
    setze_opaque(true);
    add_komponente(&flow);
    flow.add_listener(this);
}


help_frame_t::help_frame_t(cstring_t filename) : gui_frame_t("Help")
{
    char buf[8192];

    cstring_t file_prefix("text/");

    cstring_t fullname =
      file_prefix+
      translator::get_language_name_iso(translator::get_language())+
      "/"+
      filename;

    //printf("Loading '%s'\n", fullname.chars());

    FILE * file = fopen(fullname, "rb");

    if(!file) {
        //Check for the 'base' language(ie en from en_gb)
        file = fopen(file_prefix+
                     translator::get_language_name_iso_base(translator::get_language())+
                     "/"+
                     filename,
                     "rb");
    }

    if(!file) {
	// Hajo: check fallback english
	file = fopen(file_prefix+
                        "/en/"+
			filename,
			"rb");
    }

    if(file) {
	const int len = fread(buf, 1, 8191, file);
	buf[len] = '\0';

	fclose(file);
    } else {
        tstrncpy(buf, "<title>Error</title>Help text not found", 64);
    }

    setze_text(buf);
    setze_opaque(true);
    add_komponente(&flow);
    flow.add_listener(this);
}


/**
 * Called upon link activation
 * @param the hyper ref of the link
 * @author Hj. Malthaner
 */
void help_frame_t::hyperlink_activated(const cstring_t &txt)
{
  create_win(new help_frame_t(txt), w_autodelete, magic_none);
}
