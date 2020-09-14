/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
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

private:
	uint32 vehicle_speed;
	uint8 comfort;
	uint8 catering_level;
	uint32 distance_meters;
	uint8 g_class;

	// The number of goods to be displayed. May be less than maximum number of goods possible,
	// if we are filtering to only the goods being produced by factories in the current game.
	int listd_goods;

public:
	goods_stats_t() {}

	// update list and resize
	//void update_goodslist(uint16 *g, uint32 vehicle_speed, int listd_goods, uint32 distance, uint8 comfort, uint8 catering, uint8 g_class);
	void update_goodslist(vector_tpl<const goods_desc_t*>, uint32 vehicle_speed, /*int listd_goods,*/ uint32 distance, uint8 comfort, uint8 catering, uint8 g_class);
};

#endif
