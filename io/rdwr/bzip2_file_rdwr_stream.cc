/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "bzip2_file_rdwr_stream.h"

#include "../../sys/simsys.h"

#include <cassert>


bzip2_file_rdwr_stream_t::bzip2_file_rdwr_stream_t(const std::string &filename, bool writing) :
	rdwr_stream_t(writing)
{
	if (is_writing()) {
		fp = dr_fopen(filename.c_str(), "wb");
		bzfp = BZ2_bzWriteOpen( &bse, fp, 9, 0, 30 /* default is 30 */ );
	}
	else {
		fp = dr_fopen(filename.c_str(), "rb");
		bzfp = BZ2_bzReadOpen( &bse, fp, 0, 0, NULL, 0 );
	}

	if(  bse!=BZ_OK  ) {
		status = STATUS_ERR_CORRUPT;
	}

	status = STATUS_OK;
}


bzip2_file_rdwr_stream_t::~bzip2_file_rdwr_stream_t()
{
	if (is_writing()) {
		// BZLIB seems to eat the last byte if it is at an odd position
		// => we just write a dummy zero padding byte
		if (status == STATUS_OK) {
			write( "", 1 );
		}

		BZ2_bzWriteClose( &bse, bzfp, 0, NULL, NULL );
	}
	else {
		BZ2_bzReadClose( &bse, bzfp );
	}

	fclose( fp );
}


size_t bzip2_file_rdwr_stream_t::read(void *buf, size_t len)
{
	assert(!is_writing());
	assert(bse == BZ_OK || bse == BZ_STREAM_END);
	assert(len < 0x7FFFFFFFU);

	const int bytes_read = (bse == BZ_OK) ? BZ2_bzRead(&bse, bzfp, buf, len) : 0;

	switch (bse) {
	case BZ_OK:         status = STATUS_OK;          return bytes_read;
	case BZ_STREAM_END: status = STATUS_EOF;         return bytes_read;
	default:            status = STATUS_ERR_CORRUPT; return 0;
	}
}


size_t bzip2_file_rdwr_stream_t::write(const void* buf, size_t len)
{
	assert(is_writing());
	assert(bse==BZ_OK);

	BZ2_bzWrite( &bse, bzfp, const_cast<void *>(buf), len);

	if (bse == BZ_OK) {
		return len;
	}
	else {
		status = STATUS_ERR_FULL;
		return 0;
	}
}

