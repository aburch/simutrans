/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>

#include "simdebug.h"
#include "sys/simsys.h"
#include "simintr.h"
#include "gui/simwin.h"
#include "player/simplay.h"
#include "player/finance.h"
#include "dataobj/environment.h"
#include "dataobj/translator.h"

#include "world/simworld.h"
#include "display/simview.h"

#include "ground/wasser.h"

static main_view_t *main_view = NULL;


static uint32 last_time;
static bool enabled = false;

#define FRAME_TIME_MULTI (16)

// pause between two frames
static uint32 frame_time = 36*FRAME_TIME_MULTI;


bool reduce_frame_time()
{
	if(  frame_time > 150*FRAME_TIME_MULTI  ) { // < ~6.6fps
		frame_time -= 8;
		return true;
	}
	else if(frame_time > (FRAME_TIME_MULTI*1000)/env_t::max_fps) {
		frame_time -= 1;
		return true;
	}
	else {
		frame_time = (FRAME_TIME_MULTI*1000)/env_t::max_fps;
		return false;
	}
}


bool increase_frame_time()
{
	if(frame_time > (FRAME_TIME_MULTI*1000)/5) { // < 5 fps
		return false;
	}
	else {
		frame_time ++;
		return true;
	}
}


uint32 get_frame_time()
{
	return frame_time/FRAME_TIME_MULTI;
}


void set_frame_time(uint32 time)
{
	frame_time = clamp( time, 1000/env_t::max_fps, 1000/env_t::min_fps )*FRAME_TIME_MULTI;
}


void intr_refresh_display(bool dirty)
{
	wasser_t::prepare_for_refresh();
	dr_prepare_flush();
	main_view->display( dirty );
	if(  env_t::player_finance_display_account  ) {
		win_display_flush( (double)world()->get_active_player()->get_finance()->get_account_balance()/100.0 );
	}
	else {
		win_display_flush( (double)world()->get_active_player()->get_finance()->get_netwealth()/100.0 );
	}
	// with a switch statement more types could be supported ...
	dr_flush();
}


// debug version with caller information
void interrupt_check(const char* caller_info)
{
	DBG_DEBUG4("interrupt_check", "called from (%s)", caller_info);
	(void)caller_info;

	if(  !enabled  ) {
		return;
	}

	static uint32 last_ms = 0;
	if(  !world()->is_fast_forward()  ||  world()->get_ticks() != last_ms  ) {
		const uint32 now = dr_time();
		if((now-last_time)*FRAME_TIME_MULTI < frame_time) {
			return;
		}

		const sint32 diff = (( (sint32)now - (sint32)last_time)*world()->get_time_multiplier())/16;
		if(  diff>0  ) {
			enabled = false;
			last_time = now;
			if (!world()->is_fast_forward()) {
				world()->sync_step( diff );
			}
			world()->display(diff);
			// since pause maz have been activated in this sync_step
			enabled = !world()->is_paused();
		}
	}
	last_ms = world()->get_ticks();
}


void intr_set_view(main_view_t *view)
{
	main_view = view;
	last_time = dr_time();
	enabled = true;
}


void intr_set_last_time(uint32 time)
{
	last_time = time;
}


void intr_disable()
{
	enabled = false;
}


void intr_enable()
{
	enabled = true;
}


char const *tick_to_string( uint32 ticks, bool only_DDMMHHMM )
{
	static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	static char const* const seasons[] = { "q2", "q3", "q4", "q1" };
	static char time [128];

	time[0] = 0;

	if (world()== NULL) {
		return time;
	}

	sint32 month = world()->get_last_month();
	sint32 year = world()->get_last_year();

	// calculate right month first
	const uint32 ticks_this_month = ticks % world()->ticks_per_world_month;
	month += ticks_this_month / world()->ticks_per_world_month;
	while(  month>11  ) {
		month -= 12;
		year ++;
	}
	while(  month<0  ) {
		month += 12;
		year --;
	}

	uint32 tage, hours, minuten;
	if (env_t::show_month > env_t::DATE_FMT_MONTH) {
		tage = (((sint64)ticks_this_month*tage_per_month[month]) >> world()->ticks_per_world_month_shift) + 1;
		hours = (((sint64)ticks_this_month*tage_per_month[month]) >> (world()->ticks_per_world_month_shift-16));
		minuten = (((hours*3) % 8192)*60)/8192;
		hours = ((hours*3) / 8192)%24;
	}
	else {
		tage = 0;
		hours = (ticks_this_month * 24) >> world()->ticks_per_world_month_shift;
		minuten = ((ticks_this_month * 24 * 60) >> world()->ticks_per_world_month_shift)%60;
	}

	//DBG_MESSAGE("env_t::show_month","%d",env_t::show_month);
	// since seasons 0 is always summer for backward compatibility
	char const* const date = only_DDMMHHMM ?
		translator::get_day_date(tage) :
		translator::get_date(year, month, tage, translator::translate(seasons[world()->get_season()]));
	switch (env_t::show_month) {
	case env_t::DATE_FMT_US:
	case env_t::DATE_FMT_US_NO_SEASON: {
		uint32 hours_ = hours % 12;
		if (hours_ == 0) hours_ = 12;
		sprintf(time, "%s%2d:%02d%s", date, hours_, minuten, hours < 12 ? "am" : "pm");
		break;
	}
	case env_t::DATE_FMT_SEASON:
		sprintf(time, "%s", date);
		break;
	default:
		sprintf(time, "%s%2d:%02dh", date, hours, minuten);
		break;
	}
	return time;
}


