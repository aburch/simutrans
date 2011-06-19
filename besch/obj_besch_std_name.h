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
			return get_child<text_besch_t>(0)->get_text();
		}

		const char* get_copyright() const
		{
			text_besch_t const* const ts = get_child<text_besch_t>(1);
			if (!ts) return 0;
			char const* const text = ts->get_text();
			return text[0] != '\0' ? text : 0;
		}
};

#endif
