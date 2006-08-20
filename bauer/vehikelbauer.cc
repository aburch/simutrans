/*
 * vehikelbauer.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simvehikel.h"
#include "../simplay.h"
#include "../simworld.h"  // for year/month
#include "../simdebug.h"
#include "../simtools.h"  // for simrand

#include "../dataobj/umgebung.h"

#include "../besch/vehikel_besch.h"
#include "../boden/wege/weg.h"

#include "../bauer/warenbauer.h"
#include "../bauer/vehikelbauer.h"


inthashtable_tpl<int, const vehikel_besch_t *> vehikelbauer_t::_fahrzeuge;
stringhashtable_tpl<const vehikel_besch_t *> vehikelbauer_t::name_fahrzeuge;
inthashtable_tpl<weg_t::typ, slist_tpl<const vehikel_besch_t *> > vehikelbauer_t::typ_fahrzeuge;


vehikel_t *
vehikelbauer_t::baue(karte_t *welt, koord3d k,
                   spieler_t *sp, convoi_t *cnv, const vehikel_besch_t *vb)
{
    vehikel_t *v = NULL;


    if(vb) {

  switch(vb->gib_typ()) {
  case weg_t::strasse:
      v = new automobil_t(welt, k, vb, sp, cnv);
      break;
  case weg_t::schiene:
  case weg_t::schiene_strab:
  case weg_t::schiene_monorail:
      v = new waggon_t(welt, k, vb, sp, cnv);
      break;
  case weg_t::wasser:
      v = new schiff_t(welt, k, vb, sp, cnv);
      break;
  case weg_t::luft:
      v = new aircraft_t(welt, k, vb, sp, cnv);
      break;
   case weg_t::invalid:
   case weg_t::powerline:
   	dbg->fatal("vehikelbauer_t::baue()","cannot built a vehicle with waytype %i",vb->gib_typ());
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

	weg_t::typ typ = besch->gib_typ();

	slist_tpl<const vehikel_besch_t *> *typ_liste = typ_fahrzeuge.access(typ);
	if(!typ_liste) {
		typ_fahrzeuge.put(typ, slist_tpl<const vehikel_besch_t *>());
		typ_liste = typ_fahrzeuge.access(typ);
	}

	// sort vehicles according to their engine type
	bool append = true;
	for(unsigned i=0;  i<typ_liste->count();  i++  ) {
		const vehikel_besch_t *sort_besch = typ_liste->at(i);
		if(sort_besch->get_engine_type()<besch->get_engine_type()) {
			continue;
		}
		if(sort_besch->get_engine_type()>besch->get_engine_type()) {
			typ_liste->insert(besch,0);
			append = false;
			break;
		}
	}
	if(append) {
		typ_liste->append(besch);
	}

	// correct for driving on left side
	if(typ==weg_t::strasse  &&  umgebung_t::drive_on_left) {
		const int XOFF=(12*get_tile_raster_width())/64;
		const int YOFF=(6*get_tile_raster_width())/64;

		display_set_image_offset( besch->gib_bild_nr(0,true), +XOFF, +YOFF );
		display_set_image_offset( besch->gib_bild_nr(1,true), -XOFF, +YOFF );
		display_set_image_offset( besch->gib_bild_nr(2,true), 0, +YOFF );
		display_set_image_offset( besch->gib_bild_nr(3,true), +XOFF, 0 );
		display_set_image_offset( besch->gib_bild_nr(4,true), -XOFF, -YOFF );
		display_set_image_offset( besch->gib_bild_nr(5,true), +XOFF, -YOFF );
		display_set_image_offset( besch->gib_bild_nr(6,true), 0, -YOFF );
		display_set_image_offset( besch->gib_bild_nr(7,true), -XOFF-YOFF, 0 );
		// need also to correct empty graph
		if(besch->gib_bild_nr(0,true)!=besch->gib_bild_nr(0,false)) {
			display_set_image_offset( besch->gib_bild_nr(0,false), +XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(1,false), -XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(2,false), 0, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(3,false), +XOFF+YOFF, 0 );
			display_set_image_offset( besch->gib_bild_nr(4,false), -XOFF, -YOFF );
			display_set_image_offset( besch->gib_bild_nr(5,false), +XOFF, -YOFF );
			display_set_image_offset( besch->gib_bild_nr(6,false), 0, -YOFF );
			display_set_image_offset( besch->gib_bild_nr(7,false), -XOFF-YOFF, 0 );
		}
	}

	return true;
}



void vehikelbauer_t::sort_lists()
{
    inthashtable_iterator_tpl<weg_t::typ, slist_tpl<const vehikel_besch_t *> > typ_iter(typ_fahrzeuge);

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
      //  1. Warengruppe
      //  2. Ware
      //  3. Speed
      //  4. Power
      //  5. Name
      int l = 0, r = typ_liste->count() - 1;

      while(l <= r) {
    int m = (r + l) / 2;

    const vehikel_besch_t *test = typ_liste->at(m);

    int cmp = test->gib_ware()->gib_catg() - besch->gib_ware()->gib_catg();

    if(cmp == 0) {
        cmp = warenbauer_t::gib_index(test->gib_ware()) -
        warenbauer_t::gib_index(besch->gib_ware());

        if(cmp == 0) {
      cmp = test->gib_zuladung() - besch->gib_zuladung();

        if(cmp == 0) {
        	if(test->gib_leistung()>0) {
	        	// to handle tender correctly
	        	uint8 engine = (test->gib_zuladung()+test->gib_leistung()==0) ? vehikel_besch_t::steam : test->get_engine_type();
	        	uint8 besch_engine = (besch->gib_zuladung()+besch->gib_leistung()==0) ? vehikel_besch_t::steam : besch->get_engine_type();
	        	cmp =  engine - besch_engine;
	        }

        if(cmp == 0) {
      cmp = test->gib_geschw() - besch->gib_geschw();

      if(cmp == 0) {
      	// put tender at the end of the list ...
      	int leistung = test->gib_leistung()==0 ? 0x7FFFFFF : test->gib_leistung();
      	int besch_leistung = besch->gib_leistung()==0 ? 0x7FFFFFF : besch->gib_leistung();
          cmp =  leistung - besch_leistung;

        if(cmp == 0) {
      cmp = test->get_intro_year_month() - besch->get_intro_year_month();

          if(cmp == 0) {
        cmp = strcmp(test->gib_name(), besch->gib_name());
          }
      }
      }
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
                             weg_t::typ vtyp,
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

    DBG_MESSAGE("vehikelbauer_t::gib_info()","no vehicle matches way type %d, good %d, min. power %d", vtyp, wtyp, min_power);
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
vehikelbauer_t::gib_info(weg_t::typ typ, unsigned int i)
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



/* test, if a certain vehicle can lead a convoi *
 * used by vehikel_search
 * @author prissi
 */
