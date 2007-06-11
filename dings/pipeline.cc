/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "leitung.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simplay.h"
#include "../simimg.h"
#include "../simfab.h"
#include "../simskin.h"

#include "../dataobj/translator.h"
#include "../boden/grund.h"
#include "../besch/weg_besch.h"
#include "../bauer/wegbauer.h"


static const char * measures[] =
{
    "fail",
    "weak",
    "good",
    "strong",
};

int leitung_t::kapazitaet = 500;


fabrik_t *
leitung_t::suche_fab_4(const koord pos)
{
    for(int k=0; k<4; k++) {

	const grund_t *gr = welt->lookup(pos+koord::nsow[k])->gib_kartenboden();

	const int n = gr->gib_top();

	for(int i=0; i<n; i++) {
	    const ding_t * dt = gr->obj_bei(i);
	    if(dt != NULL && dt->fabrik() != NULL) {
		return dt->fabrik();
	    }
	}
    }
    return NULL;
}


leitung_t::leitung_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
    conn[0] = conn[1] = conn[2] = conn[3] = NULL;
    menge = 0;

    rdwr(file);
    verbinde();

    welt->sync_add(this);
}


leitung_t::leitung_t(karte_t *welt, koord3d pos, spieler_t *sp) : ding_t(welt, pos)
{
    conn[0] = conn[1] = conn[2] = conn[3] = NULL;
    menge = 0;

    setze_besitzer( sp );
    verbinde();
    welt->sync_add(this);
}

leitung_t::~leitung_t()
{
    entferne(gib_besitzer());
}


bool
leitung_t::verbinde_mit(leitung_t *lt)
{
    bool ok = false;

    if(lt != NULL) {

	for(int i=0; i<4; i++) {
	    if(lt->gib_pos().gib_2d() == gib_pos().gib_2d() + koord::nsow[i]) {
		conn[i] = lt;

//		printf("%d\n", i);
		ok = true;
		break;
	    }
	}

    }

    return ok;
}


bool
leitung_t::verbinde_pos(koord k)
{
    bool ok = true;
    grund_t *gr = welt->lookup(k) ? welt->lookup(k)->gib_kartenboden() : NULL;

	if (gr) {
		leitung_t* lt = gr->find<leitung_t>();
		if (lt == NULL) lt = gr->find<pumpe_t>();
		if (lt == NULL) lt = gr->find<senke_t>();
	if(lt != NULL) {
	    ok &= lt->verbinde_mit(this);
	    ok &= this->verbinde_mit(lt);
	}
    }
    return ok;
}


bool
leitung_t::verbinde()
{
    bool ok = true;

    for(int i=0; i<4; i++) {
	ok &= verbinde_pos(gib_pos().gib_2d()+koord::nsow[i]);
    }

    return ok;
}


void
leitung_t::trenne(leitung_t *lt)
{
    for(int i=0; i<4; i++) {
	if(conn[i] == lt) {
	    conn[i] = NULL;
	    break;
	}
    }
}

void
leitung_t::entferne(const spieler_t *)
{
    welt->sync_remove(this);

    for(int i=0; i<4; i++) {
	if(conn[i] != NULL) {
	    conn[i]->trenne(this);
	    conn[i] = NULL;
	}
    }

}

bool
leitung_t::sync_step(long /*delta_t*/)
{
    // DBG_MESSAGE("leitung_t::sync_step()", "called");

    int count = 0;
    int dsum = 0;


    fluss = 0;

    for(int i=0; i<4; i++) {

	if(conn[i] != NULL) {
	    count ++;

	    if(conn[i]->menge < menge) {
		dsum += menge - conn[i]->menge;
	    }
	}
    }


//    printf("Leitung: dsum=%d count=%d\n", dsum, count);

    if(count > 0) {
	for(int i=0; i<4; i++) {

	    if(conn[i] != NULL) {
		if(conn[i]->menge < menge) {

		    int idp = (menge - conn[i]->menge)/count;

		    // hier koennte ein widerstand miteingerechnet werden, z.B.
		    // idp /=2

		    fluss += conn[i]->einfuellen(idp);
		}
	    }
	}

	menge -= fluss;
    }
    return true;
}

