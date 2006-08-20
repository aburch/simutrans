/* fabesch.cc
 *
 * Fabrikbeschreibungen, genutzt vom Fabrikbauer
 * Hj. Malthaner, 2000
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "koord.h"
#include "../simio.h"
#include "../simdebug.h"
#include "../simware.h"
#include "../simfab.h"
#include "fabesch.h"

static void check(const char *what, const char *ist, const char *soll)
{
    if(strncmp(ist, soll, strlen(soll))) {
      dbg->fatal("fabesch_t::check()",
		   "failed: required='%s', found='%s'\nwhile loading a '%s'",
		 soll, ist, what);
    }
}

rabesch_t::rabesch_t(char * dataline)
{
    sscanf(dataline,
           "smoke=%hd,%hd,%hd,%hd,%d,%d",
	   &pos_off.x,
	   &pos_off.y,
	   &xy_off.x,
	   &xy_off.y,
	   &zeitmaske,
	   &rauch);
}


bool fabesch_t::laden(FILE *file)
{
    char line[256];

    eingang = new vector_tpl<ware_t> (4);
    ausgang = new vector_tpl<ware_t> (4);

    read_line(line, 255, file);
    line[strlen(line)-1] = 0;
    check("factory name", line, "name=");
    name = strdup(line+5);

//    printf("Reading industry '%s'\n", name);

    read_line(line, 255, file);
    check(name, line, "loco=");

    switch(line[5]) {
    case 'L':
	platzierung = Land;
	break;
    case 'W':
	platzierung = Wasser;
	break;
    case 'C':
	platzierung = Stadt;
	break;

    default:
	dbg->error("fabesch_t::laden(FILE *file)"
		   "Warning: illegal placement constraint '%s' in industry.tab, using 'land' instead",
		   line);

	platzierung = Land;
    }
    read_line(line, 255, file);
    check(name, line, "weight=");
    sscanf(line, "weight=%d", &gewichtung);

    read_line(line, 255, file);
    check(name, line, "prod=");
    sscanf(line, "prod=%d", &produktivitaet);

    read_line(line, 255, file);
    check(name, line, "range=");
    sscanf(line, "range=%d", &bereich);

    read_line(line, 255, file);
    check(name, line, "smoke=");
    if(line[6] != 'N') {
	raucherdaten = new rabesch_t(line);
    } else {
	raucherdaten = NULL;
    }

    read_line(line, 255, file);
    check(name, line, "supp=");
    sscanf(line, "supp=%d", &anz_lieferanten);

    read_line(line, 255, file);
    line[strlen(line)-1] = 0;
    check(name, line, "input=");

    while(strcmp(line+6, "None")) {
	ware_t * ware = new ware_t( *warenbauer::gib_info(line+6) );

	read_line(line, 255, file);
	check(name, line, "capacity=");
	sscanf(line, "capacity=%d", &ware->max);
	ware->max <<= fabrik_t::precision_bits;

	eingang->append( *ware );

	read_line(line, 255, file);
	line[strlen(line)-1] = 0;
	check(name, line, "input=");
    }

    read_line(line, 255, file);
    check(name, line, "capacity=0");

    read_line(line, 255, file);
    line[strlen(line)-1] = 0;
    check(name, line, "output=");

    while(strcmp(line+7, "None")) {
	ware_t * ware = new ware_t( *warenbauer::gib_info(line+7) );

	read_line(line, 255, file);
	check(name, line, "capacity=");
	sscanf(line, "capacity=%d", &ware->max);
	ware->max <<= fabrik_t::precision_bits;

	ausgang->append( *ware );

	read_line(line, 255, file);
	line[strlen(line)-1] = 0;
	check(name, line, "output=");
    }

    read_line(line, 255, file);
    check(name, line, "capacity=0");

    /* Hajo: read the factory color */
    read_line(line, 255, file);
    line[strlen(line)-1] = 0;
    check(name, line, "color=");
    sscanf(line, "color=%d", &kennfarbe);

    return true;
}

fabesch_t::~fabesch_t()
{
    delete eingang;
    delete ausgang;
}
