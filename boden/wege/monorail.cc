/* schiene.cc
 *
 * Schienen für Simutrans
 *
 * Überarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "../../simdebug.h"
#include "../../simworld.h"
#include "../../blockmanager.h"
#include "../grund.h"
#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"
#include "../../besch/weg_besch.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/wegbauer.h"

#include "monorail.h"

void monorail_t::info(cbuffer_t & buf) const
{
	weg_t::info(buf);

	buf.append(translator::translate("\nRail block "));
	buf.append(gib_blockstrecke().get_id());
	buf.append("\n");

	gib_blockstrecke()->info(buf);

	buf.append(translator::translate("\n\nRibi (unmasked) "));
	buf.append(gib_ribi_unmasked());

	buf.append(translator::translate("\nRibi (masked) "));
	buf.append(gib_ribi());
	buf.append("\n");
}
