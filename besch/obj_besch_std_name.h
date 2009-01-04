#ifndef OBJ_BESCH_STD_NAME_H
#define OBJ_BESCH_STD_NAME_H

#include "text_besch.h"


/**
 * Common base class for all object descriptors, which get their name and
 * copyright information from child 0 and 1
 */
class obj_besch_std_name_t : public obj_besch_t {
	public:
		const char* get_name() const
		{
			return static_cast<const text_besch_t*>(get_kind(0))->get_text();
		}

		const char* get_copyright() const
		{
			const obj_besch_t* os = get_kind(1);
			return os != 0 ? static_cast<const text_besch_t*>(os)->get_text() : 0;
		}
};

#endif
