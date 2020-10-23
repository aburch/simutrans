/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_LABELLIST_STATS_T_H
#define GUI_LABELLIST_STATS_T_H


#include "components/gui_aligned_container.h"
#include "components/gui_label.h"
#include "components/gui_scrolled_list.h"


namespace labellist {
	enum sort_mode_t {
		by_name = 0,
		by_koord,
		by_player,
		SORT_MODES
	};
};

class label_t;

class labellist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	koord label_pos;
	gui_label_buf_t label;

	const label_t* get_label() const;
public:
	static labellist::sort_mode_t sortby;
	static bool sortreverse, filter;

	static bool compare(const gui_component_t *a, const gui_component_t *b );

	labellist_stats_t(koord label_pos);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	*/
	void draw(scr_coord offset) OVERRIDE;

	void map_rotate90( sint16 );

	bool is_valid() const OVERRIDE;

	const char* get_text() const OVERRIDE;
};

#endif
