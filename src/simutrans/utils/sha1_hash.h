/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_SHA1_HASH_H
#define UTILS_SHA1_HASH_H


#include "../simtypes.h"


/**
 * Stores the value of a SHA-1 message digest.
 */
class sha1_hash_t
{
public:
	/// @warning @p raw_value must be at least 20 bytes long
	sha1_hash_t(const uint8 *raw_value);

	/// Initializes hash to all zeroes
	sha1_hash_t();

public:
	void clear();

	bool empty() const;

	uint8 &operator[](size_t i);
	const uint8 &operator[](size_t i) const;

	bool operator==(const sha1_hash_t &o) const;
	bool operator!=(const sha1_hash_t &o) const;

private:
	uint8 hash[20];
};


typedef sha1_hash_t pwd_hash_t;


#endif
