/*
 * railblocks.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef railblocks_h
#define railblocks_h

#include "simtypes.h"
#include "dataobj/koord3d.h"
#include "tpl/slist_tpl.h"
#include "convoihandle_t.h"
#include "tpl/quickstone_tpl.h"

class karte_t;
class loadsave_t;
class signal_t;
class vehikel_t;
class karte_t;
class blockstrecke_t;
class cbuffer_t;

typedef quickstone_tpl<blockstrecke_t> blockhandle_t;


/**
 * Blockstrecken für das Schienennetz. Werden vom Blockmanager verwaltet.
 * @see blockmanager
 * @author Hj. Malthaner
 */
class blockstrecke_t
{
private:
	slist_tpl <signal_t *> signale;

	static karte_t *welt;

	uint16 v_rein, v_raus;

	/**
	* Das Handle für uns selbst. In Anlehnung an 'this' aber mit
	* allen checks beim Zugriff.
	* @author Hanjsörg Malthaner
	*/
	blockhandle_t self;

	blockstrecke_t(karte_t *welt);
	blockstrecke_t(karte_t *welt, loadsave_t *file);

public:
    ~blockstrecke_t();

	typedef enum { BLOCK_EMPTY=0, BLOCK_RESERVED=1, BLOCK_FILLED=2 } blockstate;

	karte_t *gib_welt() { return welt; }

    /**
     * Rail block factory method. Returns handles instead of pointers.
     * @author Hj. Malthaner
     */
    static blockhandle_t create(karte_t *welt);


    /**
     * Rail block factory method. Returns handles instead of pointers.
     * @author Hj. Malthaner
     */
    static blockhandle_t create(karte_t *welt, loadsave_t *file);


    /**
     * Rail block destruction method.
     * @author Hj. Malthaner
     */
    static void destroy(blockhandle_t &bs);


    void laden_abschliessen();

    void verdrahte_signale_neu();

    void add_signal(signal_t *sig);

    bool loesche_signal_bei(koord3d k);
    bool entferne_signal(signal_t *sig);

    slist_tpl<signal_t *> & gib_signale() {return signale;};
    signal_t * gib_signal_bei(koord3d k);

    void schalte_signale();

    void betrete(vehikel_t *v);
    void verlasse(vehikel_t *v);

    /**
     * da am anfang und am ende einer blockstrecke gezaehlt wird
     * gelten nur gerade "raus" werte
     * ausserdem muessen genauso viele "raus" sein wie "rein" damit
     * die Strecke leer ist.
     */
    blockstate ist_frei() const;

    void setze_belegung(int count);

    void vereinige_vehikel_counter(blockhandle_t bs);

    void rdwr(loadsave_t *file);

    void info(cbuffer_t & buf) const;
};

#endif
