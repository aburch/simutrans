/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GROUND_INFO_H
#define GUI_GROUND_INFO_H


#include "gui_frame.h"
#include "components/gui_location_view.h"
#include "obj_info.h"

class grund_t;

class grund_info_t : public base_infowin_t
{
protected:
	const grund_t* gr;

	location_view_t lview;

public:
	grund_info_t(const grund_t* _gr);

	koord3d get_weltpos(bool) OVERRIDE;

	bool is_weltpos() OVERRIDE;

	void fill_buffer();

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	void map_rotate90( sint16 ) OVERRIDE;
};

#endif
