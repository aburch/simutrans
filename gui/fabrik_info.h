/*
 * fabrik_info.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef fabrikinfo_t_h
#define fabrikinfo_t_h

#include "thing_info.h"
#include "components/gui_world_view_t.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "gui_container.h"
#include "../utils/cbuffer_t.h"
#include "../simfab.h"

class fabrik_t;
class gebaeude_t;
class karte_t;

class button_t;

/**
 * Info window for factories
 * @author Hj. Malthaner
 */
class fabrik_info_t : public ding_infowin_t, public action_listener_t
{
 private:
	fabrik_t * fab;
	karte_t  * welt;

	button_t * lieferbuttons;
	button_t * supplierbuttons;
	button_t * stadtbuttons;

	button_t * about;

	gui_scrollpane_t scrolly;
	gui_container_t cont;
	gui_textarea_t txt;
	static cbuffer_t info_buf;

 public:
	fabrik_info_t(fabrik_t *fab, gebaeude_t *gb, karte_t *welt);

	~fabrik_info_t();

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * gib_hilfe_datei() const {return "industry_info.txt";}

	/**
	 * @return the text to display in the info window
	 *
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	void info(cbuffer_t & buf) const { fab->info(buf); }

	/**
	* komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*
	* @author Hj. Malthaner
	*/
	void zeichnen(koord pos, koord gr);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered(gui_komponente_t *komp, value_t extra);
};

#endif // fabrikinfo_t_h
