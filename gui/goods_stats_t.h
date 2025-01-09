/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GOODS_STATS_T_H
#define GUI_GOODS_STATS_T_H


#include "../simtypes.h"
#include "components/action_listener.h"
#include "components/gui_aligned_container.h"
#include "../tpl/ptrhashtable_tpl.h"

template<class T> class vector_tpl;
class goods_desc_t;
class gui_combobox_t;

class goods_stats_t : public gui_aligned_container_t, public action_listener_t
{
	static karte_ptr_t welt;

public:
	goods_stats_t() {}

	// update list and resize
	void update_goodslist(vector_tpl<const goods_desc_t*>, int bonus);
	
private:
	// key: the combo box pointer, value: the goods category index of the combo box
	ptrhashtable_tpl<gui_action_creator_t*, uint8> goods_catg_indexes;
	bool action_triggered(gui_action_creator_t*, value_t);
	void draw(scr_coord offset) OVERRIDE;
};

#endif
