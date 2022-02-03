/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_PAKSET_INFO_H
#define NETWORK_PAKSET_INFO_H


#include "../tpl/stringhashtable_tpl.h"
#include "../descriptor/objversion.h"
#include "checksum.h"


class pakset_info_t
{
	/**
	 * checksums of all desc's
	 * since their names are unique we can index them by name
	 */
	static stringhashtable_tpl<checksum_t*> info;

	/**
	 * pakset checksum
	 */
	static checksum_t general;

public:
	static const checksum_t& get_pakset_checksum() { return general; }
	static const stringhashtable_tpl<checksum_t*>& get_info() { return info; }

	static void calculate_checksum();
	static checksum_t* get_checksum() { return &general; }

	static void append(const char* name, obj_type type, checksum_t *chk);

	static void debug();

	friend class nwc_pakset_info_t;
};

#endif
