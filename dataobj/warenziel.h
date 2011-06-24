/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dataobj_warenziel_h
#define dataobj_warenziel_h

class loadsave_t;

/**
 * class necessary to load pre0.99.13 savegames
 */

class warenziel_t
{
public:
	warenziel_t() {}
	void rdwr(loadsave_t *file);
};

#endif
