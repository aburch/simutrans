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
  "July", "August", "September", "October", "November", "December"
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
	if(m>=16) {
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


/**
 * wartet usec mikrosekunden
 * ruft INT_CHECK falls mehr als 10000 mikrosekunden gewartet werden muss
 * @author Hj. Malthaner
 */
void simusleep(unsigned long usec)
{
#if 0
  // das ist zu betriebssystemabhängig und wird deshalb
  // an einen systemwraper delegiert

  if(usec < 11) {
    // ist wohl schon durch aufruf dieser methode langsamer als 11µs
  } else if(usec < 1024) {

    // busy wait one millisecond
    wait_msec(1);

  } else {
    long ms = usec >> 10;

    do {
      const unsigned long T0 = dr_time();
      dr_sleep(2048);
      INT_CHECK("simtime 137");

      const long diff = (long)(dr_time() - T0);
      ms -= diff;
      // printf("Try to wait 1ms, time passed %d, still to wait %d\n", diff, ms);
    } while(ms > 0);
  }
#else
  if(usec < 11) {
    // ist wohl schon durch aufruf dieser methode langsamer als 11µs
  } else if(usec < 1024) {

    // busy wait one millisecond => vetter do an display check ...
    INT_CHECK("simtime 133");

  } else {
    long ms = usec >> 10;
    INT_CHECK("simtime 171");

    while(ms>20)  {
    	// wait at most 10 ms
      const unsigned long T0 = dr_time();
      dr_sleep(10240);
      INT_CHECK("simtime 177");

      const long diff = (long)(dr_time() - T0);
      ms -= diff;
    }
    if(ms>0) {
       dr_sleep(ms<<10);
    }
  }
#endif
}


void
time_init()
{
  last_time = dr_time();
}
