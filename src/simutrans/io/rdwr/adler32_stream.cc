/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "adler32_stream.h"

#include "../../simdebug.h"

#include <zlib.h>


adler32_stream_t::adler32_stream_t() :
	rdwr_stream_t(true)
{
	adler_checksum = adler32(0, NULL, 0);
	status = STATUS_OK;
}


size_t adler32_stream_t::read(void *, size_t)
{
	dbg->fatal("adler32_stream_t::read", "Cannot reconstruct original message from checksum!");
	return 0;
}


size_t adler32_stream_t::write(const void *buf, size_t len)
{
	adler_checksum = adler32(adler_checksum, static_cast<const uint8 *>(buf), len);
	return len;
}


uint32 adler32_stream_t::get_hash()
{
	const uint32 result = adler_checksum;
	adler_checksum = adler32(0, NULL, 0);
	return result;
}
