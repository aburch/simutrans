/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "raw_file_rdwr_stream.h"

#include "../../sys/simsys.h"

#include <cassert>


raw_file_rdwr_stream_t::raw_file_rdwr_stream_t(const std::string &filename, bool writing) :
	rdwr_stream_t(writing)
{
	file = dr_fopen(filename.c_str(), writing ? "wb" : "rb");
	if (!file) {
		status = STATUS_ERR_NOT_EXISTING;
	}

	status = STATUS_OK;
}


raw_file_rdwr_stream_t::raw_file_rdwr_stream_t(FILE *f, bool writing) :
	rdwr_stream_t(writing),
	file(f)
{
	if (!file) {
		status = STATUS_ERR_NOT_EXISTING;
	}
	else if (ferror(file)) {
		status = STATUS_ERR_CORRUPT;
	}
	else if (feof(file)) {
		status = STATUS_EOF;
	}
	else {
		status = STATUS_OK;
	}
}


raw_file_rdwr_stream_t::~raw_file_rdwr_stream_t()
{
	fflush(file);
	fclose(file);
}


size_t raw_file_rdwr_stream_t::read(void *buf, size_t len)
{
	assert(!is_writing());
	const size_t bytes_read = fread(buf, 1, len, file);

	if (bytes_read == len) {
		status = STATUS_OK;
		return bytes_read;
	}
	else if (feof(file)) {
		status = STATUS_EOF;
		return bytes_read;
	}
	else {
		status = STATUS_ERR_CORRUPT;
		return 0;
	}
}


size_t raw_file_rdwr_stream_t::write(const void *buf, size_t len)
{
	assert(is_writing());
	const size_t bytes_written = fwrite(buf, 1, len, file);

	if (bytes_written == len) {
		status = STATUS_OK;
	}
	else {
		status = STATUS_ERR_FULL;
	}

	return bytes_written;
}
