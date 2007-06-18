/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "warenziel.h"
#include "koord.h"

#include "../simmem.h"
#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"

#include "../simtypes.h"
#include "loadsave.h"

warenziel_t::warenziel_t(loadsave_t *file)
{
  rdwr(file);
}


void
warenziel_t::rdwr(loadsave_t *file)
{
	// dummy ...
	koord ziel;
  ziel.rdwr(file);

  const char *tn = NULL;
  if(file->is_saving()) {
		tn = warenbauer_t::gib_info_catg_index(catg_index)->gib_name();
  }
  file->rdwr_str(tn, " ");
  if(file->is_loading()) {
		halt = halthandle_t();
		catg_index = warenbauer_t::gib_info(tn)->gib_catg_index();
    guarded_free(const_cast<char *>(tn));
  }
}
