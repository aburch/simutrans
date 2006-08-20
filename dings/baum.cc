/*
 * baum.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simplay.h"
#include "../boden/grund.h"
#include "../simmem.h"
#include "../simcosts.h"
#include "../simtools.h"
#include "../mm/mempool.h"
#include "../besch/baum_besch.h"
#include "../utils/cbuffer_t.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"


#include "baum.h"

static const int baum_bild_alter[12] =
{
    0,1,2,3,3,3,3,3,3,4,4,4
};


bool baum_t::hide = false;

mempool_t * baum_t::mempool = new mempool_t(sizeof(baum_t) );

/*
 * Diese Tabelle ermöglicht das Auffinden dient zur Auswahl eines Baumtypen
 */
slist_tpl<const baum_besch_t *> baum_t::baum_typen;

/*
 * Diese Tabelle ermöglicht das Auffinden einer Beschreibung durch ihren Namen
 */
stringhashtable_tpl<const baum_besch_t *> baum_t::besch_names;


int baum_t::gib_anzahl_besch()
{
    return baum_typen.count();
}


/**
 * tree planting function - it takes care of checking suitability of area
 */
bool baum_t::plant_tree_on_coordinate(karte_t * welt,
				      koord pos,
				      const unsigned char maximum_count)
{
  const planquadrat_t *plan = welt->lookup(pos);

  if(plan) {
    grund_t * gr = plan->gib_kartenboden();

    if( gr ){

      if(gr != NULL
	 && gr->ist_natur()
	 && gib_anzahl_besch() > 0
	 && gr->gib_hoehe() > welt->gib_grundwasser()
	 && gr->obj_count() < maximum_count ) {

	gr->obj_add( new baum_t(welt, gr->gib_pos()) ); //plants the tree

	return true; //tree was planted - currently unused value is not checked

      }
    }
  }
  return false;
}

int
baum_t::gib_bild() const
{
    return hide ? besch->gib_bild(0, 0)->gib_nummer() : ding_t::gib_bild();
}

bool baum_t::alles_geladen()
{
    if(besch_names.count() == 0) {
	dbg->message("baum_t", "No trees found - feature disabled");
    }
    return true;
}

bool baum_t::register_besch(baum_besch_t *besch)
{
    baum_typen.append(besch);
    besch_names.put(besch->gib_name(), const_cast<baum_besch_t *>(besch));
    return true;
}


void baum_t::calc_off()
{
  int liob;
  int reob;
  switch (welt->get_slope( gib_pos().gib_2d()) )
  {
  case 0:
    liob=simrand(30)-15;
    reob=simrand(30)-15;
    setze_xoff( reob - liob  );
    setze_yoff( (reob + liob)/2 );
    break;

  case 1:
  case 4:
  case 5:
  case 8:
  case 9:
  case 12:
  case 13:
    liob=simrand(16)-8;
    reob=simrand(16)-8;
    setze_xoff( reob - liob  );
    setze_yoff( reob + liob );
    break;

  case 2:
  case 3:
  case 6:
  case 7:
  case 10:
  case 11:
  case 14:
  case 15:
    liob=simrand(16)-8;
    reob=simrand(16)-8;
    setze_xoff( reob + liob  );
    setze_yoff(-10-(reob - liob)/2 );
    break;
  }
}

inline void
baum_t::calc_bild(const unsigned long alter)
{
    // alter/2048 gibt die tage

    const int bild_neu = besch->gib_bild(0, baum_bild_alter[MIN((alter >> karte_t::ticks_bits_per_tag)>>6, 11)])->gib_nummer();

    if(bild_neu != gib_bild()) {
	setze_bild(0, bild_neu);
    }
}

inline void
baum_t::calc_bild()
{
    calc_bild(welt->gib_zeit_ms() - geburt);
}


const baum_besch_t *baum_t::gib_aus_liste(int level)
{
    slist_tpl<const baum_besch_t *> auswahl;
    slist_iterator_tpl<const baum_besch_t *>  iter(baum_typen);

   while(iter.next()) {
	if(iter.get_current()->gib_hoehenlage() == level) {
	    auswahl.append(iter.get_current());
	}
    }
    return auswahl.count() > 0 ? auswahl.at(simrand(auswahl.count())) : NULL;
}


baum_t::baum_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
    besch = NULL;
    rdwr(file);
    if(gib_besch()) {
	calc_bild();
    }

    step_frequency = 7;
}


baum_t::baum_t(karte_t *welt, koord3d pos) : ding_t(welt, pos)
{
  // Hajo: auch aeltere Baeume erzeugen
  geburt = welt->gib_zeit_ms() - (simrand(400) * welt->ticks_per_tag);

  const grund_t *gr = welt->lookup(pos);

  const int maxindex = baum_typen.count();

  if(gr) {
    int index = gr->gib_hoehe()/16 + (simrand(5)-2);

    if(index < 0) {
      index = 0;
    }

    if(index >= maxindex) {
      index = maxindex - 1;
    }

    besch = gib_aus_liste(index);

  }
  if(!besch) {
    besch = baum_typen.at(simrand(maxindex));
  }

  //  const int guete = 140 - ABS(bd->gib_hoehe() - (bildbasis - IMG_BAUM_1)*6);

  //  printf("Baum: Guete %d\n", guete);


  calc_off();
  calc_bild();


  step_frequency = 7;
}


