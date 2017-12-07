/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef simintr_h
#define simintr_h

#include "macros.h"

class world_t;
class main_view_t;


bool reduce_frame_time();
bool increase_frame_time();
uint32 get_frame_time();
void set_frame_time(uint32 time);


void intr_refresh_display(bool dirty);

void intr_set(world_t *welt, main_view_t *view);


/**
 * currently only used by the pause tool. Use with care!
 * @author Hj. Malthaner
 */
void intr_set_last_time(uint32 time);
uint32 intr_get_last_time();


void intr_enable();
void intr_disable();


// force sync_step (done before sleeping)
void interrupt_force();

void interrupt_check();
void interrupt_check(const char* caller_info);

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
char const *tick_to_string( sint32 ticks, bool show_full );
