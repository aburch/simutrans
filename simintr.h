/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMINTR_H
#define SIMINTR_H


#include "macros.h"

class karte_t;
class main_view_t;


/// Try to increase fps
bool reduce_frame_time();

/// Try to decrease fps
bool increase_frame_time();
sint32 get_frame_time();
void set_frame_time(sint32 ms);


void intr_refresh_display(bool dirty);

void intr_set(karte_t *welt, main_view_t *view);


/**
 * currently only used by the pause tool. Use with care!
 * @author Hj. Malthaner
 */
void intr_set_last_time(sint32 time);
sint32 intr_get_last_time();


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
// Returns an empty string if called before the world model defining time is initalized.
char const *tick_to_string( sint64 ticks, bool show_full );