baum_t::baum_t(karte_t *welt, koord3d pos, const baum_besch_t *besch) : ding_t(welt, pos)
{
  // alter = simrand( 16 ) * welt->ticks_per_tag;

  geburt = welt->gib_zeit_ms();

  this->besch = besch;
  calc_off();
  calc_bild();

  step_frequency = 7;
}


void
baum_t::saee_baum()
{
    const koord k = gib_pos().gib_2d() + koord(simrand(5)-2, simrand(5)-2);
  //the area for normal new tree planting is slightly more restricted, square of 9x9 was too much

    if(welt->lookup(k)) {
	grund_t *bd = welt->lookup( k )->gib_kartenboden();

	if(bd != NULL) {
	    // printf("Saee Baum an %d %d.\n",x,y);

	    if( bd->obj_count() < welt->gib_max_no_of_trees_on_square() && bd->ist_natur() &&
		bd->gib_hoehe() > welt->gib_grundwasser())
	    {
		const int guete = 140 - ABS(bd->gib_hoehe() - besch->gib_hoehenlage()*16);
		//		printf("Guete %d\n", guete);

		if(guete > simrand(128)) {
		    bd->obj_add( new baum_t(welt, bd->gib_pos(), besch) );
		    //		    printf("  Erfolgreich.\n");
		}
	    } else {
		//		printf("  Kein Platz.\n");
	    }
	}
    }
}

bool baum_t::step(long /*delta_t*/)
{
    const long alter = welt->gib_zeit_ms() - geburt;
    const long t_alter_temp1 = (welt->ticks_per_tag)<<9;  //ticks * 512

    calc_bild(alter);

    if(alter >= t_alter_temp1)
    {
        const long t_alter_temp2 = (welt->ticks_per_tag)<<6; //ticks * 64

        if (alter < (t_alter_temp1+t_alter_temp2))
        {  //    ticks * ( 2^9+2^6 = 512 + 64 = 576)

            //plant 3 trees was too much - now only 1-3 trees will be planted....
            const unsigned char c_plant_tree_max = 1+simrand(3);
            for (unsigned char c_temp = 0 ; c_temp < c_plant_tree_max ; c_temp++ )
			{
				saee_baum();
			}

			// und danach nicht noch mal säen
			// Trick: Geburtsdatum verlegen
			geburt -= t_alter_temp2;

        }

        /*Because the first IF was splitted to two - this check can be here
           alter>704*welt->ticks_per_tag can be true only iff (if and only if)
		    alter>512*welt->ticks_per_tag is true :-)
          That saves calculation of 704*world->ticks_per_tag to evaluate this IF for most of tree life */
        if(alter > (t_alter_temp1 + t_alter_temp2 + (t_alter_temp2<<1)) ) {
         // this shall evaluate to (512 + 128 + 64 = 704) * welt->ticks_per_tag
         // that should be faster thant direct multiplication - at least on older processors

            return false;
        }

    }
    return true;

}


void
baum_t::rdwr(loadsave_t *file)
{
    ding_t::rdwr(file);

    long alter = welt->gib_zeit_ms() - geburt;
    file->rdwr_long(alter, "\n");

    // after loading, calculate new
    geburt = welt->gib_zeit_ms() - alter;

    if(file->is_saving()) {
	const char *s = besch->gib_name();
	file->rdwr_str(s, "N");
    }

    if(file->is_loading()) {
	char bname[128];
	file->rd_str_into(bname, "N");

	besch = (const baum_besch_t *)besch_names.get(bname);

	if(!besch) {
    	    dbg->message("baum_t::rwdr", "description %s for tree at %d,%d not found, will be removed!", bname, gib_pos().x, gib_pos().y);
	}
    }
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void baum_t::info(cbuffer_t & buf) const
{
  ding_t::info(buf);

  buf.append("\n");
  buf.append(translator::translate(besch->gib_name()));
  buf.append("\n");
  // buf.append((welt->gib_zeit_ms() - geburt) >> welt->ticks_bits_per_tag);
  // buf.append(" ");
  // buf.append(translator::translate("Tage alt"));
}


void
baum_t::entferne(spieler_t *sp)
{
    if(sp != NULL) {
	sp->buche(CST_BAUM_ENTFERNEN, gib_pos().gib_2d(), COST_CONSTRUCTION);
    }
}


void *
baum_t::operator new(size_t /*s*/)
{
//    printf("new baum_t\n");

    return mempool->alloc();
}


void
baum_t::operator delete(void *p)
{
//    printf("delete baum_t\n");

    mempool->free( p );
}
