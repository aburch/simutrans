/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_halt_detail_h
#define gui_halt_detail_h

#include "gui_frame.h"
#include "components/gui_scrollpane.h"

#include "../halthandle_t.h"
#include "../utils/cbuffer_t.h"
#include "simwin.h"


/**
 * Window with destination information for a stop
 * @author Hj. Malthaner
 */

class halt_detail_t : public gui_frame_t
{
private:
	halthandle_t halt;
	uint8 destination_counter;	// last destination counter of the halt; if mismatch to current, then redraw destinations
	uint32 cached_line_count;
	uint32 cached_convoy_count;

	gui_aligned_container_t cont;
	gui_scrollpane_t scrolly;

	void init(halthandle_t halt);

	void halt_detail_info();

	void insert_empty_row();
	void insert_show_nothing();

public:
	halt_detail_t(halthandle_t halt = halthandle_t());

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const { return "station_details.txt"; }

	// only defined to update schedule, if changed
	void draw( scr_coord pos, scr_size size );

	void rdwr( loadsave_t *file );

	uint32 get_rdwr_id() { return magic_halt_detail; }

	// force reset on rotate
	virtual void map_rotate90(sint16) { cached_line_count = 1<<17;}
};

#endif
