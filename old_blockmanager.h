/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef blockmanager_h
#define blockmanager_h


class world_t;


/**
 * Old class to manage track reservations.
 * Only needed to load very old savegames.
 */
class old_blockmanager_t
{
private:
	static void rdwr_block(world_t *welt,loadsave_t *file);

public:
	static void rdwr(world_t *welt, loadsave_t *file);
	static void finish_rd(world_t *welt);
};

#endif
