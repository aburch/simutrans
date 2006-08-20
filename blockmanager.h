/*
 * blockmanager.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef blockmanager_h
#define blockmanager_h


#include "railblocks.h"
#include "tpl/array_tpl.h"
#include "dataobj/marker.h"
#include "ifc/koord_beobachter.h"

class vehikel_t;
class karte_t;
class schiene_t;
class spieler_t;


/**
 * Der Blockmanager verwaltet die Blockstrecken.
 * Als singleton implementiert.
 * @see blockstrecke_t
 * @author Hj. Malthaner
 */
class blockmanager
{
private:

    class block_ersetzer : public koord_beobachter
    {
    private:
        karte_t *welt;
	blockhandle_t alt;

    public:
        block_ersetzer(karte_t *welt, blockhandle_t bs);
	virtual ~block_ersetzer() {};

	blockhandle_t neu;

	bool neue_koord(koord3d k);
	void wieder_koord(koord3d );
	bool ist_uebergang_ok(koord3d pos1, koord3d pos2);
    };


    /**
     * (Er)setzt den tracktyp eines blocks. Z.B. auf elektrifiziert.
     * @author Hj. Malthaner
     */
    class tracktyp_ersetzer : public koord_beobachter
    {
    private:
      karte_t *welt;
      blockhandle_t alt;
      uint8 electric;

    public:
      tracktyp_ersetzer(karte_t *welt, blockhandle_t alt, uint8 el);
      virtual ~tracktyp_ersetzer() {};

      bool neue_koord(koord3d k);
      void wieder_koord(koord3d );
      bool ist_uebergang_ok(koord3d pos1, koord3d pos2);
    };



    /**
     * normalerweise weiss eine blockstrecke, ob sie frei ist oder nicht
     * aber neu angelegte blockstrecken besitzen diese information noch
     * nicht. deshalb können objekte dieser Klasse prüfen, ob eine blockstrecke
     * frei ist.
     * @author Hj. Malthaner
     */
    class pruefer_ob_strecke_frei : public koord_beobachter
    {
    private:
        karte_t *welt;
	blockhandle_t bs;
    public:
	pruefer_ob_strecke_frei(karte_t *welt, blockhandle_t bs);
	virtual ~pruefer_ob_strecke_frei() {};

	bool neue_koord(koord3d k);
	void wieder_koord(koord3d );
	bool ist_uebergang_ok(koord3d pos1, koord3d pos2);

	int count;
    };



    static blockmanager * single_instance;

    blockmanager();

    slist_tpl< blockhandle_t  > strecken;
    marker_t marker;

    const char * baue_neues_signal(karte_t *welt, spieler_t *sp,
                                   koord3d pos, koord3d pos2, schiene_t *sch,
                                   ribi_t::ribi dir, bool presignal = false);


    const char * baue_andere_signale(koord3d pos1, koord3d pos2,
                                     schiene_t *sch1, schiene_t *sch2,
			             ribi_t::ribi ribi, bool presignal = false);


    /**
     * vereinigt zwei blockstrecken zu einer einzigen und gibt
     * diese zurück. Dabei wird bs2 zu bs1 zugeschlagen, bs2 geloescht.
     * bs1 bleibt uebrig.
     */
    void vereinige(karte_t *welt,
                   blockhandle_t  bs1,
                   blockhandle_t  bs2);

public:

    void setze_welt_groesse(int w,int h);

    int gib_block_nr(blockhandle_t bs) {return strecken.index_of(bs);};
    blockhandle_t  gib_strecke_bei(int i) {return (blockhandle_t )strecken.at(i);};

    const slist_tpl< blockhandle_t  > * gib_strecken() {return &strecken;};


    /**
     * gibt den blockmanager zurück
     */
    static blockmanager * gib_manager();

    void neue_schiene(karte_t *welt, grund_t *gr, ding_t *sig = NULL);

    void schiene_erweitern(karte_t *welt, grund_t *gr);

    /**
     * entfernt ein signal aus dem blockstreckennetz.
     */
    bool entferne_signal(karte_t *welt, koord3d k);

    /**
     * entfernt eine koordinate aus dem blockstreckennetz.
     */
    bool entferne_schiene(karte_t *welt, koord3d k);

    /**
     * @return NULL wenn ok, oder Fehlermeldung
     */
    const char* neues_signal(karte_t *welt, spieler_t *sp,
                             koord3d pos, ribi_t::ribi dir, bool presignal = false);


    bool entferne_signal(signal_t *sig, blockhandle_t bs);


    /**
     * findet blockstrecke zu einem beliebigen punkt auf der strecke
     */
    blockhandle_t  finde_blockstrecke(karte_t *welt, koord3d punkt);



    /**
     * Nur zum Debugging, setzt leere Blockstrecken auf leer
     * wenn der Blockstreckenzustand inkonsistent geworden ist
     */
    void pruefe_blockstrecke(karte_t *welt, koord3d k);

    void setze_tracktyp(karte_t *welt, koord3d k, uint8 styp);


    void rdwr(karte_t *welt, loadsave_t *file);
    void laden_abschliessen();

    void delete_all_blocks();

private:

    array_tpl<koord3d> &finde_nachbarn(const karte_t *welt, const koord3d pos,
                                  const ribi_t::ribi ribi, int &anzahl);

    bool ist_markiert(grund_t *gr) const { return marker.ist_markiert(gr); }
    void traversiere_netz(const karte_t * welt, const koord3d start, koord_beobachter *cmd);
};


#endif
