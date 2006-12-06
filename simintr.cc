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
#include "simtime.h"
#include "simintr.h"
#include "simwin.h"
#include "simplay.h"

#include "simworld.h"
#include "simview.h"

static karte_t *welt_modell = NULL;
static karte_ansicht_t *welt_ansicht = NULL;


static int base_refresh = 1;
static int refresh_counter = 0;

//#define SHOW_TIME

//static long sim_time;
static long last_time;
static bool enabled = false;


static long frame_time = 36;
static long actual_frame_time = 30;
static long average_frame_time = 30;



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

long get_average_frame_time()
{
    return average_frame_time;
}



void
intr_refresh_display(bool dirty)
{
	welt_ansicht->display( dirty );
	win_display_flush(welt_modell->get_active_player()->gib_konto_als_double());
}


void
intr_routine(long delta_t)
{
	refresh_counter --;
	if(refresh_counter <= 0) {
		refresh_counter = base_refresh;
		intr_refresh_display( false );
	}

//	welt_modell->sync_prepare();
	welt_modell->sync_step( delta_t );
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

	const long now = get_system_ms();
	if(now-last_call_time<frame_time) {
		return;
	}
	if(enabled) {
		enabled = false;
		const long diff = now - last_time;
	      last_time = now;
		intr_routine(diff);
		enabled = true;
		last_call_time = get_system_ms();
	}
	last_caller = caller_info;
}


void
intr_set(karte_t *welt, karte_ansicht_t *view, int refresh)
{
	welt_modell = welt;
	welt_ansicht = view;
	base_refresh = refresh;
	refresh_counter = refresh;
	last_time = get_system_ms();
	enabled = true;
}

void
intr_set(karte_t *welt, karte_ansicht_t *view)
{
    intr_set(welt, view, base_refresh);
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
