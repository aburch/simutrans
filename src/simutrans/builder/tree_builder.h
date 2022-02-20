/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BUILDER_TREE_BUILDER_H
#define BUILDER_TREE_BUILDER_H


#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/weighted_vector_tpl.h"
#include "../tpl/vector_tpl.h"

#include "../utils/plainstring.h"

#include "../dataobj/koord.h"
#include "../display/simimg.h"


class tree_desc_t;
class baum_t;


/// Handles desc management and distribution of trees.
class tree_builder_t
{
private:
	static stringhashtable_tpl<const tree_desc_t *> desc_table;            ///< Mapping desc_name -> desc
	static vector_tpl<const tree_desc_t *> tree_list;                      ///< Mapping tree_id -> desc
	static weighted_vector_tpl<uint8> tree_list_per_climate[MAX_CLIMATES]; ///< index vector into tree_list, accessible per climate

	static vector_tpl<plainstring> loaded_tree_names; ///< Maps loaded_id -> loaded_desc_name

public:
	static void rdwr_tree_ids(loadsave_t *file);

	/// Tree IDs never change after loading paks, so when loading a save
	/// which was saved with a different set of tree paks the tree ID from the save
	/// has to be translated to the one used in the current game.
	/// This is done by mapping
	///   loaded_id -> loaded_desc_name -> real_desc_name -> real_id
	/// This function does the first mapping based on the data stored in the save file.
	/// Returns NULL if no desc name could be found.
	/// @sa boden_t::boden_t
	static const char *get_loaded_desc_name(uint8 loaded_id);

	static const tree_desc_t *get_desc_by_name(const char * tree_name) { return desc_table.get(tree_name); }
	static const tree_desc_t *get_desc_by_id(uint8 tree_id) { return tree_id < get_num_trees() ? tree_list[tree_id] : NULL; }
	static uint8 get_id_by_desc(const tree_desc_t *desc) { return tree_list.index_of(desc); }

	static image_id get_tree_image(uint8 idx, uint32 age, uint8 season);

	static bool has_trees() { return get_num_trees() > 0; }
	static bool has_trees_for_climate(climate cl) { return !tree_list_per_climate[cl].empty(); }

	static sint32 get_num_trees() { return tree_list.get_count()-1; }

	/// Fill rectangular region with trees.
	static void fill_trees(int dichte, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom);

	static bool register_desc(const tree_desc_t *desc);
	static bool successfully_loaded();

	/// @returns list of all registered descriptors
	static const vector_tpl<const tree_desc_t *> &get_all_desc() { return tree_list; }

	/// Plant a new tree in a (2n+1) * (2n+1) square around @p tree.
	static bool spawn_tree_near(const baum_t *tree, int radius = 3);

	static uint32 create_forest(koord center, koord size, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom);

public:
	/// distributes trees in a rectangular region of the map
	static void distribute_trees(int dichte, sint16 xtop, sint16 ytop, sint16 xbottom, sint16 ybottom);

	/// tree planting function - it takes care of checking suitability of area
	static bool plant_tree_on_coordinate(koord pos, const tree_desc_t *desc, const bool check_climate, const bool random_age);

	/// tree planting function - it takes care of checking suitability of area
	static uint8 plant_tree_on_coordinate(koord pos, const uint8 maximum_count, const uint8 count);

	static const tree_desc_t *find_tree( const char *tree_name );

	static const tree_desc_t *random_tree_for_climate(climate cl);

	/// also checks for distribution values
	/// @returns the tree_id, or 0xFFFF if no trees exist for the requested climate.
	static uint16 random_tree_id_for_climate(climate cl);
};


#endif
