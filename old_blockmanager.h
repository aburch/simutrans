/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef blockmanager_h
#define blockmanager_h


class karte_t;


/**
 * Old class to manage track reservations.
 * Only needed to load very old savegames.
 */
class old_blockmanager_t
{
private:
	static void rdwr_block(karte_t *welt,loadsave_t *file);

public:
	static void rdwr(karte_t *welt, loadsave_t *file);
	static void finish_rd(karte_t *welt);
};

#endif
