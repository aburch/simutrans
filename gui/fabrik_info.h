/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef fabrikinfo_t_h
#define fabrikinfo_t_h

#include "../simwin.h"

#include "factory_chart.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_ding_view_t.h"
#include "gui_container.h"
#include "../utils/cbuffer_t.h"

class welt_t;
class fabrik_t;
class gebaeude_t;
class button_t;


/**
 * info on city demand

 * @author
 */
class gui_fabrik_info_t : public gui_container_t
{
public:
	const fabrik_t* fab;

	gui_fabrik_info_t() {}

	void zeichnen(koord offset);
};


/**
 * Info window for factories
 * @author Hj. Malthaner
 */
class fabrik_info_t : public gui_frame_t, public action_listener_t
{
 private:
	fabrik_t *fab;
	karte_t *welt;

	cbuffer_t info_buf, prod_buf;

	factory_chart_t chart;
	button_t chart_button;

	button_t details_button;

	ding_view_t view;

	char fabname[256];
	gui_textinput_t input;

	button_t *lieferbuttons;
	button_t *supplierbuttons;
	button_t *stadtbuttons;

	gui_scrollpane_t scrolly;
	gui_fabrik_info_t fab_info;
	gui_textarea_t prod, txt;

	void rename_factory();

public:
	// refreshes all text and location pointers
	void update_info();

	fabrik_info_t(fabrik_t* fab, const gebaeude_t* gb);
	virtual ~fabrik_info_t();

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "industry_info.txt";}

	virtual bool has_min_sizer() const {return true;}

	virtual koord3d get_weltpos(bool) { return fab->get_pos(); }

	virtual bool is_weltpos();

	virtual void set_fenstergroesse(koord groesse);

	/**
	* komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*
	* @author Hj. Malthaner
	*/
	virtual void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// rotated map need new info ...
	void map_rotate90( sint16 ) { update_info(); }

	// this contructor is only used during loading
	fabrik_info_t(karte_t *welt);

	void rdwr( loadsave_t *file );

	uint32 get_rdwr_id() { return magic_factory_info; }
};

#endif
