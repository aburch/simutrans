#include <stdlib.h>
#ifdef _MSC_VER
#include <string.h>
#endif

#include "../simevent.h"
#include "../simplay.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../boden/grund.h"
#include "../simfab.h"
#include "../simcity.h"
#include "karte.h"
#include "../dataobj/translator.h"
#include "../boden/wege/schiene.h"

extern "C" {
#include "../simgraph.h"
}

reliefkarte_t * reliefkarte_t::single_instance = NULL;
int reliefkarte_t::mode = -1;

const int reliefkarte_t::map_type_color[MAX_MAP_TYPE] =
{
  7, 11, 15, 132, 23, 31, 35, 7
};

const int reliefkarte_t::severity_color[12] =
{
  //11, 10, 9, 23, 22, 21, 15, 14, 13, 18, 19, 35
   9, 10, 11, 21, 22, 23, 13, 14, 15, 18, 19, 35
};

void
reliefkarte_t::setze_relief_farbe(koord k, const int color)
{
	// if map is in normal mode, set new color for map
	// otherwise do nothing
	// result: convois will not "paint over" special maps
	if (is_map_locked)
	{
		return;
	}

	if ((k.x < 0) || (k.y < 0) || (k.x > welt->gib_groesse() -1) || (k.y > welt->gib_groesse() -1))
		return;

  /*if(zoom == 1) {
    relief->at(k.x, k.y) = color;

  } else {
    k.x += k.x;
    k.y += k.y;

    relief->at(k.x,   k.y) = color;
    relief->at(k.x+1, k.y) = color;
    relief->at(k.x,   k.y+1) = color;
    relief->at(k.x+1, k.y+1) = color;
  }*/

  k.x *= zoom;
  k.y *= zoom;

  for (int x = 0; x < zoom; x++)
  	for (int y = 0; y < zoom; y++)
  		relief->at(k.x + x, k.y + y) = color;
}


/**
 * Berechnet Farbe für Höhenstufe hdiff (hoehe - grundwasser).
 * @author Hj. Malthaner
 */
int
reliefkarte_t::calc_hoehe_farbe(const int hoehe, const int grundwasser)
{
    int color;

//    printf("%d %d\n", hoehe, grundwasser);

    if(hoehe <= grundwasser) {
	color = BLAU;
    } else switch((hoehe-grundwasser)>>4) {
    case 0:
	color = BLAU;
	break;
    case 1:
	color = 183;
	break;
    case 2:
	color = 162;
	break;
    case 3:
	color = 163;
	break;
    case 4:
	color = 164;
	break;
    case 5:
	color = 165;
	break;
    case 6:
	color = 166;
	break;
    case 7:
	color = 167;
	break;
    case 8:
	color = 161;
	break;
    case 9:
	color = 182;
	break;
    case 10:
	color = 147;
	break;
    default:
	color = 199;
	break;
    }
    return color;
}

int
reliefkarte_t::calc_relief_farbe(const karte_t *welt, const koord k)
{
    int color = SCHWARZ;

    if(welt != NULL)
    {
		const planquadrat_t *plan = welt->lookup(k);
		grund_t *gr = plan->gib_kartenboden();

		// Umsetzung von Hoehe in Farben wie bei Reliefkarten ueblich
		if(plan->gib_boden_bei(plan->gib_boden_count() - 1)->ist_bruecke()) {
		    // Brücke
		    color = MN_GREY3;
		} else if(gr->gib_weg(weg_t::strasse)) {
		    if(gr->hat_gebaeude(hausbauer_t::frachthof_besch)) {
				color = HALT_KENN;
		    }
		    else {
				color = STRASSE_KENN;
		    }
		} else if(gr->gib_weg(weg_t::schiene)) {
		    if(gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) {
				color = HALT_KENN;
		    } else {
				color = SCHIENE_KENN;
		    }
		} else if(gr->gib_typ() == grund_t::fundament) {

		    // auf einem fundament steht ein gebaeude
		    // das ist objekt nr. 1

		    ding_t * dt = gr->obj_bei(1);

		    if(dt == NULL || dt->fabrik() == NULL) {
				color = GRAU3;
		    } else {
				color = dt->fabrik()->gib_kennfarbe();
		    }

		} else if(gr->gib_hoehe() <= welt->gib_grundwasser()) {

		    ding_t * dt = gr->obj_bei(1);

		    if(dt != NULL && dt->fabrik() != NULL) {
				color = dt->fabrik()->gib_kennfarbe();
		    } else {
				color = BLAU;
		    }

		} else if(plan->gib_boden_bei(plan->gib_boden_count() > 1 ? 1 : 0)->ist_tunnel()) {
		    // Tunnel
	    	    color = MN_GREY0;
		} else {
		    color = calc_hoehe_farbe(gr->gib_hoehe(), welt->gib_grundwasser());
		}
	}

    return color;
}

