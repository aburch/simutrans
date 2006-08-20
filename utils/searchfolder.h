/*
 *
 *  searchfolder.h
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
#ifndef __SEARCHFOLDER_H
#define __SEARCHFOLDER_H

/*
 *  includes
 */
#include "../utils/cstring_t.h"
#include "../tpl/slist_tpl.h"


/**
 *  class:
 *      searchfolder_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 */
class searchfolder_t {
    slist_tpl<cstring_t> files;

public:
    int search(const char *filepath, const char *extension);

    static cstring_t complete(const char *filepath, const char *extension);
    const cstring_t & at(unsigned int i);
};

#endif // __SEARCHFOLDER_H
