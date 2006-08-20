/* simtime.cc
 *
 * High-level Timer-Routinen
 * von Hj. Maltahner, 2000
 */

#include <stdio.h>
#include <time.h>

#include "simsys.h"
#include "simintr.h"
#include "simtime.h"
#include "simdebug.h"
#include "simtypes.h"


const char * month_names[] =
{
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "Oktober", "November", "December"
};


/**
 * Last real time value
 */
static unsigned long last_time = 0;


/**
 * Simutrans internal time count. This is influenced by
 * time lapse settings, and may be different from the real elapsed
 * time.
 * @author Hj. Malthaner
 */
static unsigned long sim_time = 0;


/**
 * Zeitfaktor in 1/16, 16=1.0
 * @author Hj. Malthaner
 */
static long multi = 16;


/**
 * Sets the last_time mark to current time. This is needed
 * after a break to hide the elapsed time from Simutrans code
 * @author Hj. Malthaner
 */
void sync_last_time_now()
{
	last_time = dr_time();
}


/**
 * Setzt den Zeitmultiplikator.
 *
 * @param m Multiplikator in 1/16 (16=1.0)
 * @author Hj. Malthaner
 */
void set_time_multi(long m)
{
	// smaller 1.0 does not work
	if(m>=1) {
		multi = m;
	}
}


/**
 * Ermittelt den Zeitmultiplikator.
 *
 * @return Multiplikator in 1/16 (16=1.0)
 * @author Hj. Malthaner
 */
long get_time_multi()
{
    return multi;
}



unsigned long get_system_ms()
{
	return dr_time();
}



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
unsigned long get_current_time_millis()
{
	unsigned long now = dr_time();

	if(now > last_time) {
		long diff = (now - last_time);

		diff = (diff * multi) >> 4;
		sim_time += diff;
		last_time = now;
	}

	return sim_time;
}


#if 0
/**
 * Busy wait, millisecond precision
 * @author Hj. Malthaner
 */
static void wait_msec(long msec)
{
  unsigned long T = dr_time() + msec;

  do {
    // how to CPU friendly wait a bit ?!
  } while(dr_time() < T);

  INT_CHECK("simtime 131");
}
#endif


/**
 * wartet millisekunden
 * @author Hj. Malthaner
 */
void simusleep(unsigned milli)
{
	signed ms = milli;
	if(ms==0) {
		// busy wait one millisecond => better do a display check ...
		INT_CHECK("simtime 133");
	}
	else {
		INT_CHECK("simtime 171");
		while(ms>20)  {
			// wait at most 10 ms
			const unsigned long T0 = dr_time();
			dr_sleep(10240);
			const long diff = (long)(dr_time() - T0);
			ms -= diff;
		}
		while(ms>0) {
			INT_CHECK("simtime 177");
			const unsigned long T0 = dr_time();
			dr_sleep(ms<<10);
			const long diff = (long)(dr_time() - T0);
			ms -= diff;
		}
		INT_CHECK("simtime 177");
	}
}



void
time_init()
{
  last_time = dr_time();
}