int vehikelbauer_t::vehikel_can_lead( const vehikel_besch_t *v )
{
  if(  v->gib_vorgaenger(0)==NULL  ) {
    // ok, can lead a convoi
    return 1;
  }
  for(  int i=1;  i<v->gib_vorgaenger_count();  i++  ) {
//DBG_MESSAGE( "vehikelbauer_t::vehikel_can_lead","try freight car %s %i %p",v->gib_name(),v->gib_vorgaenger_count(),v->gib_vorgaenger(i));
    if(  v->gib_vorgaenger(i)==NULL  ) {
      // ok, can lead a convoi
      return 1;
    }
  }
  // cannot lead
  return 0;
}



/* extended sreach for vehicles for KI *
 * checks also timeline and contraits
 * tries to get best with but adds a little random action
 * @author prissi
 */
const vehikel_besch_t *vehikelbauer_t::vehikel_search(weg_t::typ typ,const unsigned month_now,const int target_power,const int target_speed,const ware_besch_t * target_freight,bool include_electric)
{
  // only needed for iteration
  inthashtable_iterator_tpl<int, const vehikel_besch_t *> iter(_fahrzeuge);
  const vehikel_besch_t *besch;

  if(  target_freight==NULL  &&  target_power==0  ) {
    // no power, no freight => no vehikel to search
    return NULL;
  }

DBG_MESSAGE( "vehikelbauer_t::vehikel_search()","for speed %i, power %i",target_speed,target_power);

  besch = NULL;
//  leistung <<= 6; // to account for gear
  while(iter.next()) {
    // check for wegetype
    const vehikel_besch_t *test_besch = iter.get_current_value();
    // correct type and useable=
    if(test_besch->gib_typ()==typ  &&  !test_besch->is_future(month_now)) {
        // finally, we might be able to use this vehicle
        int power = test_besch->gib_leistung();
        int speed = test_besch->gib_geschw();

        // we want a car
        if(  target_freight!=NULL ) {
        	if( test_besch->gib_zuladung()>0
        		&&  target_power==0
        			||  (power>=target_power  &&  vehikel_can_lead(test_besch)  &&  (test_besch->gib_nachfolger_count()==0  ||  test_besch->gib_nachfolger(0)==0))
        	) {
            // it is a good car (and road vehicles need power)
            if(  test_besch->gib_ware()->is_interchangeable( target_freight )  ) {
DBG_MESSAGE( "vehikelbauer_t::vehikel_search","try freight car %s",test_besch->gib_name());
              // freight category ok
#if 1
              int difference=0;	// smaller is better
              // assign this vehicle, if we have none found one yet, or we found only a too week one
              if(  besch!=NULL  ) {
                // it is cheaper to run? (this is most important)
                difference = besch->gib_zuladung()-test_besch->gib_zuladung();
                difference += (test_besch->gib_betriebskosten() - besch->gib_betriebskosten())/2;
                if(  target_power>0  ) {
	             // it is strongerer?
     	             difference += (besch->gib_leistung() < power)? -10 : 10;
     	           }
                // it is faster? (although we support only up to 120km/h for goods)
                difference += (besch->gib_geschw() < test_besch->gib_geschw())? -20 : 20;
                // it is cheaper? (not so important)
                difference += (besch->gib_preis() > test_besch->gib_preis())? -10 : 10;
              }
              // ok, final check
              if(  besch==NULL  ||  difference<simrand(30)    )
              {
                  // then we want this vehicle!
                  besch = test_besch;
DBG_MESSAGE( "vehikelbauer_t::vehikel_search","Found engine %s",besch->gib_name());
              }
#else
              if(  besch==NULL  ||  (besch->gib_zuladung()+besch->gib_geschw())<(test_besch->gib_zuladung()+test_besch->gib_geschw())  ) {
DBG_MESSAGE( "vehikelbauer_t::vehikel_search","Found freight car %s",test_besch->gib_name());
                // we have more freigth here ....
                // then we want this vehicle!
                besch = test_besch;
              }
#endif
            }
          }
        }
        else {

          // so we have power: this is an engine, which can lead a track and have no constrains
          if(  power>0  &&
            (include_electric  ||  test_besch->get_engine_type()!=vehikel_besch_t::electric)  &&
            vehikel_can_lead(test_besch)  &&
            test_besch->gib_nachfolger_count()==0
          ) {
            // we cannot use an engine with freight
            if(  besch==NULL  ||  power>=target_power  ||   speed>=target_speed  ) {
              // we found a useful engine
              int difference=0;	// smaller is better
              // assign this vehicle, if we have none found one yet, or we found only a too week one
              if(  besch!=NULL  ) {
                // it is cheaper to run? (this is most important)
                difference = test_besch->gib_betriebskosten() - besch->gib_betriebskosten();
                // it is strongerer?
                difference += (besch->gib_leistung() < power)? -40 : 40;
                // it is faster? (although we support only up to 120km/h for goods)
                difference += (besch->gib_geschw() < test_besch->gib_geschw())? -40 : 40;
                // it is cheaper? (not so important)
                difference += (besch->gib_preis() > test_besch->gib_preis())? -20 : 20;
              }
              // ok, final check
              if(  besch==NULL  ||  besch->gib_leistung()<target_power  ||  besch->gib_geschw()<target_speed  ||  difference<simrand(100)    )
              {
                  // then we want this vehicle!
                  besch = test_besch;
DBG_MESSAGE( "vehikelbauer_t::vehikel_search","Found engine %s",besch->gib_name());
              }
            }
          }
        }
      }
  } // end of iteration
  // no vehicle found!
  if(  besch==NULL  ) {
    DBG_MESSAGE( "vehikelbauer_t::vehikel_search()","could not find a suitable vehicle! (speed %i, power %i)",target_speed,target_power);
  }
  return besch;
}



