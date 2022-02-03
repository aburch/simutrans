/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RDWR_ADLER32_STREAM_H
#define IO_RDWR_ADLER32_STREAM_H


#include "rdwr_stream.h"


/// Computes the adler32 checksum over some data
class adler32_stream_t : public rdwr_stream_t
{
public:
	adler32_stream_t();

public:
	/// @copydoc rdwr_stream_t::write
	size_t write(const void *buf, size_t len) OVERRIDE;

	/// @returns the adler32 checksum of all the data written since the last get_hash()
	uint32 get_hash();

private:
	/// DO NOT USE!
	size_t read(void *buf, size_t len) OVERRIDE;

private:
	uint32 adler_checksum;
};


#endif
