/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef gui_gui_frame_h
#define gui_gui_frame_h

#include "../dataobj/koord.h"
#include "../simgraph.h"
#include "gui_container.h"
#include "../simcolor.h"
#include "../dataobj/koord3d.h"


class loadsave_t;
class spieler_t;


/*
 * The following gives positioning aids for elements in dialoges
 * Only those, LINESPACE, and dimensions of elements itself must be
 * exclusively used to calculate positions in dialogues to have a
 * scaleable interface
 */

// default button width (may change with langugae and font)
#define D_BUTTON_WIDTH (gui_frame_t::gui_button_width)
#define D_BUTTON_HEIGHT (gui_frame_t::gui_button_height)

// titlebar height
#define D_TITLEBAR_HEIGHT (gui_frame_t::gui_titlebar_height)

// dialog borders
#define D_MARGIN_LEFT (gui_frame_t::gui_frame_left)
#define D_MARGIN_TOP (gui_frame_t::gui_frame_top)
#define D_MARGIN_RIGHT (gui_frame_t::gui_frame_right)
#define D_MARGIN_BOTTOM (gui_frame_t::gui_frame_bottom)

// space between two elements
#define D_H_SPACE (gui_frame_t::gui_hspace)
#define D_V_SPACE (gui_frame_t::gui_vspace)

#define BUTTON1_X (D_MARGIN_LEFT)
#define BUTTON2_X (D_MARGIN_LEFT+1*(D_BUTTON_WIDTH+D_H_SPACE))
#define BUTTON3_X (D_MARGIN_LEFT+2*(D_BUTTON_WIDTH+D_H_SPACE))
#define BUTTON4_X (D_MARGIN_LEFT+3*(D_BUTTON_WIDTH+D_H_SPACE))

// The width of a typical dialoge (either list/covoi/factory) and intial width when it makes sense
#define D_DEFAULT_WIDTH (D_MARGIN_LEFT+4*D_BUTTON_WIDTH+3*D_H_SPACE+D_MARGIN_RIGHT)

// dimensions of indicator bars (not yet a gui element ...)
#define D_INDICATOR_WIDTH (gui_frame_t::gui_indicator_width)
#define D_INDICATOR_HEIGHT (gui_frame_t::gui_indicator_height)




/**
 * Eine Klasse für Fenster mit Komponenten.
 * Anders als die anderen Fensterklasen in Simutrans ist dies
 * ein richtig Komponentenorientiertes Fenster, das alle
 * aktionen an die Komponenten delegiert.
 *
 * @author Hj. Malthaner
 */
class gui_frame_t
{
public:
	/**
	 * Resize modes
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	enum resize_modes {
		no_resize = 0, vertical_resize = 1, horizonal_resize = 2, diagonal_resize = 3
	};

	// default button sizes
	static KOORD_VAL gui_button_width;
	static KOORD_VAL gui_button_height;

	// titlebar height
	static KOORD_VAL gui_titlebar_height;

	// dialog borders
	static KOORD_VAL gui_frame_left;
	static KOORD_VAL gui_frame_top;
	static KOORD_VAL gui_frame_right;
	static KOORD_VAL gui_frame_bottom;

	// space between two elements
	static KOORD_VAL gui_hspace;
	static KOORD_VAL gui_vspace;

	// and the indicator box dimension
	static KOORD_VAL gui_indicator_width;
	static KOORD_VAL gui_indicator_height;

private:
	gui_container_t container;

	const char * name;
	koord groesse;

	/**
	 * Min. size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	koord min_windowsize;

	resize_modes resize_mode; // 25-may-02  markus weber added
	const spieler_t *owner;

	// set true for total redraw
	bool dirty:1;
	bool opaque:1;

	uint8 percent_transparent;
	COLOR_VAL color_transparent;

protected:
	void set_dirty() { dirty=1; }

	/**
	 * resize window in response to a resize event
	 * @author Markus Weber, Hj. Malthaner
	 * @date   11-May-2002
	 */
	virtual void resize(const koord delta);

	void set_owner( const spieler_t *sp ) { owner = sp; }

	void set_transparent( uint8 percent, COLOR_VAL col ) { opaque = percent==0; percent_transparent = percent; color_transparent = col; }

public:
	/**
	 * @param name Fenstertitel
	 * @param sp owner for color
	 * @author Hj. Malthaner
	 */
	gui_frame_t(const char *name, const spieler_t *sp=NULL);

