/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "goods_desc.h"


static const char * catg_names[32] = {
	"special freight",
	"CATEGORY_01", // was "piece goods",
	"CATEGORY_02", // was "bulk goods",
	"CATEGORY_03", // was "oil/gasoline",
	"CATEGORY_04", // was "cooled goods",
	"CATEGORY_05", // was "liquid food",
	"CATEGORY_06", // was "long goods",
	"CATEGORY_07",

	"CATEGORY_08",
	"CATEGORY_09",
	"CATEGORY_10",
	"CATEGORY_11",
	"CATEGORY_12",
	"CATEGORY_13",
	"CATEGORY_14",
	"CATEGORY_15",

	"CATEGORY_16",
	"CATEGORY_17",
	"CATEGORY_18",
	"CATEGORY_19",
	"CATEGORY_20",
	"CATEGORY_21",
	"CATEGORY_22",
	"CATEGORY_23",

	"CATEGORY_24",
	"CATEGORY_25",
	"CATEGORY_26",
	"CATEGORY_27",
	"CATEGORY_28",
	"CATEGORY_29",
	"CATEGORY_30",
	"CATEGORY_31",
};


/**
 * @return Name of the category of the good
 */
const char * goods_desc_t::get_catg_name() const
{
	return catg_names[catg & 31];
}
