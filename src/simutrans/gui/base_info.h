/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_BASE_INFO_H
#define GUI_BASE_INFO_H


#include "gui_frame.h"
#include "components/gui_fixedwidth_textarea.h"
#include "../utils/cbuffer.h"

class gui_textarea_with_embedded_element_t;

/**
 * Base class to show window with fixed-width text with an embedded object.
 */
class base_infowin_t : public gui_frame_t
{
protected:
	/// buffers the text
	cbuffer_t buf;

	/// displays the text
	gui_fixedwidth_textarea_t textarea;

	// special container to handle text area with embedded component
	gui_textarea_with_embedded_element_t *container;

	/// recalcs size of text, adjusts window-size if necessary
	void recalc_size();

	/// adds @p other as embedded element within textarea
	void set_embedded(gui_component_t *other);

	gui_component_t *get_embedded() const;

public:
	base_infowin_t(const char *name, const player_t *player=NULL);
};


#endif
