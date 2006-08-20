/*
 *
 *  text_besch.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __TEXT_BESCH_H
#define __TEXT_BESCH_H

/*
 *  includes
 */
#include "obj_besch.h"


/*
 *  class:
 *      text_besch_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 */
class text_besch_t : public obj_besch_t {
    friend class text_writer_t;

public:
    const char *gib_text() const
    {
		return reinterpret_cast<const char *>(this + 1);
    }
};

#endif // __TEXT_BESCH_H
