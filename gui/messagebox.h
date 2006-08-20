#ifndef gui_messagebox_h
#define gui_messagebox_h

#include "gui_frame.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"

/**
 * Eine Klasse für Nachrichtenfenster.
 * @author Hj. Malthaner
 */
class nachrichtenfenster_t : public gui_frame_t
{
private:
    gui_image_t bild;
    gui_textarea_t meldung;
    koord bild_offset;

public:
    nachrichtenfenster_t(karte_t *welt, const char *text, image_id bild=skinverwaltung_t::meldungsymbol->gib_bild_nr(0) );
};

#endif
