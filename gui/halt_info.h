/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_HALT_INFO_H
#define GUI_HALT_INFO_H


#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_button.h"
#include "components/gui_button_to_chart.h"
#include "components/gui_location_view_t.h"
#include "components/gui_tab_panel.h"
#include "components/action_listener.h"
#include "components/gui_chart.h"
#include "components/gui_image.h"
#include "components/gui_colorbox.h"

#include "../utils/cbuffer_t.h"
#include "../simhalt.h"
#include "simwin.h"

class gui_departure_board_t;
class gui_halt_detail_t;

/**
 * Helper class to show type symbols (train, bus, etc)
 */
class gui_halt_type_images_t : public gui_aligned_container_t
{
	halthandle_t halt;
	gui_image_t img_transport[9];
public:
	gui_halt_type_images_t(halthandle_t h);

	void draw(scr_coord offset) OVERRIDE;
};


/**
 * Main class: the station info window.
 * Window with destination information for a stop
 */
class halt_info_t : public gui_frame_t, private action_listener_t
{
private:

	gui_aligned_container_t *container_top;
	gui_label_buf_t lb_capacity[3], lb_happy[3];
	gui_colorbox_t indicator_color;
	gui_image_t img_enable[3];
	gui_halt_type_images_t *img_types;

	/**
	* Buffer for freight info text string.
	*/
	cbuffer_t freight_info;

	// departure stuff (departure and arrival times display)
	gui_departure_board_t *departure_board;

	// halt connections plane
	gui_halt_detail_t *halt_detail;

	// other UI definitions
	gui_aligned_container_t container_freight, container_chart;
	gui_textarea_t text_freight;
	gui_scrollpane_t scrolly_freight, scrolly_departure, scrolly_details;

	gui_textinput_t input;
	gui_chart_t chart;
	location_view_t view;
	button_t sort_button;

	gui_button_to_chart_array_t button_to_chart;

	gui_tab_panel_t switch_mode;

	halthandle_t halt;
	char edit_name[256];

	void update_components();

	void init(halthandle_t halt);

public:
	halt_info_t(halthandle_t halt = halthandle_t());

	virtual ~halt_info_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 */
	const char *get_help_filename() const OVERRIDE {return "station.txt";}

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	koord3d get_weltpos(bool) OVERRIDE;

	bool is_weltpos() OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE;

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_halt_info; }
};

#endif
