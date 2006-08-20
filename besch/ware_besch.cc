#include "ware_besch.h"



static const char * catg_names[32] = {
  "special freight",
  "piece goods",
  "bulk goods",
  "oil/gasoline",
  "cooled goods",
  "liquid food",
  "long goods",
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
 * @author Hj. Malthaner
 */
const char * ware_besch_t::gib_catg_name() const
{
  return catg_names[catg & 31];
}
