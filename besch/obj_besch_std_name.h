#ifndef OBJ_BESCH_STD_NAME_H
#define OBJ_BESCH_STD_NAME_H

#include "text_besch.h"


/**
 * Common base class for all object descriptors, which get their name and
 * copyright information from child 0 and 1
 */
class obj_besch_std_name_t : public obj_besch_t {
	public:
		const char* gib_name() const
		{
			return static_cast<const text_besch_t*>(gib_kind(0))->gib_text();
		}

		const char* gib_copyright() const
		{
			const obj_besch_t* os = gib_kind(1);
			return os != 0 ? static_cast<const text_besch_t*>(os)->gib_text() : 0;
		}
};

#endif
