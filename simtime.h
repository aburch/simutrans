/*
 * simtime.h
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


/* simtime.h
 *
 * High-level Timer-Routinen
 * von Hj. Malthaner, 2000
 */


/**
 * Sets the last_time mark to current time. This is needed
 * after a break to hide the elapsed time from Simutrans code
 * @author Hj. Malthaner
 */
void sync_last_time_now();


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
unsigned long get_current_time_millis();



/**
 * Setzt den Zeitmultiplikator.
 *
 * @param m Multiplikator in 1/16 (16=1.0)
 * @author Hj. Malthaner
 */
void set_time_multi(long m);


/**
 * Ermittelt den Zeitmultiplikator.
 *
 * @return Multiplikator in 1/16 (16=1.0)
 * @author Hj. Malthaner
 */
long get_time_multi();


/**
 * wartet usec mikrosekunden
 * ruft INT_CHECK falls mehr als 10000 mikrosekunden gewartet werden muss
 * @author Hj. Malthaner
 */
void simusleep(unsigned long usec);


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