int
leitung_t::gib_bild() const
{
    ribi_t::ribi ribi =
	(conn[0] ? ribi_t::nord : ribi_t::keine) |
	(conn[1] ? ribi_t::sued : ribi_t::keine) |
	(conn[2] ? ribi_t::ost  : ribi_t::keine) |
	(conn[3] ? ribi_t::west : ribi_t::keine);

    return wegbauer_t::leitung_besch->gib_bild_nr(ribi);
}

int
leitung_t::einfuellen(int hinein)
{
    const int frei = kapazitaet - menge;

    if(hinein < frei) {
	menge += hinein;
	fluss += hinein;
	return hinein;
    } else {
	menge = kapazitaet;
	fluss += frei;
	return frei;
    }
}



char *
leitung_t::info(char *buf) const
{
//    buf += sprintf(buf, "Menge: %d\nFluss: %d\n", menge, fluss);

    buf += sprintf(buf, "%s: %s\n",
                   translator::translate("Power"),
                   translator::translate(measures[MIN(fluss>>3, 3)]));

    return buf;
}



pumpe_t::pumpe_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
    abgabe = 0;
    menge = 1000;
    fab = NULL;

    setze_bild(0, skinverwaltung_t::pumpe->gib_bild_nr(0));
}

pumpe_t::pumpe_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
    abgabe = 0;
    menge = 100000;
    fab = NULL;

    setze_bild(0, skinverwaltung_t::pumpe->gib_bild_nr(0));
}


int
pumpe_t::einfuellen(int )
{
    return 0;
}

void
pumpe_t::sync_prepare()
{
    if(fab == NULL) {
	fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
    }
}


bool
pumpe_t::sync_step(long delta_t)
{
    // DBG_MESSAGE("pumpe_t::sync_step()", "called");


    int menge_alt = menge;

	if (fab->gib_eingang()->empty() || fab->gib_eingang()->get(0).menge <= 0) {
	menge = 0;
    }

    leitung_t::sync_step(delta_t);

    int menge_neu = menge;
    menge = 100000;

    abgabe = (menge_alt - menge_neu);

    return true;
}


char *
pumpe_t::info(char *buf) const
{
/*
    buf += sprintf(buf, "%s: %d%s\nFluss:  %d\n",
                   translator::translate("Abgabe"),
		   abgabe,
                   translator::translate("m3"),
                   fluss);
*/


	if (fab->gib_eingang()->empty() || fab->gib_eingang()->get(0).menge <= 0) {
	buf += sprintf(buf, "%s: %s\n",
                       translator::translate("Power"),
                       translator::translate(measures[0]));
    } else {
	buf += sprintf(buf, "%s: %s\n",
                       translator::translate("Power"),
                       translator::translate(measures[3]));
    }

    return buf;
}

senke_t::senke_t(karte_t *welt, loadsave_t *file) : leitung_t(welt , file)
{
    setze_bild(0, skinverwaltung_t::senke->gib_bild_nr(0));
    fluss_alt = 0;
    fab = NULL;
    einkommen = 0;
}

senke_t::senke_t(karte_t *welt, koord3d pos, spieler_t *sp) : leitung_t(welt , pos, sp)
{
    setze_bild(0, skinverwaltung_t::senke->gib_bild_nr(0));
    fluss_alt = 0;
    fab = NULL;
    einkommen = 0;
}

/**
 * Methode für asynchrone Funktionen eines Objekts.
 * @author Hj. Malthaner
 */
bool senke_t::step(long /*delta_t*/)
{
    gib_besitzer()->buche(einkommen >> 3, gib_pos().gib_2d(), COST_INCOME);

    einkommen = 0;
    return true;
}

void
senke_t::sync_prepare()
{
    if(fab == NULL) {
	fab = leitung_t::suche_fab_4(gib_pos().gib_2d());
    }
}

bool
senke_t::sync_step(long time)
{
    //DBG_MESSAGE("senke_t::sync_step()", "called");

    menge = 0;
    fluss_alt = fluss;
    einkommen += ((fluss>>3) > 0) * time;

    fluss = 0;

    return true;
}


char *
senke_t::info(char *buf) const
{
    buf += sprintf(buf, "%s: %s\n",
                   translator::translate("Power"),
                   translator::translate(measures[MIN(fluss_alt>>3, 3)]));

    return buf;
}
