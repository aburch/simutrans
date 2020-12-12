/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OLD_BLOCKMANAGER_H
#define OLD_BLOCKMANAGER_H


class karte_t;
class loadsave_t;


/**
 * Der Blockmanager verwaltet die Blockstrecken.
 * Als singleton implementiert.
 * @see blockstrecke_t
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
