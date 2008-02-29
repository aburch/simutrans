/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef blockmanager_h
#define blockmanager_h


class karte_t;


/**
 * Der Blockmanager verwaltet die Blockstrecken.
 * Als singleton implementiert.
 * @see blockstrecke_t
 * @author Hj. Malthaner
 */
class old_blockmanager_t
{
private:
	static void rdwr_block(karte_t *welt,loadsave_t *file);

public:
	static void rdwr(karte_t *welt, loadsave_t *file);
	static void laden_abschliessen(karte_t *welt);
};

#endif
