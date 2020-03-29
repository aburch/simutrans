/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_GOODS_STATS_T_H
#define GUI_GOODS_STATS_T_H


#include "../simtypes.h"
#include "components/gui_aligned_container.h"

template<class T> class vector_tpl;
class goods_desc_t;

class goods_stats_t : public gui_aligned_container_t
{
	static karte_ptr_t welt;

public:
	goods_stats_t() {}

	// update list and resize
	void update_goodslist(vector_tpl<const goods_desc_t*>, int bonus);
};

#endif