const vehikel_besch_t *vehikelbauer_t::vehikel_fuer_leistung(int leistung, weg_t::typ typ,const unsigned month_now)
{
  // only needed for iteration
  inthashtable_iterator_tpl<int, const vehikel_besch_t *> iter(_fahrzeuge);
  const vehikel_besch_t *besch = NULL;

//  leistung <<= 6; // to account for gear
  while(iter.next()) {
      // check for rail
    if(  (iter.get_current_value()->gib_typ()==typ)  &&
      // in the moment no vehicles with additional freight are allowed
      (iter.get_current_value()->gib_zuladung()==0)
        &&
      // and can this vehicle lead a convoi?
      (iter.get_current_value()->gib_vorgaenger_count()==0  ||  iter.get_current_value()->gib_vorgaenger(0)==NULL)  &&
      // an in the moment we support no tenders for steam engines
      (iter.get_current_value()->gib_nachfolger_count()==0)
    ) {
DBG_MESSAGE( "vehikelbauer_t::vehikel_fuer_leistung()","%s: vorgaenger %i nachfolger %i",iter.get_current_value()->gib_name(),iter.get_current_value()->gib_vorgaenger_count(),iter.get_current_value()->gib_nachfolger_count());
      // ok, might be useful: Now, check for intro year
      const unsigned month = iter.get_current_value()->get_intro_year_month();

      if(month <= month_now) {
        // finally, we might be able to use this vehicle
        int power = iter.get_current_value()->gib_leistung();
        if(  power>leistung  ||  besch==NULL  ) {
          // assign this vehicle, if we have none found one yet, or we found only a too week one
          if(  besch==NULL  ||  besch->gib_leistung()<leistung  ||
            // it is cheaper to run?
            iter.get_current_value()->gib_betriebskosten()-besch->gib_betriebskosten()<simrand(100)  ||
            // it is faster
             iter.get_current_value()->gib_geschw()-besch->gib_geschw()>simrand(128)    ) {

              // then we want this vehicle!
              besch = iter.get_current_value();
          }
        }
      }
    }
  }
  // no vehicle found!
  if(  besch==NULL  ) {
    DBG_MESSAGE( "spieler_t::create_rail_transport_vehikel()","could not find a suitable rail vehicle!");
  }
  return besch;
}
