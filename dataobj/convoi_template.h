/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_CONVOI_TEMPLATE_H
#define DATAOBJ_CONVOI_TEMPLATE_H

#include <string>
#include <vector>
#include "../tpl/vector_tpl.h"

struct convoi_template_t {
	std::string name;
	std::string source_file;            // tab file this template was loaded from
	std::vector<std::string> vehicles;  // pak descriptor names
};

// Load convoy templates into `out` from pak_dir/convoy_template/ and
// (when load_addons is true) addons/(pak)/convoy_template/.
void convoi_template_load(const std::string &pak_dir, bool load_addons, vector_tpl<convoi_template_t> &out);

#endif
