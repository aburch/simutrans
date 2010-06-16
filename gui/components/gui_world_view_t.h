/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef world_view_t_h
#define world_view_t_h

#include "../../ifc/gui_komponente.h"
#include "../../dataobj/koord3d.h"
#include "../../tpl/vector_tpl.h"

class ding_t;
class karte_t;


/**
 * Displays a little piece of the world
 *
 * @autor Hj. Malthaner
 */
class world_view_t : public gui_komponente_t
{
	public:
		world_view_t(karte_t*, koord size);

		/**
		 * Events werden hiermit an die GUI-Komponenten
		 * gemeldet
		 * @author Hj. Malthaner
		 */
		bool infowin_event(const event_t *);

		/**
		 * resize window in response to a resize event
		 * need to recalculate the list of offsets
		 * @author prissi
		 */
		virtual void set_groesse(koord groesse);

	protected:
		virtual koord3d get_location() = 0;

		void internal_draw(koord offset, ding_t const*);

	private:
		vector_tpl<koord> offsets; /**< Offsets are stored. */
		sint16            raster;  /**< For this rastersize. */
		karte_t*          welt;    /**< The world to display. */
};

#endif
