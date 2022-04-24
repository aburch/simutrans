/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef PAKSET_DOWNLOADER_H
#define PAKSET_DOWNLOADER_H

#include "../simtypes.h"
#include "../tpl/vector_tpl.h"

typedef struct {
	const char *url;
	const char *name;
	const char *version;
	uint size;
} paksetinfo_t;


// download and install pak into current path
bool pak_download(vector_tpl<paksetinfo_t *>);

// check if pakset is installed, and if yes, check version string
bool pak_need_update(const paksetinfo_t *);

// check, if we can handle these kindS of paks with the current configuration
bool pak_can_download(const paksetinfo_t *);

bool pak_remove(const paksetinfo_t*);

#endif
