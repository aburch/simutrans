/*
 * schiene.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_schiene_h
#define boden_wege_schiene_h


#include "../../railblocks.h"
#include "weg.h"

class vehikel_t;

/**
 * Klasse für Schienen in Simutrans.
 * Auf den Schienen koennen Züge fahren.
 * Jede Schiene gehört zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class schiene_t : public weg_t
{
protected:
    /**
     * Rail block handle. Should be always bound to a rail block,
     * except a short time after creation. Everything else is an error!
     *
     * @author Hj. Malthaner
     */
    blockhandle_t bs;

    	/**
	 * Bound when this block was sucessfully reserved by the convoi
	 * @author prissi
	 */
	convoihandle_t reserved;

	/**
	 * is electric track?
	 * @author Hj. Malthaner
	 */
	uint8 is_electrified:1;

	/**
	 * number of signals (max 3)
	 * @author prissi
	 */
	uint8 nr_of_signals:2;

public:
	static const weg_besch_t *default_schiene;

    /**
     * File loading constructor.
     * @author Hj. Malthaner
     */
    schiene_t(karte_t *welt, loadsave_t *file);

    /**
     * Basic constructor.
     * @author Hj. Malthaner
     */
    schiene_t(karte_t *welt);

    /**
     * Basic constructor with top_speed
     * @author Hj. Malthaner
     */
    schiene_t(karte_t *welt, int top_speed);


	/**
	 * Destruktor. Entfernt etwaige Debug-Meldungen vom Feld
	 * @author Hj. Malthaner
	 */
	virtual ~schiene_t() {}

	/**
	 * electric tracks allow for more vehicles
	 * @author Hj. Malthaner
	 */
	void setze_elektrisch(bool yesno) { is_electrified=1; }
	uint8 ist_elektrisch() const { return is_electrified; }

	/**
	 * If a signal is here, then we must check, if we enter to a new block
	 * @author prissi
	 */
	bool count_signals();
	uint8 has_signal() const { return nr_of_signals; }

	// not used but for calculating signal state ...
	void setze_blockstrecke(blockhandle_t bs);
	const blockhandle_t & gib_blockstrecke() const {return bs;}

    /**
     * Calculates the image of this pice of railroad track.
     *
     * @author Hj. Malthaner
     */
    virtual void calc_bild(koord3d) { weg_t::calc_bild(); }

    virtual const char *gib_typ_name() const {return "Schiene";}
    virtual typ gib_typ() const {return schiene;}

    /**
     * @return Infotext zur Schiene
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;

    /**
     * true, if this rail can be reserved
     * @author prissi
     */
	bool can_reserve( convoihandle_t c) const { return !reserved.is_bound()  ||  c==reserved; }

    /**
     * true, if this rail can be reserved
     * @author prissi
     */
	bool is_reserved() const { return reserved.is_bound(); }

    /**
     * true, then this rail was reserved
     * @author prissi
     */
	bool reserve( convoihandle_t c);

    /**
     * releases previous reservation
     * @author prissi
     */
	bool unreserve( convoihandle_t c);

    /**
     * releases previous reservation
     * @author prissi
     */
	bool unreserve( vehikel_t *);

    /**
     * gets the related convoi
     * @author prissi
     */
	convoihandle_t get_reserved_convoi() const {return reserved;}

    void rdwr(loadsave_t *file);
};

#endif