/**
 * Updated Kartenfarbe an Position k
 * @author Hj. Malthaner
 */
void
reliefkarte_t::recalc_relief_farbe(const koord k)
{
  setze_relief_farbe(k, calc_relief_farbe(welt, k));
}



reliefkarte_t::reliefkarte_t(karte_t *welt)
{
  relief = NULL;

  zoom = 1;
  mode = -1;
  is_map_locked = false;
  is_shift_pressed = false;

  fab = 0;
  gr = 0;

  if(welt) {
    setze_welt(welt);
    setze_groesse(koord(welt->gib_groesse() * zoom,
			welt->gib_groesse() * zoom));

    init();
  } else {
    setze_welt(NULL);
    setze_groesse(koord(256, 256));
  }
}


reliefkarte_t::~reliefkarte_t()
{
  if(relief != NULL) {
    delete [] relief;
  }
}


reliefkarte_t *
reliefkarte_t::gib_karte(karte_t *welt)
{
    if(single_instance == NULL) {
	single_instance = new reliefkarte_t(welt);
    } else {
	if(single_instance->welt != welt) {
	    single_instance->setze_welt(welt);
	}
    }

    return single_instance;
}


reliefkarte_t *
reliefkarte_t::gib_karte()
{
    if(single_instance == NULL) {
	single_instance = new reliefkarte_t(NULL);
    }

    return single_instance;
}


void
reliefkarte_t::init()
{
  if(welt != NULL) {
    const short map_size = welt->gib_groesse();
    koord k;

    for(k.y=0; k.y<map_size; k.y++) {
      for(k.x=0; k.x<map_size; k.x++) {
	recalc_relief_farbe(k);
      }
    }
  }
}


void
reliefkarte_t::setze_welt(karte_t *welt)
{
    this->welt = welt;			// Welt fuer display_win() merken

    if(relief) {
		delete relief;
		relief = NULL;
    }

    if(welt) {
		setze_groesse(koord(welt->gib_groesse()*zoom,
				    welt->gib_groesse()*zoom));
		relief = new array2d_tpl<unsigned char> (welt->gib_groesse()*zoom,
							 welt->gib_groesse()*zoom);
    } else {
		setze_groesse(koord(0,0));
    }
}


void
reliefkarte_t::infowin_event(const event_t *ev)
{
	is_shift_pressed = ev->ev_key_mod & 0x1;

	// get factory under mouse cursor
	fab = fabrik_t::gib_fab(welt, koord(ev->mx / zoom, ev->my / zoom));

    if(IS_LEFTCLICK(ev) || IS_LEFTDRAG(ev)) {
		welt->setze_ij_off(koord(ev->mx/zoom-8, ev->my/zoom-8)); // Offset empirisch ermittelt !
    }

    if(IS_RIGHTRELEASE(ev) || IS_WHEELUP(ev) || IS_WHEELDOWN(ev)) {

      if (is_shift_pressed || IS_WHEELDOWN(ev)) {
      	zoom == 1 ? zoom = MAX_MAP_ZOOM : zoom /= 2;
    } else {
      	zoom >= MAX_MAP_ZOOM ? zoom = 1 : zoom *= 2;
    }

      setze_welt(welt);
      init();
      calc_map(mode);
    }
}


void
reliefkarte_t::zeichnen(koord pos) const
{
  if(welt != NULL && relief != NULL) {

    display_fillbox_wh_clip(pos.x,
			    pos.y,
			    4000, 4000, SCHWARZ, true);

    display_array_wh(pos.x, pos.y,
		     relief->get_width(),
		     relief->get_height(),
		     relief->to_array());

    int xpos = (welt->gib_ij_off().x+8)*zoom;
    int ypos = (welt->gib_ij_off().y+8)*zoom;
    unsigned int i;

    // zoom faktor
    const int zf = zoom * get_zoom_factor();

    // zoom/resize "selection box" in map
    display_direct_line(pos.x+xpos+12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos-12*zf, WEISS);
    display_direct_line(pos.x+xpos-12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos-12*zf, WEISS);
    display_direct_line(pos.x+xpos+12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos+12*zf, WEISS);
    display_direct_line(pos.x+xpos-12*zf, pos.y+ypos, pos.x+xpos, pos.y+ypos+12*zf, WEISS);

    const array_tpl<stadt_t *> * staedte = welt->gib_staedte();

    for(i=0; i<staedte->get_size(); i++) {
      const stadt_t *stadt = staedte->get(i);

      koord p = stadt->gib_pos();
      const char * name = stadt->gib_name();

      display_text(zoom > 1 ? 1 : 0, pos.x+p.x*zoom-strlen(name)*zoom,
		   pos.y+p.y*zoom-4,
		   stadt->gib_name(), WEISS, true);
    }

    if (fab) {
      draw_fab_connections(fab, is_shift_pressed ? ROT : WEISS, pos);
      const koord fabpos = koord(pos.x + fab->pos.x * zoom,
				 pos.y + fab->pos.y * zoom);
      const koord boxpos = fabpos + koord(10, 0);
      const char * name = translator::translate(fab->gib_name());
      display_ddd_proportional_clip(boxpos.x, boxpos.y,
				    proportional_string_width(name)+8,
				    0, 10, WEISS, name, true);
    }
  }
}

