/*
 * vehikelbauer.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../besch/vehikel_besch.h"
#include "../bauer/warenbauer.h"
#include "../simvehikel.h"
#include "../simplay.h"
#include "../simdebug.h"

#include "vehikelbauer.h"


inthashtable_tpl<int, const vehikel_besch_t *> vehikelbauer_t::_fahrzeuge;
stringhashtable_tpl<const vehikel_besch_t *> vehikelbauer_t::name_fahrzeuge;
inthashtable_tpl<vehikel_besch_t::weg_t, slist_tpl<const vehikel_besch_t *> > vehikelbauer_t::typ_fahrzeuge;


vehikel_t *
vehikelbauer_t::baue(karte_t *welt, koord3d k,
                   spieler_t *sp, convoi_t *cnv, const vehikel_besch_t *vb)
{
    vehikel_t *v = NULL;


    if(vb) {

	switch(vb->gib_typ()) {
	case vehikel_besch_t::strasse:
	    v = new automobil_t(welt, k, vb, sp, cnv);
	    break;
	case vehikel_besch_t::schiene:
	    v = new waggon_t(welt, k, vb, sp, cnv);
	    break;
	case vehikel_besch_t::wasser:
	    v = new schiff_t(welt, k, vb, sp, cnv);
	    break;
	case vehikel_besch_t::luft:
	    break;
	}

	assert(v != NULL);

	if(sp) {
	    sp->buche(-(vb->gib_preis()), k.gib_2d(), COST_NEW_VEHICLE);
	}
    }

    assert (v != NULL);

    return v;
}


bool vehikelbauer_t::register_besch(const vehikel_besch_t *besch)
{
  // printf("N=%s T=%d V=%d P=%d\n", besch->gib_name(), besch->gib_typ(), besch->gib_geschw(), besch->gib_leistung());


    name_fahrzeuge.put(besch->gib_name(), besch);
    _fahrzeuge.put(besch->gib_basis_bild(), besch);


    vehikel_besch_t::weg_t typ = besch->gib_typ();

    slist_tpl<const vehikel_besch_t *> *typ_liste = typ_fahrzeuge.access(typ);
    if(!typ_liste) {
	typ_fahrzeuge.put(typ, slist_tpl<const vehikel_besch_t *>());

	typ_liste = typ_fahrzeuge.access(typ);
    }
    typ_liste->append(besch);

    return true;
}



void vehikelbauer_t::sort_lists()
{
    inthashtable_iterator_tpl<vehikel_besch_t::weg_t, slist_tpl<const vehikel_besch_t *> > typ_iter(typ_fahrzeuge);

    while(typ_iter.next()) {
	slist_tpl<const vehikel_besch_t *> *typ_liste = &typ_iter.access_current_value();
	slist_tpl<const vehikel_besch_t *> tmp_liste;

	while(typ_liste->count() > 0) {
	    tmp_liste.insert(typ_liste->remove_first());
	}

	while(tmp_liste.count() > 0) {
	    const vehikel_besch_t *besch = tmp_liste.remove_first();

	    //
	    // Sortieren nach:
	    //	1. Warengruppe
	    //	2. Ware
	    //	3. Speed
	    //  4. Power
	    //	5. Name
	    int l = 0, r = typ_liste->count() - 1;

	    while(l <= r) {
		int m = (r + l) / 2;

		const vehikel_besch_t *test = typ_liste->at(m);

		int cmp = test->gib_ware()->gib_catg() - besch->gib_ware()->gib_catg();

		if(cmp == 0) {
		    cmp = warenbauer_t::gib_index(test->gib_ware()) -
			  warenbauer_t::gib_index(besch->gib_ware());

		    if(cmp == 0) {
			cmp = test->gib_geschw() - besch->gib_geschw();

			if(cmp == 0) {
			    cmp = test->gib_leistung() - besch->gib_leistung();

			    if(cmp == 0) {
				cmp = strcmp(test->gib_name(), besch->gib_name());
			    }
			}
		    }
		}

		if(cmp > 0) {
		    //printf("cmp=%d: l=%d r=%d m=%d: %s=%d\n", cmp, r, l, m, "r", m -1);
		    r = m - 1;
		}
		else if(cmp < 0) {
		    //printf("cmp=%d: l=%d r=%d m=%d: %s=%d\n", cmp, r, l, m, "l", m + 1);
		    l = m + 1;
		}
		else {
		    l = m;
		    r = m -1;
		}
		//else
		    //printf("cmp=%d: l=%d r=%d m=%d: ???\n", cmp, r, l, m);

		//printf("%d..%d\n", l, r);
	    }
	    typ_liste->insert(besch, l);
	}
    }
}


/**
 * ermittelt ein basis bild fuer ein Fahrzeug das den angegebenen
 * Bedingungen entspricht.
 *
 * @param vtyp strasse, schiene oder wasser
 * @param min_power minimalleistung des gesuchten Fahrzeuges (inclusiv)
 * @author Hansjörg Malthaner
 */
const vehikel_besch_t *
vehikelbauer_t::gib_info(const ware_besch_t *wtyp,
                             vehikel_besch_t::weg_t vtyp,
			     int min_power)
{
    inthashtable_iterator_tpl<int, const vehikel_besch_t *> iter(_fahrzeuge);

    while(iter.next()) {
	const vehikel_besch_t *vb = iter.get_current_value();

	if(vb->gib_typ() == vtyp &&
	   wtyp->is_interchangeable(vb->gib_ware()) &&
	   vb->gib_leistung() >= min_power) {

	    return vb;
	}
    }

    dbg->message("vehikelbauer_t::gib_info()",
		 "no vehicle matches way type %d, good %d, min. power %d",
		 vtyp, wtyp, min_power);

    return NULL;
}


const vehikel_besch_t *
vehikelbauer_t::gib_info(const char *name)
{
    return name_fahrzeuge.get(name);
}


const vehikel_besch_t *
vehikelbauer_t::gib_info(int base_img)
{
  const vehikel_besch_t * besch = _fahrzeuge.get(base_img);

  //  besch->dump();

  return besch;
}


const vehikel_besch_t *
vehikelbauer_t::gib_info(vehikel_besch_t::weg_t typ, unsigned int i)
{
  slist_tpl<const vehikel_besch_t *> *typ_liste = typ_fahrzeuge.access(typ);

  if(typ_liste && i < typ_liste->count()) {
    return typ_liste->at(i);
  } else {
    return NULL;
  }
}


int
vehikelbauer_t::gib_preis(int base_img)
{
    const vehikel_besch_t *vb = gib_info(base_img);
    int preis = 0;

    if(vb) {
	preis = vb->gib_preis();
    }

    return preis;
}



const vehikel_besch_t *vehikelbauer_t::vehikel_fuer_leistung(int leistung, vehikel_besch_t::weg_t typ)
{
    inthashtable_iterator_tpl<int, const vehikel_besch_t *> iter(_fahrzeuge);
    const vehikel_besch_t *besch = NULL;
    int besch_power = 0;

    while(iter.next()) {
	if(iter.get_current_value()->gib_typ() == typ) {
	    int power = iter.get_current_value()->gib_leistung();

	    if(power > leistung)
		power -= leistung;
	    else
		power = leistung - power;

	    if(!besch || power < besch_power) {
		besch_power = power;
		besch = iter.get_current_value();
	    }
	}
    }
    return besch;
}
