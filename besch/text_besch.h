/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 */
#ifndef __TEXT_BESCH_H
#define __TEXT_BESCH_H

#include "obj_besch.h"


/*
 *  Autor:
 *      Volker Meyer
 */
class text_besch_t : public obj_besch_t {
    friend class text_writer_t;

public:
		const char* gib_text() const { return text; }

		using obj_besch_t::operator new;

	private:
		char text[];

	friend class text_reader_t;
};

#endif