void
reliefkarte_t::set_mode (int new_mode)
{
	calc_map(new_mode);
	mode = new_mode;
}

void
reliefkarte_t::calc_map(int render_mode)
{
  is_map_locked = false;
  // prepare empty map
  init();
  if(welt != NULL) {

    slist_iterator_tpl <weg_t *> iter (weg_t::gib_alle_wege());

    while(iter.next()) {

      int cargo = 0;
      grund_t *gr = welt->lookup(iter.get_current()->gib_pos());

      if(gr) {

	const koord k = iter.get_current()->gib_pos().gib_2d();

	switch (render_mode) {
	  // show passenger coverage
	  // display coverage
	case 0:
	  if ((gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::dock_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::bushalt_besch))) {
	    setze_relief_farbe_area(k, 7, map_type_color[render_mode]);
	  }
	  break;
	  // show mail coverage
	  // display coverage
	case 1:
	  if ((gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::dock_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::bushalt_besch))) {

	    halthandle_t halt = haltestelle_t::gib_halt(welt, k);
	    // only show player's haltestellen
	    if ((halt->gib_besitzer() == welt->gib_spieler(0)) && halt->get_post_enabled()) {
	      setze_relief_farbe_area(k, 7, map_type_color[render_mode]);
	    }
	  }
	  break;
	  // show usage
	case 2:
	  if (gr->gib_weg(weg_t::strasse))
	    cargo += gr->gib_weg(weg_t::strasse)->get_statistics(WAY_STAT_GOODS);
	  if (gr->gib_weg(weg_t::schiene))
	    cargo += gr->gib_weg(weg_t::schiene)->get_statistics(WAY_STAT_GOODS);
	  if (gr->gib_weg(weg_t::wasser))
	    cargo += gr->gib_weg(weg_t::wasser)->get_statistics(WAY_STAT_GOODS);
	  if (cargo > 0)
	    setze_relief_farbe_area(k, 1, calc_severity_color(cargo, 1000));
	  break;
				// show station status
	case 3:
	  if ((gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::dock_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::bushalt_besch)))
	    {
	      halthandle_t halt = haltestelle_t::gib_halt(welt, k);
	      // only show player's haltestellen
	      if (halt->gib_besitzer() == welt->gib_spieler(0))
		{
		  int color = GREEN;
		  if(halt->get_pax_happy() > 0 || halt->get_pax_no_route() > 0) {
		    if(halt->get_pax_no_route() > halt->get_pax_happy() * 8) {
		      color = GELB;
		    }
		    if(halt->get_pax_unhappy() > 200) {
		      color = ROT;
		    } else if(halt->get_pax_unhappy() > 40) {
		      color = ORANGE;
		    }
		  }
		  setze_relief_farbe_area(k, 3, color);
		}
	    }
	  break;
				// show frequency of convois visiting a station
	case 4:
	  if ((gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::dock_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::bushalt_besch)))
	    {
	      halthandle_t halt = haltestelle_t::gib_halt(welt, k);
	      // only show player's haltestellen
	      if (halt->gib_besitzer() == welt->gib_spieler(0))
		{
		  // get number of last month's arrived convois
		  setze_relief_farbe_area(k, 3, calc_severity_color(halt->get_finance_history(1, HALT_CONVOIS_ARRIVED), 5));
		}
	    }
	  break;
				// show traffic (=convois/month)
	case 5:
	  if (gr->gib_weg(weg_t::strasse)) {
	    cargo += gr->gib_weg(weg_t::strasse)->get_statistics(WAY_STAT_CONVOIS);
	  }
	  if (gr->gib_weg(weg_t::schiene)) {
	    cargo += gr->gib_weg(weg_t::schiene)->get_statistics(WAY_STAT_CONVOIS);
	  }
	  if (gr->gib_weg(weg_t::wasser)) {
	    cargo += gr->gib_weg(weg_t::wasser)->get_statistics(WAY_STAT_CONVOIS);
	  }
	  if (cargo > 0) {
	    setze_relief_farbe_area(k, 1, calc_severity_color(cargo, 4));
	  }
	  break;
				// show sources of passengers
	case 6:
	  if ((gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::dock_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::bushalt_besch))) {
	    halthandle_t halt = haltestelle_t::gib_halt(welt, k);
	    // only show player's haltestellen
	    if (halt->gib_besitzer() == welt->gib_spieler(0)) {
	      const int net = halt->get_finance_history(1, HALT_DEPARTED)-halt->get_finance_history(1, HALT_ARRIVED);
	      setze_relief_farbe_area(k, 3, calc_severity_color(net, 64));
	    }
	  }
	  break;
				// show destinations for passengers
	case 7:
	  if ((gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::dock_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::bushalt_besch))) {
	    halthandle_t halt = haltestelle_t::gib_halt(welt, k);
	    // only show player's haltestellen
	    if (halt->gib_besitzer() == welt->gib_spieler(0)) {
	      const int net = halt->get_finance_history(1, HALT_ARRIVED)-halt->get_finance_history(1, HALT_DEPARTED);
	      setze_relief_farbe_area(k, 3, calc_severity_color(net, 32));
	    }
	  }
	  break;
				// show waiting goods
	case 8:
	  if ((gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::dock_besch)) ||
	      (gr->hat_gebaeude(hausbauer_t::bushalt_besch))) {
	    halthandle_t halt = haltestelle_t::gib_halt(welt, k);
	    // only show player's haltestellen
	    if (halt->gib_besitzer() == welt->gib_spieler(0)) {
	      setze_relief_farbe_area(k,
				      3,
				      calc_severity_color(halt->get_finance_history(0, HALT_WAITING),
							  8*halt->gib_grund_count()));
	    }
	  }
	  break;
	  // show tracks: white: no electricity, red: electricity, yellow: signal
	case 9:
		// show track
	  if (gr->gib_weg(weg_t::schiene)) {
	  	const schiene_t * sch = dynamic_cast<const schiene_t *> (gr->gib_weg(weg_t::schiene));
		if (sch->ist_elektrisch()) {
	      setze_relief_farbe(k, ROT);
		} else {
	      setze_relief_farbe(k, WEISS);
		}
		// show signals
		if (sch->gib_blockstrecke().is_bound()) {
			if (sch->gib_blockstrecke()->gib_signal_bei(gr->gib_pos())) {
				setze_relief_farbe(k, GELB);
			};
		}
	  }
	break;
	case 10:
		// show max speed
		setze_relief_farbe(k, calc_severity_color(gr->get_max_speed(), 20));
	break;
	default:
	  recalc_relief_farbe(k);
	  break;
	}
      }
    }
  }
  is_map_locked = (render_mode != -1);
}


