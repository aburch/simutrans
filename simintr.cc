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
#include "simworldview.h"

static karte_t *welt_modell = NULL;
static karte_vollansicht_t *welt_ansicht = NULL;


static int base_refresh = 1;
static int refresh_counter = 0;

//#define SHOW_TIME

static long last_time;
static bool enabled = false;


static long frame_time = 36;
static long actual_frame_time = 30;
static long average_frame_time = 30;

static int late_frames = 0;


bool reduce_frame_time()
{
    if(frame_time > 28 &&
       frame_time > average_frame_time+8 &&
       late_frames < 1) {
	frame_time -= 8;
//	printf("*** erniedrige frame_time auf %ld\n", frame_time);
        return true;
    } else {
	return false;
    }
}

bool increase_frame_time()
{
    if(frame_time < 124) {
	frame_time += 4;
//	printf("*** erhoehe frame_time auf %ld\n", frame_time);
        return true;
    } else {
	return false;
    }
}

void set_frame_time(long t)
{
    frame_time = t;
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
	win_display_flush(	welt_modell->gib_zeit_ms(), welt_modell->gib_spieler(0)->kennfarbe, welt_modell->gib_spieler(0)->gib_konto_als_double() );
}


void
intr_routine(long delta_t)
{
#ifdef SHOW_TIME
	const long t0 = get_current_time_millis();
#endif

	refresh_counter --;

	if(refresh_counter <= 0) {
	    refresh_counter = base_refresh;

	    intr_refresh_display( false );
	}


#ifdef SHOW_TIME
	const long t1 = get_current_time_millis();
#endif

	welt_modell->sync_prepare();
	welt_modell->sync_step(delta_t);

	const long t3 = get_current_time_millis();

	actual_frame_time = t3 - last_time;
	average_frame_time = (average_frame_time+actual_frame_time) >> 1;

	if(average_frame_time >= frame_time) {
	    late_frames ++;
	    if(late_frames > 2) {
		increase_frame_time();
	    }
	} else {
	    late_frames --;

	    if(late_frames < 0) {
		late_frames = 0;
	    }
	}

#ifdef SHOW_TIME
	DBG_MESSAGE("intr_routine(long delta_t)",
		     "step %ld disp %ld ms, total %ld ms",
		     (long)(t3-t1),(long)(t1-t0),(long)(t3-t0));
#endif
}

void
interrupt_check()
{
    if(enabled) {
	enabled = false;

        const long now = get_current_time_millis();

        const long diff = now - last_time;

//        printf("now %lld delta_t %ld\n", now, diff);

        if(diff > frame_time) {

//            printf("  displaying a frame\n");

            last_time = now;
	    intr_routine(diff);
	}

	enabled = true;
    } else {
	// Hajo: Well, this not really a bug, and it doesn't hurt because the
	// routine is locked after a call but I should inspect that some day
	// fprintf(stderr, "interrupt_check() called while interrupt was running!\n");
    }
}

// debug version with caller information

void
interrupt_check(char *caller_info)
{
    static const char * last_caller = "program start";
    static long last_call_time = 0;

    const long now = get_current_time_millis();


    // check for stalls - timer (sleep) precision is about 10ms

    if(now - last_call_time > 2
       //       || now - last_call_time < 1
       ) {

      if(strncmp(caller_info, "simtime", 7)) {

      DBG_MESSAGE("interrupt_check()",
		   "%ld from %s - %s", now-last_call_time, last_caller, caller_info);

      }
    }

    if(enabled) {
	enabled = false;

	const long diff = now - last_time;

	// standard check
	// printf("interrupt check from %s at %ld\n", caller_info, (long) diff);
	if( diff > frame_time) {

            last_time = now;
	    intr_routine(diff);

	    // standard check
            // printf("now %ld delta_t %ld\n", (long) now, diff);
	}
	enabled = true;
    }

    last_caller = caller_info;
    last_call_time = get_current_time_millis();
}


void
intr_set(karte_t *welt, karte_vollansicht_t *view, int refresh)
{
    welt_modell = welt;
    welt_ansicht = view;

    base_refresh = refresh;

    last_time = get_current_time_millis();
    enabled = true;
}

void
intr_set(karte_t *welt, karte_vollansicht_t *view)
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
