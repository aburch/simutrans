/*
 * signal.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_signal_h
#define dings_signal_h

#include "../simdings.h"
#include "../railblocks.h"

/**
 * Signale für die Bahnlinien.
 *
 * @see blockstrecke_t
 * @see blockmanager
 * @author Hj. Malthaner
 */
class signal_t : public ding_t
{
public:
	enum signalzustand {rot=0, gruen=1, naechste_rot=2 };

protected:

	unsigned char zustand;
	unsigned char blockend;
	unsigned char dir;

public:

    /*
     * @return direction of the signal (one of nord, sued, ost, west)
     * @author Hj. Malthaner
     */
    ribi_t::ribi gib_richtung() const { return dir; }

    enum ding_t::typ gib_typ() const {return signal;};
    const char *gib_name() const {return "Signal";};

    signal_t(karte_t *welt, loadsave_t *file);
    signal_t(karte_t *welt, koord3d pos, ribi_t::ribi dir);

    /**
     * signale muessen bei der destruktion von der
     * Blockstrecke abgemeldet werden
     */
    ~signal_t();

    virtual void setze_zustand(enum signalzustand z) {zustand = z; calc_bild();};
    enum signalzustand gib_zustand() {return (enum signalzustand) zustand;};

    bool ist_blockiert() const {return blockend != 0;};
    void setze_blockiert(bool b) {blockend = b; calc_bild();};

    /**
     * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     */
    virtual void info(cbuffer_t & buf) const;


    /**
     * berechnet aktuelles bild
     */
    void calc_bild();

    void rdwr(loadsave_t *file);
    void rdwr(loadsave_t *file, bool force);


    /**
     * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
     * um das Aussehen des Dings an Boden und Umgebung anzupassen
     *
     * @author Hj. Malthaner
     */
    virtual void laden_abschliessen();
};

class presignal_t : public signal_t
{
private:

protected:
	blockhandle_t related_block;	// only used for presignals: last asked block

public:
    enum ding_t::typ gib_typ() const {return presignal;};
    const char *gib_name() const {return "preSignals";};

    presignal_t(karte_t *welt, loadsave_t *file);
    presignal_t(karte_t *welt, koord3d pos, ribi_t::ribi dir);

    void set_next_block_pos( blockhandle_t bs ) {related_block = bs; };

    virtual void setze_zustand(enum signal_t::signalzustand z);
	/* recalculate image */
	bool step(long);

    void calc_bild();
};

class choosesignal_t : public presignal_t
{
public:
    enum ding_t::typ gib_typ() const {return choosesignal;}
    const char *gib_name() const {return "ChooseSignal";}

    choosesignal_t(karte_t *welt, loadsave_t *file) : presignal_t(welt,file) {}
    choosesignal_t(karte_t *welt, koord3d pos, ribi_t::ribi dir) : presignal_t(welt,pos,dir) {}

    void calc_bild();
};

#endif
