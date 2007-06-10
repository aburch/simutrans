/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_leitung_t
#define dings_leitung_t


#include "../ifc/sync_steppable.h"
#include "../dataobj/koord3d.h"
#include "../simdings.h"

class spieler_t;
class fabrik_t;

class leitung_t : public ding_t, sync_steppable
{
private:

    leitung_t *conn[4];

    static int kapazitaet;

    bool verbinde_mit(leitung_t *lt);
    bool verbinde_pos(koord k);
    bool verbinde();

    void trenne(leitung_t *lt);


protected:

    virtual int einfuellen(int hinein);

    int menge, fluss;

public:

    static fabrik_t * suche_fab_4(koord pos);

    leitung_t(karte_t *welt, loadsave_t *file);
    leitung_t(karte_t *welt, koord3d pos, spieler_t *sp);
    virtual ~leitung_t();

    /**
     * Vorbereitungsmethode für Echtzeitfunktionen eines Objekts.
     * @author Hj. Malthaner
     */
    virtual void sync_prepare() {}

    bool sync_step(long delta_t);

    enum ding_t::typ gib_typ() const { return leitung; }

    const char* gib_name() const { return "Leitung"; }
    image_id gib_bild() const;
    char *info(char *buf) const;

    void entferne(const spieler_t *sp);
};

class pumpe_t : public leitung_t
{
private:
    int abgabe;

    fabrik_t *fab;

protected:
    virtual int einfuellen(int hinein);

public:
    pumpe_t(karte_t *welt, loadsave_t *file);
    pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp);

    void setze_fabrik(fabrik_t* fab) { this->fab = fab; }

    enum ding_t::typ gib_typ() const { return pumpe; }

    /**
     * Eine Pumpe kann zu einer Fabrik gehören.
     * @return Einen Zeiger auf die Fabrik zu der die Pumpe gehört
     *
     * @author Hj. Malthaner
     */
    virtual inline fabrik_t* fabrik() const { return fab; }

    void sync_prepare();
    bool sync_step(long delta_t);
    int gib_bild() const { return ding_t::gib_bild(); }

    const char* name() const { return "Pumpe"; }
    char *info(char *buf) const;
};

class senke_t : public leitung_t
{
private:
    int fluss_alt;
    int einkommen;

    fabrik_t *fab;

protected:

public:

    senke_t(karte_t *welt, loadsave_t *file);
    senke_t(karte_t *welt, koord3d pos, spieler_t *sp);

    enum ding_t::typ gib_typ() const { return senke; }

    /**
     * Eine Senke kann zu einer Fabrik gehören.
     * @return Einen Zeiger auf die Fabrik zu der die Senke gehört
     *
     * @author Hj. Malthaner
     */
    virtual inline fabrik_t* fabrik() const { return fab; }

    /**
     * Methode für asynchrone Funktionen eines Objekts.
     * @author Hj. Malthaner
     */
    virtual bool step(long /*delta_t*/);

    void sync_prepare();
    bool sync_step(long delta_t);
    int gib_bild() const { return ding_t::gib_bild(); }

    const char* name() const { return "Senke"; }

    char *info(char *buf) const;
};

#endif
