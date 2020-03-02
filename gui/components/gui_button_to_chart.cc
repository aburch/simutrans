/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_button_to_chart.h"
#include "../../dataobj/loadsave.h"
#include "../../tpl/vector_tpl.h"

void gui_button_to_chart_array_t::rdwr(loadsave_t* file)
{
	uint32 count = array.get_count();
	file->rdwr_long(count);
	bool press;
	sint32 curve;

	for(uint32 i=0; i<count; i++) {
		if (i < array.get_count()) {
			press = array[i]->get_button()->pressed;
			curve = array[i]->get_curve();
		}
		file->rdwr_bool(press);
		file->rdwr_long(curve);
		if (i < array.get_count()) {
			if (file->is_loading()) {
				if (press  &&  curve == array[i]->get_curve()) {
					array[i]->get_button()->pressed = press;
					array[i]->update();
				}
			}
		}
	}
}