char const *difftick_to_string( sint32 ticks, bool round_to_quaters )
{
	static char time [128];

	time[0] = 0;

	// World model might not be initalized if this is called while reading saved windows.
	if ( world() == NULL) {
		return time;
	}

	// suppress as much as possible, assuming this is an relative offset to the current month
	sint32 num_days = ( ticks * (env_t::show_month==env_t::DATE_FMT_MONTH? 1 : 31) ) >> world()->ticks_per_world_month_shift;
	char days[64];
	days[0] = 0;
	if(  num_days!=0  ) {
		sprintf( days, "%+i ", num_days );
	}

	uint32 hours, minuten;
	if( env_t::show_month > env_t::DATE_FMT_MONTH ) {
		if( world()->ticks_per_world_month_shift>=16 ) {
			hours = (((sint64)ticks*31) >> (world()->ticks_per_world_month_shift-16));
		}
		else {
			hours = (((sint64)ticks*31) << (16-world()->ticks_per_world_month_shift));
		}
	}
	else {
		if( world()->ticks_per_world_month_shift>=16 ) {
			uint32 precision = round_to_quaters ? 0 : 15;
			hours = (ticks >> (world()->ticks_per_world_month_shift-16)) + precision;
		}
		else {
			uint32 precision = round_to_quaters ? 0 : 15 >> (16-world()->ticks_per_world_month_shift);
			hours = (((sint64)ticks) << (16-world()->ticks_per_world_month_shift)) + precision;
		}
	}
	minuten = (((hours * 3) % 8192) * 60) / 8192;
	hours = ((hours * 3) / 8192) % 24;

	// maybe round minutes
	if( round_to_quaters ) {
		int switchtick = world()->ticks_per_world_month_shift;
		if(  env_t::show_month == env_t::DATE_FMT_MONTH  ) {
			// since a month is then just three days instead of about 30 ...
			switchtick += 3;
		}
		if(  switchtick <= 19  ) {
			minuten = ( (minuten + 30 ) / 60 ) * 60;
			hours += minuten /60;
			if(  switchtick < 18 ) {
				// four hour intervals
				hours = (hours + 3 ) & 0xFFFFC;
			}
			else if(  switchtick == 18 ) {
				// two hour intervals
				hours = (hours + 1 ) & 0xFFFFE;
			}
		}
		else if(  switchtick == 20  ) {
			minuten = ( (minuten + 15) / 30 ) * 30;
		}
		else if(  switchtick == 21  ) {
			minuten = ( (minuten + 7) / 15 ) * 15;
		}
		else if(  switchtick == 22  ) {
			minuten = ( (minuten + 2) / 5 ) * 5;
		}
	}
	// take care of overflow
	hours += (minuten / 60);
	minuten %= 60;
	hours %= 24;

	switch(env_t::show_month) {
	case env_t::DATE_FMT_GERMAN:
	case env_t::DATE_FMT_GERMAN_NO_SEASON:
	case env_t::DATE_FMT_US:
	case env_t::DATE_FMT_US_NO_SEASON:
		sprintf(time, "%s%2d:%02dh", days, hours, minuten);
		break;

	case env_t::DATE_FMT_JAPANESE:
	case env_t::DATE_FMT_JAPANESE_NO_SEASON:
		sprintf(time, "%s%2d:%02dh", days, hours, minuten);
		break;

	case env_t::DATE_FMT_MONTH:
		sprintf(time, "%s%2d:%02dh", days, hours, minuten);
		break;
	}
	return time;
}