void
reliefkarte_t::setze_relief_farbe_area(koord k, int areasize, int color)
{
  k -= koord(areasize/2, areasize/2);
  koord p;

  for (p.x = 0; p.x<areasize; p.x++) {
    for (p.y = 0; p.y<areasize; p.y++) {
      setze_relief_farbe(k + p, color);
    }
  }
}

int
reliefkarte_t::calc_severity_color(int amount, int scale)
{
  // color array goes from light blue to red
  int severity = amount / scale;
  if (severity > 11) {
    severity = 11;
  }

  if (severity < 0) {
    severity = 0;
  }

  return reliefkarte_t::severity_color[severity];
}

void
reliefkarte_t::draw_fab_connections(const fabrik_t * fab, int colour, koord pos) const
{
  const koord fabpos = koord(pos.x + fab->pos.x * zoom, pos.y + fab->pos.y * zoom);
  const vector_tpl <koord> &lieferziele = is_shift_pressed ? fab->get_suppliers() : fab->gib_lieferziele();
  for(uint32 i=0; i<lieferziele.get_count(); i++) {
    const koord lieferziel = lieferziele.get(i);
    const fabrik_t * fab2 = fabrik_t::gib_fab(welt, lieferziel);
    if (fab2) {
      const koord end = pos + lieferziel * zoom;
      display_direct_line(fabpos.x+zoom, fabpos.y+zoom,
			  end.x+zoom, end.y+zoom, colour);
      const koord boxpos = end + koord(10, 0);
      display_fillbox_wh_clip(end.x, end.y, 3 * zoom, 3 * zoom,
			      ((welt->gib_zeit_ms() >> 10) & 1) == 0 ? ROT : WEISS, true);

      const char * name = translator::translate(fab2->gib_name());

      display_ddd_proportional_clip(boxpos.x, boxpos.y,
				    proportional_string_width(name)+8,
				    0, 5, WEISS, name, true);
    }
  }
}
