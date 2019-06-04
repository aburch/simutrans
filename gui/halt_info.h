/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Window with destination information for a stop
 * @author Hj. Malthaner
 */

#ifndef gui_halt_info_h
#define gui_halt_info_h

#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_button.h"
#include "components/gui_location_view_t.h"
#include "components/action_listener.h"
#include "components/gui_chart.h"
#include "components/gui_combobox.h"

#include "../utils/cbuffer_t.h"
#include "../simhalt.h"
#include "../gui/simwin.h"


class halt_info_t : public gui_frame_t, private action_listener_t
{
private:

	/**
	* Buffer for freight info text string.
	* @author Hj. Malthaner
	*/
	cbuffer_t freight_info;
	cbuffer_t info_buf, joined_buf, tooltip_buf;

	// other UI definitions
	gui_scrollpane_t scrolly;
	gui_textarea_t text;
	gui_textinput_t input;
	gui_chart_t chart;
	gui_label_t sort_label;
	location_view_t view;
	button_t button;
	// button_t sort_button;     // @author hsiegeln
	button_t filterButtons[MAX_HALT_COST];
	button_t toggler, toggler_departures;
	sint16 chart_total_size;

	gui_combobox_t freight_sort_selector;

	halthandle_t halt;
	char edit_name[256];

	void show_hide_statistics( bool show );

	char modified_name[320];

	void show_hide_classes(bool show);

	// departure stuff (departure and arrival times display)
	class dest_info_t {
	public:
		bool compare( const dest_info_t &other ) const;
		halthandle_t halt;
		sint32 delta_ticks;
		convoihandle_t cnv;
		dest_info_t() : delta_ticks(0) {}
		dest_info_t( halthandle_t h, sint32 d_t, convoihandle_t c ) : halt(h), delta_ticks(d_t), cnv(c) {}
		bool operator == (const dest_info_t &other) const { return ( this->cnv==other.cnv ); }
	};

	static bool compare_hi(const dest_info_t &a, const dest_info_t &b) { return a.delta_ticks <= b.delta_ticks; }

	vector_tpl<dest_info_t> destinations;
	vector_tpl<dest_info_t> origins;
	cbuffer_t departure_buf;

	void update_departures();

	void show_hide_departures( bool show );

public:
	enum sort_mode_t { by_destination = 0, by_via = 1, by_amount_via = 2, by_amount = 3, by_origin = 4, by_origin_sum = 5, by_destination_detil = 6, by_class_detail = 7, by_class_via = 8, SORT_MODES = 9 };
//	enum sort_mode_t { by_destination = 0, by_via = 1, by_amount_via = 2, by_amount = 3, by_origin = 4, by_origin_sum = 5, by_destination_detil = 6, by_transfer_time = 7, SORT_MODES = 8 };

	halt_info_t(halthandle_t halt);

	virtual ~halt_info_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const {return "station.txt";}

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size);

	/**
	 * Set window size and adjust component sizes and/or positions accordingly
	 * @author Hj. Malthaner
	 */
	virtual void set_windowsize(scr_size size);

	virtual koord3d get_weltpos(bool);

	virtual bool is_weltpos();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void map_rotate90( sint16 );

	// this constructor is only used during loading
	halt_info_t();

	void rdwr( loadsave_t *file );

	uint32 get_rdwr_id() { return magic_halt_info; }
};

#endif
