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

    /**
     * Anzahl vehikel in der blockstrecke. Muss ein zaehler sein, da ein
     * aus n wagen bestehen kann, und die strecke erst freigegeben werden
     * kann wenn der letzte wagen die strecke verlassen hat.
     * @author Hj. Malthaner
     */

    uint16 v_rein;         // anzahl betreten
    uint16 v_raus;         // anzahl verlassen

	convoihandle_t cnv_reserved;	// this convoi wants to enter next

    blockstrecke_t(karte_t *welt);
    blockstrecke_t(karte_t *welt, loadsave_t *file);

public:
    ~blockstrecke_t();

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

    void betrete(vehikel_basis_t *v);
    void verlasse(vehikel_basis_t *v);

	bool can_reserve_block(convoihandle_t cnv);	// we can enter here?
	bool reserve_block(convoihandle_t cnv);	// ok, we want to enter here (false, if not possible)
	bool unreserve_block(convoihandle_t cnv);	// we wanted to enter here but now changed our intention

    /**
     * da am anfang und am ende einer blockstrecke gezaehlt wird
     * gelten nur gerade "raus" werte
     * ausserdem muessen genauso viele "raus" sein wie "rein" damit
     * die Strecke leer ist.
     */
    bool ist_frei() const;

    void setze_belegung(int count);

//    void clone_vehikel_counter(blockhandle_t bs) { v_rein = bs->v_rein; v_raus = bs->v_raus; schalte_signale(); }

    void vereinige_vehikel_counter(blockhandle_t bs);

    void rdwr(loadsave_t *file);

    void info(cbuffer_t & buf) const;
};

#endif
