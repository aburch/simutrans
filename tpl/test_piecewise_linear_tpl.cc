/**
 * Copyright 2013 Nathanael Nerode
 * This file is licensed under the Artistic License as part of the Simutrans project. (See license.txt)
 * This file is also licensed under the Artistic License 2.0 and the GNU General Public License 3.0.
 *
 * Unit Test for piecewise_linear_tpl.h
 * Do NOT link this into simutrans!  This is a unit test!
 */
#include "../simtypes.h"
#include "piecewise_linear_tpl.h"

// This is a hack, but it's worth it.  The templates need logging in order to link.
#include "../simdebug.cc"
#include "../utils/dumb-log.cc"

typedef piecewise_linear_tpl<sint8, sint32> widening_table;

int main( int argc, char** argv)
{
	widening_table foo(2);
	// widening_table foo = widening_table(2);
	foo.insert(5, 100);
	foo.insert(15, 220); // test out-of-order insertion
	foo.insert(10, 200);
	fprintf(stdout, "Piecewise linear table:\n");
	for (uint8 i = 0; i < 20; i++) {
		fprintf(stdout, "Key %i Value %i\n", i, foo(i) );
	}
}

