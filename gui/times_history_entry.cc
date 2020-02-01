/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "times_history_entry.h"

#include "../display/simgraph.h"
#include "../dataobj/translator.h"

times_history_entry_t::times_history_entry_t(times_history_data_t *history_) : history(history_) {
	for (int i = 0; i < TIMES_HISTORY_SIZE; i++) {
		uint32 time = history->get_entry(i);
		if (time != 0) welt->sprintf_time_tenths(times_str[i], 32, time);
		else strcpy(times_str[i], translator::translate("no_time_entry"));
	}
	uint32 time = history->get_average_seconds();
	if (time != 0) welt->sprintf_time_secs(average_time_str, 32, time);
	else strcpy(average_time_str, translator::translate("no_time_entry"));

	size.w = TIMES_HISTORY_ENTRY_WIDTH;
	size.h = LINESPACE;
}

void times_history_entry_t::draw(scr_coord offset) {
	for (int i = 0; i < TIMES_HISTORY_SIZE; i++) {
		display_proportional_clip_rgb(pos.x + offset.x + TIME_TEXT_WIDTH * (i + 1), pos.y + offset.y, times_str[i], ALIGN_RIGHT, SYSCOL_TEXT, true);
	}
	display_proportional_clip_rgb(pos.x + offset.x + TIME_TEXT_WIDTH * TIMES_HISTORY_SIZE + TIME_TEXT_MARGIN + TIME_TEXT_WIDTH, pos.y + offset.y, average_time_str, ALIGN_RIGHT, SYSCOL_TEXT, true);
}
