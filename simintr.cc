/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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

#include "boden/wasser.h"

static karte_t *welt_modell = NULL;
static karte_ansicht_t *welt_ansicht = NULL;


static long last_time;
static bool enabled = false;

#define FRAME_TIME_MULTI (16)

// pause between two frames
static long frame_time = 36*FRAME_TIME_MULTI;


bool reduce_frame_time()
{
	if(frame_time > 25*FRAME_TIME_MULTI) {
		frame_time --;
		return true;
	} else {
		frame_time = 25*FRAME_TIME_MULTI;
		return false;
	}
}

bool increase_frame_time()
{
	if(frame_time < 255*FRAME_TIME_MULTI) {
		frame_time ++;
		return true;
	} else {
		return false;
	}
}

long get_frame_time()
{
	return frame_time/FRAME_TIME_MULTI;
}

void set_frame_time(long time)
{
	frame_time = clamp( time, 10, 250 )*FRAME_TIME_MULTI;
}

void
intr_refresh_display(bool dirty)
{
	wasser_t::prepare_for_refresh();
	dr_prepare_flush();
	welt_ansicht->display( dirty );
	win_display_flush(welt_modell->get_active_player()->gib_konto_als_double());
	dr_flush();
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
	const long now = dr_time();
	if((now-last_time)*FRAME_TIME_MULTI < frame_time) {
		return;
	}
	if(enabled) {
		const long diff = ((now - last_time)*welt_modell->get_time_multiplier())/16;
		if(diff>0) {
			enabled = false;
			last_time = now;
			welt_modell->sync_step( diff, !welt_modell->is_fast_forward(), true );
			enabled = true;
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

long
intr_get_last_time()
{
	return last_time;
}


void intr_disable()
{
	enabled = false;
}

void intr_enable()
{
	enabled = true;
}
