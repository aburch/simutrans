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


// the length of the pause we will give control to the system
static long sleep_time = 36;

long get_sleep_time()
{
	return sleep_time;
}

void set_sleep_time(long new_time)
{
	new_time = abs(new_time);
	if(new_time>25) {
		new_time = 25;
	}
	sleep_time = new_time;
}

bool reduce_sleep_time()
{
	if(sleep_time > 0) {
		sleep_time --;
		return true;
	}
	return false;
}

bool increase_sleep_time()
{
	if(sleep_time < 25) {
		sleep_time ++;
		return true;
	}
	return false;
}



// pause between two frames
static long frame_time = 36;

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

void set_frame_time(long time)
{
	if(time>250) {
		time = 250;
	}
	frame_time = time;
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
	const long now = dr_time();
	if(now-last_time<frame_time) {
		return;
	}
	if(enabled) {
		const long diff = ((now - last_time)*welt_modell->get_time_multiplier())/16;
		if(diff>0) {
			enabled = false;
			last_time = now;
			welt_modell->sync_step( diff );
			if(sleep_time>0) {
				dr_sleep( sleep_time );
			}
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


