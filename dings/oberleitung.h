/*
 * oberleitung.h
 *
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef oberleitung_t_h
#define oberleitung_t_h

#include "../simdings.h"

class spieler_t;

/**
 * Overhead powelines for elctrifed tracks.
 *
 * @author Hj. Malthaner
 */
class oberleitung_t : public ding_t
{

  /**
   * Front side image
   * @author Hj. Malthaner
   */
  sint16 front;


 public:


  oberleitung_t(karte_t *welt, koord3d pos, spieler_t *besitzer);

  oberleitung_t(karte_t *welt, loadsave_t *file);

  virtual ~oberleitung_t();


  /**
   * Falls etwas nach den Vehikeln gemalt werden muß.
   * @author V. Meyer
   */
  virtual int gib_after_bild() const;



  /**
   * 'Jedes Ding braucht einen Typ.'
   * @return Gibt den typ des Objekts zurück.
   * @author Hj. Malthaner
   */
  virtual enum ding_t::typ gib_typ() const {return oberleitung;};


  /**
   * Oberleitung öffnet kein Info-Fenster
   * @author Hj. Malthaner
   */
  virtual void zeige_info();


  void calc_bild();


  /**
   * Speichert den Zustand des Objekts.
   *
   * @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
   * soll.
   * @author Hj. Malthaner
   */
  virtual void rdwr(loadsave_t *file);


  /**
   * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
   * um das Aussehen des Dings an Boden und Umgebung anzupassen
   *
   * @author Hj. Malthaner
   */
  virtual void laden_abschliessen();
};

#endif // oberleitung_t_h
