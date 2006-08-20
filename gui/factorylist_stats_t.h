/*
 * factorylist_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef factorylist_stats_t_h
#define factorylist_stats_t_h

#include "../tpl/vector_tpl.h"
#include "../ifc/gui_komponente.h"
#include "../dataobj/translator.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "button.h"
#include "fabrik_info.h"

class karte_t;
class fabrik_t;
class button_t;
class factorylist_stats_t;

enum {FL_SM_NAME=0,FL_SM_INPUT,FL_SM_OUTPUT,FL_SM_MAXPROD,FL_SM_STATUS,FL_SM_POWER};

class factorylist_header_t : public gui_komponente_t
{
 private:

  factorylist_stats_t *stats;
  button_t bt_filter;


 public:

  factorylist_header_t(factorylist_stats_t *stats);
  ~factorylist_header_t();


  /**
   * Events werden hiermit an die GUI-Komponenten
   * gemeldet
   * @author Hj. Malthaner
   */
  virtual void infowin_event(const event_t *);


  /**
   * Zeichnet die Komponente
   * @author Hj. Malthaner
   */
  void zeichnen(koord offset) const;
};

/**
 * City list stats display
 * @author Hj. Malthaner
 */
class factorylist_stats_t : public gui_komponente_t
{
private:

	karte_t * welt;
	vector_tpl<fabrik_t*> *fab_list;
	unsigned int sortmode;

public:

	factorylist_stats_t(karte_t *welt);
	~factorylist_stats_t();

	/**
	* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	virtual void infowin_event(const event_t *);

	void set_sortmode(unsigned int sortmode);
	unsigned int get_sortmode(void);
	void sort(void);

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset) const;
};

#endif // factorylist_stats_t_h
