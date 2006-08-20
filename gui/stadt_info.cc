/*
 * stadt_info.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */
#ifdef _MSC_VER
#include <string.h>
#endif

#include <stdio.h>

#include "../simdebug.h"
#include "../simcity.h"
#include "../simcolor.h"
#include "../dataobj/translator.h"
#include "../utils/simstring.h"

#include "stadt_info.h"

extern "C" {
#include "../simgraph.h"
}


stadt_info_t::stadt_info_t(stadt_t *stadt) :
  gui_frame_t("Stadtinformation")
{
    this->stadt = stadt;

    name_input.setze_text(stadt->access_name(), 30);
    name_input.setze_groesse(koord(100, 14));
    name_input.setze_pos(koord(8, 8));

    setze_opaque(true);

    add_komponente(&name_input);

    setze_fenstergroesse(koord(368, 224));
}


void
stadt_info_t::zeichnen(koord pos, koord gr)
{
    const array2d_tpl<unsigned char> * pax_alt = stadt->gib_pax_ziele_alt();
    const array2d_tpl<unsigned char> * pax_neu = stadt->gib_pax_ziele_neu();

    char buf [4096];

    gui_frame_t::zeichnen(pos, gr);

    /*
    sprintf(buf, "%s: %d (+%d)",
            translator::translate("City size"),
            stadt->gib_einwohner(),
            stadt->gib_wachstum()
	    );

    display_proportional(pos.x+8, pos.y+48, buf, ALIGN_LEFT, SCHWARZ, true);


    const koord lo = stadt->get_linksoben();
    const koord ru = stadt->get_rechtsunten();

    sprintf(buf, "%d,%d - %d,%d",
	    lo.x, lo.y, ru.x, ru.y
	    );

    display_proportional(pos.x+8, pos.y+59, buf, ALIGN_LEFT, SCHWARZ, true);

    */

    stadt->info(buf);

    display_multiline_text(pos.x+8, pos.y+48, buf, SCHWARZ);



    tstrncpy(buf, translator::translate("Passagierziele"), 64);
    display_proportional(pos.x+144, pos.y+24, buf, ALIGN_LEFT, SCHWARZ, true);

    tstrncpy(buf, translator::translate("letzen Monat: diesen Monat:"), 64);
    display_proportional(pos.x+144, pos.y+36, buf, ALIGN_LEFT, SCHWARZ, true);

    display_array_wh(pos.x+144, pos.y+52, 96, 96, pax_alt->to_array());
    display_array_wh(pos.x+144 + 96 + 16, pos.y+52, 96, 96, pax_neu->to_array());
//    display_int_array_wh(pos.x+144, pos.y+52, 96, 96, pax_alt->to_array());
//    display_int_array_wh(pos.x+144 + 96 + 16, pos.y+52, 96, 96, pax_neu->to_array());


    sprintf(buf, "%s: %d/%d",
            translator::translate("Passagiere"),
            stadt->gib_pax_transport(),
            stadt->gib_pax_erzeugt());

    display_proportional(pos.x+144, pos.y+180, buf, ALIGN_LEFT, SCHWARZ, true);
}
