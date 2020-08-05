/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
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

#include "simworld.h"
#include "display/simview.h"

#include "boden/wasser.h"

static karte_t *welt_modell = NULL;
static main_view_t *welt_ansicht = NULL;


static long last_time;
static bool enabled = false;

#define FRAME_TIME_MULTI (16)

// pause between two frames
static long frame_time = 36*FRAME_TIME_MULTI;


bool reduce_frame_time()
{
	if(frame_time > 25*FRAME_TIME_MULTI) {
		frame_time -= 1;
		if(  frame_time>150*FRAME_TIME_MULTI  ) {
			frame_time -= 8;
		}
		return true;
	}
	else {
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

void intr_refresh_display(bool dirty)
{
	wasser_t::prepare_for_refresh();
	dr_prepare_flush();
	welt_ansicht->display( dirty );
	if(  env_t::player_finance_display_account  ) {
		win_display_flush( (double)welt_modell->get_active_player()->get_finance()->get_account_balance()/100.0 );
	}
	else {
		win_display_flush( (double)welt_modell->get_active_player()->get_finance()->get_netwealth()/100.0 );
	}
	// with a switch statement more types could be supported ...
	dr_flush();
}


void interrupt_check()
{
	interrupt_check( "0" );
}



// debug version with caller information
void interrupt_check(const char* caller_info)
{
	DBG_DEBUG4("interrupt_check", "called from (%s)", caller_info);
	if(enabled) {
		static uint32 last_ms = 0;
		if(  !welt_modell->is_fast_forward()  ||  welt_modell->get_ticks() != last_ms  ) {
			const long now = dr_time();
			if((now-last_time)*FRAME_TIME_MULTI < frame_time) {
				return;
			}
			const sint32 diff = ((now - last_time)*welt_modell->get_time_multiplier())/16;
			if(  diff>0  ) {
				enabled = false;
				last_time = now;
				welt_modell->sync_step( diff, !welt_modell->is_fast_forward(), true );
				enabled = true;
			}
		}
		last_ms = welt_modell->get_ticks();
	}
	(void)caller_info;
}


void intr_set(karte_t *welt, main_view_t *view)
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
void intr_set_last_time(long time)
{
	last_time = time;
}

long intr_get_last_time()
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

// returns a time string in the desired format
char const *tick_to_string( sint64 ticks, bool show_full )
{
	static sint32 tage_per_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	static char const* const seasons[] = { "q2", "q3", "q4", "q1" };
	static char time [128];

	sint32 month = welt_modell->get_last_month();
	sint32 year = welt_modell->get_last_year();

	time[0] = 0;

	// calculate right month first
	const uint32 ticks_this_month = ticks % welt_modell->ticks_per_world_month;
	month += ticks_this_month / welt_modell->ticks_per_world_month;
	while(  month>11  ) {
		month -= 12;
		year ++;
	}
	while(  month<0  ) {
		month += 12;
		year --;
	}

	uint32 tage, hours, minuten;
	char ticks_as_clock[32], month_as_clock[32];
	if (env_t::show_month > env_t::DATE_FMT_MONTH && env_t::show_month < env_t::DATE_FMT_INTERNAL_MINUTE) {
		tage = (((sint64)ticks_this_month*tage_per_month[month]) >> welt_modell->ticks_per_world_month_shift) + 1;
		hours = (((sint64)ticks_this_month*tage_per_month[month]) >> (welt_modell->ticks_per_world_month_shift-16));
		minuten = (((hours*3) % 8192)*60)/8192;
		hours = ((hours*3) / 8192)%24;
	}
	else if (env_t::show_month == env_t::DATE_FMT_INTERNAL_MINUTE || env_t::show_month == env_t::DATE_FMT_JAPANESE_INTERNAL_MINUTE) {
		world()->sprintf_ticks(ticks_as_clock, sizeof(ticks_as_clock), ticks_this_month);
		world()->sprintf_ticks(month_as_clock, sizeof(month_as_clock), world()->ticks_per_world_month);
		tage = hours = minuten = 0;
	}
	else {
		tage = 0;
		hours = (ticks_this_month * 24) >> welt_modell->ticks_per_world_month_shift;
		minuten = ((ticks_this_month * 24 * 60) >> welt_modell->ticks_per_world_month_shift)%60;
	}

	if(  show_full  ||  env_t::show_month == env_t::DATE_FMT_SEASON  ) {

		//DBG_MESSAGE("env_t::show_month","%d",env_t::show_month);
		// @author hsiegeln - updated to show month
		// @author prissi - also show date if desired
		// since seasons 0 is always summer for backward compatibility
		char const* const date = translator::get_date(year, month, tage, translator::translate(seasons[welt_modell->get_season()]));
		switch(env_t::show_month) {
			case env_t::DATE_FMT_US:
			case env_t::DATE_FMT_US_NO_SEASON: {
				uint32 hours_ = hours % 12;
				if (hours_ == 0) hours_ = 12;
				sprintf(time, "%s %2d:%02d%s", date, hours_, minuten, hours < 12 ? "am" : "pm");
				break;
			}
			case env_t::DATE_FMT_SEASON:
				sprintf(time, "%s", date);
				break;
			case env_t::DATE_FMT_JAPANESE:
			case env_t::DATE_FMT_JAPANESE_NO_SEASON:
				sprintf(time, "%s %2d:%02d", date, hours, minuten);
				break;
			default:
				sprintf(time, "%s %2d:%02dh", date, hours, minuten);
				break;
			case env_t::DATE_FMT_INTERNAL_MINUTE:
			case env_t::DATE_FMT_JAPANESE_INTERNAL_MINUTE:
				sprintf(time, "%s %s/%s", date, ticks_as_clock, month_as_clock);
				break;
		}
	}
	else {
		if(  ticks == 0  ) {
			return translator::translate("now");
		}

		// suppress as much as possible, assuming this is an relative offset to the current month
		sint32 num_days = ( ticks * (env_t::show_month==env_t::DATE_FMT_MONTH? 3 : tage_per_month[month]) ) >> welt_modell->ticks_per_world_month_shift;
		num_days -= ( (welt_modell->get_ticks() % welt_modell->ticks_per_world_month) * (env_t::show_month==env_t::DATE_FMT_MONTH? 3 : tage_per_month[month]) ) >> welt_modell->ticks_per_world_month_shift;
		char days[64];
		static struct lang_info_t
		{
			int language;
			const char* day;
			const char* days;
			int max_strlen;

			lang_info_t() : language(-1), day(NULL), days(NULL) {}

			void update()
			{
				int lang = translator::get_language();
				if (language != lang)
				{
					day = translator::translate("day");
					days = translator::translate("days");
					max_strlen = max(strlen(day), strlen(days));
					language = lang;
				}
			}
		} lang_info;

		lang_info.update();

		switch (num_days) {
			case 0:
				sprintf( days, "  %*s ", lang_info.max_strlen, "");
				break;

			case 1:
				sprintf( days, "+1 %*s ", lang_info.max_strlen, lang_info.day);
				break;

			default:
				sprintf( days, "%+i %*s ", num_days, lang_info.max_strlen, lang_info.days);
				break;
		}

		// maybe round minutes
		//int switchtick = welt_modell->ticks_per_world_month_shift;
		//if(  env_t::show_month == env_t::DATE_FMT_MONTH  ) {
		//	// since a month is then just three days instead of about 30 ...
		//	switchtick += 3;
		//}
		//if(  switchtick <= 19  ) {
		//	minuten = ( (minuten + 30 ) / 60 ) * 60;
		//	hours += minuten /60;
		//	if(  switchtick < 18 ) {
		//		// four hour intervals
		//		hours = (hours + 3 ) & 0xFFFFC;
		//	}
		//	else if(  switchtick == 18 ) {
		//		// two hour intervals
		//		hours = (hours + 1 ) & 0xFFFFE;
		//	}
		//}
		//else if(  switchtick == 20  ) {
		//	minuten = ( (minuten + 15) / 30 ) * 30;
		//}
		//else if(  switchtick == 21  ) {
		//	minuten = ( (minuten + 7) / 15 ) * 15;
		//}
		//else if(  switchtick == 22  ) {
			minuten = ( (minuten + 2) / 5 ) * 5;
		//}
		// take care of overflow
		hours += (minuten / 60);
		minuten %= 60;
		hours %= 24;

		switch(env_t::show_month) {
			case env_t::DATE_FMT_US:
			case env_t::DATE_FMT_US_NO_SEASON: {
				uint32 hours_ = hours % 12;
				if (hours_ == 0) hours_ = 12;
				sprintf(time, "%s% 2d:%02d%s", days, hours_, minuten, hours < 12 ? "am" : "pm");
				break;
			}

			default:
				sprintf(time, "%s% 2d:%02dh", days, hours, minuten);
				break;
		}
	}
	return time;
}
