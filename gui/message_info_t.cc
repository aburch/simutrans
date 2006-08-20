/*
 * message_info_t.cpp
 *
 * Copyright (c) 2005 prissi
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simcolor.h"
#include "../simwin.h"
#include "../utils/cbuffer_t.h"
#include "../dataobj/translator.h"

#include "message_info_t.h"
#include "help_frame.h"


message_info_t::message_info_t(karte_t *welt,int color,char *text,koord k,int bild) : infowin_t(welt)
{
	message_info_t::welt = welt;
	message_info_t::text = text;
	message_info_t::color = color;
	message_info_t::k = k;
	message_info_t::bild = bild;
}



message_info_t::~message_info_t()
{
}



const char *
message_info_t::gib_name() const
{
    return "Meldung";
}


int
message_info_t::gib_bild() const
{
   return bild;
}


void
message_info_t::info(cbuffer_t & buf) const
{
	buf.append(text);
}



/* return color */
fensterfarben
message_info_t::gib_fensterfarben() const {
	fensterfarben f;
	f.titel  = color;
	f.hell   = MN_GREY4;
	f.mittel = MN_GREY2;
	f.dunkel = MN_GREY0;
	return f;
};


/* draw it *
message_info_t::zeichnen(koord offset)
{
	display_multiline_text(offset.x,offset.y,text,SCHWARZ);
}*/
