/*
 * simtime.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * High-level Timer-Routinen
 * von Hj. Malthaner, 2000
 */

#include "simtypes.h"


/**
 * Sets the last_time mark to current time. This is needed
 * after a break to hide the elapsed time from Simutrans code
 * @author Hj. Malthaner
 */
void sync_last_time_now();


uint32 get_system_ms();


/**
 * <p>Gibt die aktuelle Zeit in Millisekunden zurueck.</p>
 *
 * <p>Dabei hat die routine zwei Aufgaben:</p>
 *
 * <p>1.) Den Zeitableuf streng monoton steigend zu gestalten, auch wenn
 *        die Systemzeit zurueckgesetzt wird.</p>
 *
 * <p>2.) Den Zeitraffer mit zu berücksichtigen.</p>
 *
 * @author Hj. Malthaner
 */
uint32 get_current_time_millis();



/**
 * Setzt den Zeitmultiplikator.
 *
 * @param m Multiplikator in 1/16 (16=1.0)
 * @author Hj. Malthaner
 */
void set_time_multi(sint32 m);


/**
 * Ermittelt den Zeitmultiplikator.
 *
 * @return Multiplikator in 1/16 (16=1.0)
 * @author Hj. Malthaner
 */
sint32 get_time_multi();


/**
 * idle wait ms milleseconds
 * may update the display if time is over or more than 10ms wait
 * @author Hj. Malthaner
 */
void simusleep(uint16 ms);


/**
 * init. die warteschleife
 * @author Hj. Malthaner
 */
void time_init();


/**
 * Month names
 * @author Hj. Malthaner
 */
extern const char * month_names[];
