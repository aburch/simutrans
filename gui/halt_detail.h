/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_halt_detail_h
#define gui_halt_detail_h

#include "components/gui_textarea.h"

#include "gui_frame.h"
#include "components/gui_scrollpane.h"

#include "../halthandle_t.h"
#include "../utils/cbuffer_t.h"

class spieler_t;
class karte_t;


/**
 * Dies stellt ein Fenster mit den Zielinformationen
 * fuer eine Haltestelle dar.
 *
 * @author Hj. Malthaner
 */

class halt_detail_t : public gui_frame_t
{
private:
	halthandle_t halt;

	gui_scrollpane_t scrolly;
	gui_textarea_t txt_info;

	cbuffer_t cb_info_buffer;

public:
	halt_detail_t(halthandle_t halt);

	void halt_detail_info(cbuffer_t & buf);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * gib_hilfe_datei() const { return "station_details.txt"; }

	/**
	 * Komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord pos, koord gr);};

#endif
