#ifndef gui_sprachen_h
#define gui_sprachen_h

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_textarea.h"
#include "components/gui_image.h"
#include "components/gui_divider.h"
#include "../utils/cbuffer_t.h"

#include "../tpl/vector_tpl.h"


/**
 * Sprachauswahldialog
 *
 * @author Hj. Maltahner, Niels Roest, prissi
 */
class sprachengui_t : public gui_frame_t, private action_listener_t
{
private:
	cbuffer_t buf;
	gui_textarea_t text_label;
	gui_image_t flags;

	struct language_button_t {
		button_t* button;
		int id;
	};
	vector_tpl<language_button_t> buttons;
	gui_divider_t seperator;

	static int cmp_language_button(sprachengui_t::language_button_t a, sprachengui_t::language_button_t b);

public:
	/**
	 * Causes the required fonts for currently selected
	 * language to be loaded
	 * @author Hj. Malthaner
	 */
	static void init_font_from_lang();

	sprachengui_t();

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen für die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "language.txt";}

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
