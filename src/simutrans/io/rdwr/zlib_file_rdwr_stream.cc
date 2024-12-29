/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "zlib_file_rdwr_stream.h"

#include "../../sys/simsys.h"
#include "../../macros.h"
#include "../../simdebug.h"

#include <cassert>
#include <cerrno>


zlib_file_rdwr_stream_t::zlib_file_rdwr_stream_t(const std::string &filename, bool writing, int compression) :
	rdwr_stream_t(writing)
{
	// Should be 0 anyway, but make sure to only catch errors from zlib
	// zlib might not set errno appropriately in all error cases
	// (source: https://refspecs.linuxbase.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/zlib-gzopen-1.html)
	// so reset it to a known value. The value will be translated to GENERIC_ERROR by set_status_from_errno
	errno = 0;

	if (is_writing()) {
		compression = clamp( compression, 1, 9 );
		char compr[4] = { 'w', 'b', (char)('0' + compression), 0 };
		gzfp = dr_gzopen(filename.c_str(), compr);
	}
	else {
		gzfp = dr_gzopen(filename.c_str(), "rb");
	}

	if (gzfp == Z_NULL) {
		set_status_from_errno();
	}
	else {
		gzbuffer(gzfp, 65536);
		status = STATUS_OK;
	}
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
	assert(len > 0);

	const int bytes_written = gzwrite(gzfp, const_cast<void *>(buf), len);

	if (bytes_written <= 0) {
		// error occurred
		int errnum = Z_OK;
		gzerror(gzfp, &errnum);

		switch (errnum) {
			case Z_MEM_ERROR:    status = STATUS_ERR_OUT_OF_MEMORY;   return 0;
			case Z_STREAM_ERROR: status = STATUS_ERR_NOT_INITIALIZED; return 0;
			case Z_BUF_ERROR:    status = STATUS_ERR_FULL;            return 0;
			case Z_ERRNO:        set_status_from_errno(); return 0;
		}

		status = STATUS_ERR_GENERIC_ERROR;
		return 0;
	}
	else {
		status = STATUS_OK;
		return bytes_written;
	}
}


void zlib_file_rdwr_stream_t::set_status_from_errno()
{
	switch (errno) {
		case EPERM:
		case ENOENT:
		case EIO:
		case EBADF:
		case EBADFD:
		case EACCES:
		case ENODEV:
		case EISDIR:
		case ELOOP:
			status = STATUS_ERR_FILE_INACCESSIBLE;
			break;

		case ENOMEM:
			status = STATUS_ERR_OUT_OF_MEMORY;
			break;

		case EFBIG:
		case ENOSPC:
			status = STATUS_ERR_FULL;
			break;

		default:
			status = STATUS_ERR_GENERIC_ERROR;
			break;
	}
}

