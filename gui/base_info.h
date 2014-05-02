#ifndef base_info_h_
#define base_info_h_

#include "gui_frame.h"
#include "components/gui_fixedwidth_textarea.h"
#include "../utils/cbuffer_t.h"

/**
 * Base class to show fixed-width text with an embedded object.
 */
class base_infowin_t : public gui_frame_t
{
protected:
	/// buffers the text
	cbuffer_t buf;

	/// displays the text
	gui_fixedwidth_textarea_t textarea;

	/// the embedded component
	gui_komponente_t *embedded;

	/// recalcs size of text, adjusts window-size if necessary
	void recalc_size();

	/// sets position and size of @p other, adds it to the window
	void set_embedded(gui_komponente_t *other);

public:
	base_infowin_t(const char *name, const spieler_t *sp=NULL);
};


#endif
