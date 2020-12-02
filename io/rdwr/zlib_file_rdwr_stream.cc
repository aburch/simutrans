/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "zlib_file_rdwr_stream.h"

#include "../../sys/simsys.h"
#include "../../macros.h"
#include "../../simdebug.h"

#include <cassert>


zlib_file_rdwr_stream_t::zlib_file_rdwr_stream_t(const std::string &filename, bool writing, int compression) :
	rdwr_stream_t(writing)
{
	if (is_writing()) {
		compression = clamp( compression, 1, 9 );
		char compr[4] = { 'w', 'b', (char)('0' + compression), 0 };
		gzfp = dr_gzopen(filename.c_str(), compr);
	}
	else {
		gzfp = dr_gzopen(filename.c_str(), "rb");
	}
	gzbuffer(gzfp, 65536);
	status = STATUS_OK;
}


zlib_file_rdwr_stream_t::~zlib_file_rdwr_stream_t()
{
	if (is_writing()) {
		gzflush(gzfp, Z_FINISH);
	}

	gzclose(gzfp);
}


size_t zlib_file_rdwr_stream_t::read(void *buf, size_t len)
{
	assert(!is_writing());

	const int bytes_read = gzread(gzfp, buf, len);

	if (bytes_read >= 0 && (size_t)bytes_read == len) {
		status = STATUS_OK;
		return bytes_read;
	}
	else if (bytes_read != -1) {
		// not error => eof reached
		status = STATUS_EOF;
		return bytes_read;
	}
	else {
		// error
		int errnum = 0;
		const char *errmsg = gzerror(gzfp, &errnum);
		if (!errmsg) errmsg = "<unknown error>";

		dbg->error("zlib_file_rdwr_stream_t::read", "Error: %s", errmsg);

		status = STATUS_ERR_CORRUPT;
		return 0;
	}
}


size_t zlib_file_rdwr_stream_t::write(const void *buf, size_t len)
{
	assert(is_writing());
	const int bytes_written = gzwrite(gzfp, const_cast<void *>(buf), len);

	if (bytes_written == 0) {
		status = STATUS_ERR_FULL;
		return 0;
	}
	else {
		status = STATUS_OK;
		return bytes_written;
	}
}


