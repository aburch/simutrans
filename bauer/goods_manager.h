/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BAUER_GOODS_MANAGER_H
#define BAUER_GOODS_MANAGER_H


#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

class goods_desc_t;

/**
 * Factory-Class for Goods.
 *
 * @author Hj. Malthaner
 */
class goods_manager_t
{
private:
	static stringhashtable_tpl<const goods_desc_t *> desc_names;
	static vector_tpl<goods_desc_t *> goods;

	static goods_desc_t *load_passengers;
	static goods_desc_t *load_mail;
	static goods_desc_t *load_none;

	// number of different good classes;
	static uint8 max_catg_index;

public:
	enum { INDEX_PAS=0, INDEX_MAIL=1, INDEX_NONE=2 };

	static const goods_desc_t *passengers;
	static const goods_desc_t *mail;
	static const goods_desc_t *none;

	static bool successfully_loaded();
	static bool register_desc(goods_desc_t *desc);

	static uint8 get_max_catg_index() { return max_catg_index; }

	/**
	* Search the good 'name' information and return
	* its description. Return NULL when the Good is
	* unknown.
	*
	* @param name the non-translated good name
	* @author Hj. Malthaner/V. Meyer
	*/
	static const goods_desc_t *get_info(const char* name);

	static const goods_desc_t *get_info(uint16 idx) { return goods[idx]; }

	static goods_desc_t *get_modifiable_info(uint16 idx) { return goods[idx]; }

	static uint8 get_count() { return (uint8)goods.get_count(); }

	// good by catg
	static const goods_desc_t *get_info_catg(const uint8 catg);

	// good by catg_index
	static const goods_desc_t *get_info_catg_index(const uint8 catg_index);

	// Number of classes for a given category index
	static uint8 get_classes_catg_index(const uint8 catg_index);

	// @return translated class name
	static const char * get_translated_wealth_name(const uint8 catg_index, const uint8 g_class);
	// Currently there is no distinction in the name of class, but in case of distinction it is necessary to separate into the following
	//static const char * get_translated_fare_class_name(const uint8 catg_index, const uint8 g_class);
	//static const char * get_translated_accommodation_class_name(const uint8 catg_index, const uint8 g_class);

	/*
	 * allow to multiply all prices, 1000=1.0
	 * used for the beginner mode
	 */
	static void set_multiplier(sint32 multiplier, uint16 scale_factor);
};

#endif
