#ifndef XREF_BESCH_H
#define XREF_BESCH_H

#include "obj_besch.h"
#include "objversion.h"


class xref_besch_t : public obj_besch_t
{
	public:
		const char* get_name() const { return name; }

		using obj_besch_t::operator new;

	private:
		obj_type type;
		bool fatal;
		char name[];

	friend class xref_reader_t;
};

#endif
