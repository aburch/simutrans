/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __OBJ_BESCH_H
#define __OBJ_BESCH_H

#include <cstddef>
#include "../simtypes.h"

class checksum_t;
/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Basis aller besch_t-Klassen, d.h. Beschreibungen, die aus den .pak
 *	Files geladen werden.
 *	Keine virtuellen Methoden erlaubt !
 */
class obj_besch_t {

protected:
	template<typename T> T const* get_child(int i) const { return static_cast<T const*>(node_info[i]); }

	/*
	 * Internal Node information - the derived class knows,
	 * how many node child nodes really exist.
	 */
	obj_besch_t** node_info;

public:
	void* operator new(size_t size)
	{
		return ::operator new(size);
	}

	void* operator new(size_t size, unsigned extra)
	{
		return ::operator new(size + extra);
	}

	/**
	 * calculate checksum of the besch
	 * called in register_obj
	 * ATTENTION: xref's are NOT resolved yet
	 * @author Dwachs
	 */
	void calc_checksum(checksum_t *chk) const
	{
	}

	friend class obj_reader_t;
};

#endif
