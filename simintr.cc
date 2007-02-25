/*
 * simintr.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "simdebug.h"
#include "simsys.h"
#include "simintr.h"
#include "simwin.h"
#include "simplay.h"

#include "simworld.h"
#include "simview.h"


static karte_t *welt_modell = NULL;
static karte_ansicht_t *welt_ansicht = NULL;


static long last_time;
static bool enabled = false;


static long frame_time = 36;
static long actual_frame_time = 30;



bool reduce_frame_time()
{
	if(frame_time > 10) {
		frame_time --;
		return true;
	} else {
		frame_time = 10;
		return false;
	}
}

bool increase_frame_time()
{
	if(frame_time < 255) {
		frame_time ++;
		return true;
	} else {
		return false;
	}
}

long get_frame_time()
{
	return frame_time;
}

long get_actual_frame_time()
{
	return actual_frame_time;
}


void
intr_refresh_display(bool dirty)
{
	welt_ansicht->display( dirty );
	win_display_flush(welt_modell->get_active_player()->gib_konto_als_double());
}


void
interrupt_check()
{
	interrupt_check( "0" );
}



// debug version with caller information
void interrupt_check(const char* caller_info)
{
	static const char * last_caller = "program start";
	static long last_call_time = 0;

	const long now = dr_time();
	if(now-last_call_time<frame_time) {
		return;
	}
	if(enabled) {
		const long diff = ((now - last_time)*welt_modell->get_time_multiplier())/16;
		if(diff>0) {
			enabled = false;
			last_time = now;
			welt_modell->sync_step( diff );
			enabled = true;
			last_call_time = now;
		}
	}
	last_caller = caller_info;
}


void
intr_set(karte_t *welt, karte_ansicht_t *view)
{
	welt_modell = welt;
	welt_ansicht = view;
	last_time = dr_time();
	enabled = true;
}

/**
 * currently only used by the pause tool. Use with care!
 * @author Hj. Malthaner
 */
void
intr_set_last_time(long time)
{
	last_time = time;
}


extern "C" void
intr_disable()
{
	enabled = false;
}

extern "C" void
intr_enable()
{
	enabled = true;
}
