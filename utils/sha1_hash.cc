/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "sha1_hash.h"

#include "sha1.h"
#include "../macros.h"

#include <cassert>


sha1_hash_t::sha1_hash_t()
{
	clear();
}


sha1_hash_t::sha1_hash_t(const uint8 *value)
{
	memcpy(hash, value, 20);
}


void sha1_hash_t::clear()
{
	MEMZERO(hash);
}


bool sha1_hash_t::empty() const
{
	for (uint8 const* i = hash; i != endof(hash); ++i) {
		if (*i != 0) {
			return false;
		}
	}
	return true;
}


uint8 & sha1_hash_t::operator[](const size_t i)
{
	assert(i < 20);
	return hash[i];
}


const uint8 & sha1_hash_t::operator[](size_t i) const
{
	assert(i < 20);
	return hash[i];
}


bool sha1_hash_t::operator==(const sha1_hash_t &o) const
{
	return memcmp(hash, o.hash, 20) == 0;
}


bool sha1_hash_t::operator!=(const sha1_hash_t& o) const
{
	return memcmp(hash, o.hash, 20) != 0;
}
