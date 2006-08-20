/*
 * messagebox.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifdef _MSC_VER
#include <string.h>
#endif

#include <stdio.h>

#include "../simworld.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../dataobj/translator.h"
#include "../utils/cbuffer_t.h"

#include "messagebox.h"

nachrichtenfenster_t::nachrichtenfenster_t(karte_t *welt, const char *text)
 : infowin_t(welt)
{
    meldung = text;
    bild = skinverwaltung_t::meldungsymbol->gib_bild_nr(0);
}

nachrichtenfenster_t::nachrichtenfenster_t(karte_t *welt, const char *text, int bild)
 : infowin_t(welt)
{
	meldung = text;
	this->bild = bild;
	int xoff, yoff, xw, yw;
	xoff = yw = 0;
	display_get_image_offset( bild, &xoff, &yoff, &xw, &yw );
	this->bild_offset = koord(48-xw-xoff,72-yw-yoff);
}

nachrichtenfenster_t::nachrichtenfenster_t(karte_t *welt, const char *text, int bild, koord off)
 : infowin_t(welt)
{
	meldung = text;
	this->bild = bild;
	this->bild_offset = off;
}


const char *
nachrichtenfenster_t::gib_name() const
{
    return "Meldung";
}


int
nachrichtenfenster_t::gib_bild() const
{
   return bild;
}

/**
 * Das Bild kann im Fenster über Offsets plaziert werden
 *
 * @author Hj. Malthaner
 * @return den x,y Offset des Bildes im Infofenster
 */
koord nachrichtenfenster_t::gib_bild_offset() const
{
    return bild_offset;
}


void nachrichtenfenster_t::info(cbuffer_t & buf) const
{
    buf.append(translator::translate(meldung));
}

/**
 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
 * zurück
 * @author Hj. Malthaner
 */
/*
fensterfarben
nachrichtenfenster_t::gib_fensterfarben() const
{
  fensterfarben f;

  const spieler_t *sp = gib_besitzer();

  f.titel  = (sp != NULL) ? sp->kennfarbe : WIN_TITEL;
  f.hell   = MESG_WIN_HELL;
  f.mittel = MESG_WIN;
  f.dunkel = MESG_WIN_DUNKEL;

  return f;
}
*/
