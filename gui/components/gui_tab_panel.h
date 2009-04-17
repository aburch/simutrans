#ifndef gui_tab_panel_h
#define gui_tab_panel_h

#include "../../simimg.h"

#include "../../besch/skin_besch.h"

#include "../../ifc/gui_action_creator.h"
#include "../../ifc/gui_komponente.h"

class bild_besch_t;

/**
 * Eine Klasse für Registerkartenartige Aufteilung von gui_komponente_t
 * Objekten.
 *
 * @author Hj. Malthaner
 */
class gui_tab_panel_t :
	public gui_action_creator_t,
	public gui_komponente_t
{
private:
	struct tab
	{
		tab(gui_komponente_t* c, const char *name, const bild_besch_t *b, const char *tool) : component(c), title(name), img(b), tooltip(tool) {}

		gui_komponente_t* component;
		const char *title;
		const bild_besch_t *img;
		const char *tooltip;
	};

	slist_tpl<tab> tabs;
	int active_tab;

public:
	enum { HEADER_VSIZE = 18};

	gui_tab_panel_t();

	/**
	 * Fügt eine neue Registerkarte hinzu.
	 * @param c die Komponente für die Rgisterkarte
	 * @param name der Name der Registerkarte für die Komponente
	 * @author Hj. Malthaner
	 */
	void add_tab(gui_komponente_t *c, const char *name, const skin_besch_t *b=NULL, const char *tooltip=NULL );

	/**
	 * Gibt die aktuell angezeigte Komponente zurück.
	 * @author Hj. Malthaner
	 */
	gui_komponente_t* get_aktives_tab() const { return tabs.at(active_tab).component; }

	int get_active_tab_index() const { return min(tabs.get_count()-1,active_tab); }
	void set_active_tab_index( int i ) { active_tab = min(tabs.get_count()-1,i); }

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author Hj. Malthaner
	 */
	void infowin_event(const event_t *ev);

	/**
	 * Zeichnet die Registerkarten
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);

	/**
	 * Resizing must be propagated!
	 * @author Volker Meyer
	 * @date  18.06.2003
	 */
	void set_groesse(koord groesse);
};

#endif
