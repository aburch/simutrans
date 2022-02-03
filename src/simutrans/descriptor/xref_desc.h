/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_XREF_DESC_H
#define DESCRIPTOR_XREF_DESC_H


#include "obj_desc.h"
#include "objversion.h"


class xref_desc_t : public obj_desc_t
{
	public:
		const char* get_name() const { return name; }

		using obj_desc_t::operator new;

	private:
		obj_type type;
		bool fatal;
		char name[];

	friend class xref_reader_t;
};

#endif
