/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef DESCRIPTOR_TEXT_DESC_H
#define DESCRIPTOR_TEXT_DESC_H


#include "obj_desc.h"


/*
 *  Autor:
 *      Volker Meyer
 */
class text_desc_t : public obj_desc_t {
public:
		const char* get_text() const { return text; }

		using obj_desc_t::operator new;

	private:
		char text[];

	friend class text_reader_t;
};

#endif
