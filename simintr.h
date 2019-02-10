/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simintr_h
#define simintr_h

#include "macros.h"

class karte_t;
class main_view_t;


bool reduce_frame_time();
bool increase_frame_time();
uint32 get_frame_time();
void set_frame_time(uint32 time);


void intr_refresh_display(bool dirty);

void intr_set(karte_t *welt, main_view_t *view);

void intr_set_last_time(uint32 time);


void intr_enable();
void intr_disable();


// force sync_step (done before sleeping)
void interrupt_force();

void interrupt_check(const char* caller_info = "0");

#if COLOUR_DEPTH == 0 && defined PROFILE
	// 0 bit graphic + profiling: no interrupt_check.
	#define INT_CHECK(info);
#else
	#ifndef DEBUG
		// standard version
		#define INT_CHECK(info) interrupt_check();
	#else
		// debug version
		#define INT_CHECK(info) interrupt_check( __FILE__ ":" QUOTEME(__LINE__) );
	#endif
#endif
#endif


// returns a time string in the desired format
// Returns an empty string if called before the world model defining time is initalized.
char const *tick_to_string( sint32 ticks, bool show_full );
