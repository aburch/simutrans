#ifndef gui_messagebox_h
#define gui_messagebox_h

#include "gui_frame.h"
#include "components/gui_world_view_t.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"
#include "../simcolor.h"

/**
 * Eine Klasse für Nachrichtenfenster.
 * @author Hj. Malthaner
 */
class nachrichtenfenster_t : public gui_frame_t
{
private:
	gui_image_t bild;
	gui_textarea_t meldung;
	world_view_t view;
	PLAYER_COLOR_VAL color;

public:
	nachrichtenfenster_t(karte_t *welt, const char *text, image_id bild=skinverwaltung_t::meldungsymbol->gib_bild_nr(0), koord k=koord::invalid, PLAYER_COLOR_VAL color=WIN_TITEL );

	virtual PLAYER_COLOR_VAL get_titelcolor() const { return color; }
};

#endif
