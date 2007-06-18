/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      searches a folder for a certain extension
 */
#ifndef __SEARCHFOLDER_H
#define __SEARCHFOLDER_H


#include "../utils/cstring_t.h"
#include "../tpl/vector_tpl.h"


class searchfolder_t {
public:
	~searchfolder_t();
	int search(const char *filepath, const char *extension);

	static cstring_t complete(const char *filepath, const char *extension);

	typedef vector_tpl<char*>::const_iterator const_iterator;
	const_iterator begin() const { return files.begin(); }
	const_iterator end()   const { return files.end();   }

	private:
		vector_tpl<char*> files; // NEVER EVER USE ctring_T here!!!
};

#endif