	virtual ~gui_frame_t() {}

	/**
	 * Fügt eine Komponente zum Fenster hinzu.
	 * @author Hj. Malthaner
	 */
	void add_komponente(gui_komponente_t *komp) { container.add_komponente(komp); }

	/**
	 * Entfernt eine Komponente aus dem Container.
	 * @author Hj. Malthaner
	 */
	void remove_komponente(gui_komponente_t *komp) { container.remove_komponente(komp); }

	/**
	 * Der Name wird in der Titelzeile dargestellt
	 * @return den nicht uebersetzten Namen der Komponente
	 * @author Hj. Malthaner
	 */
	const char *get_name() const { return name; }

	/**
	 * setzt den Namen (Fenstertitel)
	 * @author Hj. Malthaner
	 */
	void set_name(const char *name) { this->name=name; }

	/* this returns an unique id, if the dialoge can be saved
	 * if this is defined, you better define a matching constructor with karte_t * and loadsave_t *
	 */
	virtual uint32 get_rdwr_id() { return 0; }

	virtual void rdwr( loadsave_t * ) {}

	/**
	 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
	 * zurück
	 * @author Hj. Malthaner
	 */
	virtual PLAYER_COLOR_VAL get_titelcolor() const;

	/**
	 * @return gibt wunschgroesse für das Darstellungsfenster zurueck
	 * @author Hj. Malthaner
	 */
	koord get_fenstergroesse() const { return groesse; }

	/**
	 * Setzt die Fenstergroesse
	 * @author Hj. Malthaner
	 */
	virtual void set_fenstergroesse(koord groesse);

	/**
	 * Set minimum size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	void set_min_windowsize(koord size) { min_windowsize = size; }

	/**
	 * Set minimum size of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	koord get_min_windowsize() { return min_windowsize; }

	/**
	 * @return returns the usable width and heigth of the window
	 * @author Markus Weber
	 * @date   11-May-2002
	*/
	koord get_client_windowsize() const {return groesse-koord(0,D_TITLEBAR_HEIGHT); }

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	virtual const char * get_hilfe_datei() const {return NULL;}

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	virtual bool has_min_sizer() const {return false;}

	/**
	 * Does this window need a next button in the title bar?
	 * @return true if such a button is needed
	 * @author Volker Meyer
	 */
	virtual bool has_next() const {return false;}

	/**
	 * Does this window need a prev button in the title bar?
	 * @return true if such a button is needed
	 * @author Volker Meyer
	 */
	virtual bool has_prev() const {return has_next();}

	virtual bool has_sticky() const { return true; }

	// if false, title and all gadgets will be not drawn
	virtual bool has_title() const { return true; }

	// position of a connected thing on the map
	// The calling parameter is only true when actually clicking the gadget
	virtual koord3d get_weltpos( bool /*set*/ ) { return koord3d::invalid; }

	// returns true, when the window show the current object already
	virtual bool is_weltpos() { return false; }

	bool is_dirty() const { return dirty; }

	/**
	 * Set resize mode
	 * @author Markus Weber
	 * @date   11-May-2002
	 */
	void set_resizemode(resize_modes mode) { resize_mode = mode; }

	/**
	 * Get resize mode
	 * @author Markus Weber
	 * @date   25-May-2002
	 */
	resize_modes get_resizemode(void) { return resize_mode; }

	/**
	 * Returns true, if inside window area.
	 */
	virtual bool getroffen(int x, int y)
	{
		koord groesse = get_fenstergroesse();
		return (  x>=0  &&  y>=0  &&  x<groesse.x  &&  y<groesse.y  );
	}

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	virtual bool infowin_event(const event_t *ev);

	/**
	 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
	 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	 * in dem die Komponente dargestellt wird.
	 * @author Hj. Malthaner
	 */
	virtual void zeichnen(koord pos, koord gr);

	// called, when the map is rotated
	virtual void map_rotate90( sint16 /*new_ysize*/ ) { }


	/**
	 * Set focus to component.
	 *
	 * @warning Calling this method from anything called from
	 * gui_container_t::infowin_event (e.g. in action_triggered)
	 * will have NO effect.
	 */
	void set_focus( gui_komponente_t *k ) { container.set_focus(k); }

	virtual gui_komponente_t *get_focus() { return container.get_focus(); }
};

#endif
