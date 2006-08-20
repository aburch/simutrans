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

	flow.setze_pos(koord(10, 6));
	flow.setze_groesse(koord(220, 0));

	// try to get the following sizes
	// y<400 or, if not possible, x<620
	int last_y = 0;
	koord curr=flow.get_preferred_size();
	for( int i=0;  i<10  &&  curr.y>400  &&  curr.y!=last_y;  i++  )
	{
		flow.setze_groesse(koord(260+i*40, 0));
		last_y = curr.y;
		curr = flow.get_preferred_size();
	}

	// the second line isn't redundant!!!
	flow.setze_groesse(flow.get_preferred_size());
	flow.setze_groesse(flow.get_preferred_size());

	setze_name(flow.get_title());

	// set window size
	curr = flow.gib_groesse()+koord(20, 36);
	if(curr.y>display_get_height()-64) {
		curr.y = display_get_height()-64;
	}
	setze_fenstergroesse( curr );
	resize( koord(0,0) );
}


help_frame_t::help_frame_t() :
	gui_frame_t("Help"),
	scrolly(&flow)
{
	char buf[64];

	tstrncpy(buf, "<title>Unnamed</title><p>No text set</p>", 64);

	setze_text(buf);
	setze_opaque(true);
	add_komponente(&flow);

	flow.add_listener(this);
}


help_frame_t::help_frame_t(cstring_t filename) :
	gui_frame_t("Help"),
	scrolly(&flow)
{
    char buf[8192];

    cstring_t file_prefix("text/");
    cstring_t fullname = file_prefix + translator::get_language_name_iso(translator::get_language()) + "/" + filename;


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
		file = fopen(file_prefix+"/en/"+filename,"rb");
	}

	if(file) {
		const long len = fread(buf, 1, 8191, file);
		buf[len] = '\0';
		fclose(file);
	}
	else {
		tstrncpy(buf, "<title>Error</title>Help text not found", 64);
	}

	setze_text(buf);
	setze_opaque(true);
	set_resizemode(diagonal_resize);
	add_komponente(&scrolly);
	flow.add_listener(this);
}


/**
 * Called upon link activation
 * @param the hyper ref of the link
 * @author Hj. Malthaner
 */
bool
help_frame_t::action_triggered(gui_komponente_t *, value_t extra)
{
	create_win(new help_frame_t((const char *)(extra.p)), w_autodelete, magic_none);
	return true;
}



/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void help_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	scrolly.setze_groesse(get_client_windowsize());
}
