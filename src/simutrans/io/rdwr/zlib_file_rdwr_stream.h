/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RDWR_ZLIB_FILE_RDWR_STREAM_H
#define IO_RDWR_ZLIB_FILE_RDWR_STREAM_H


#include "rdwr_stream.h"

#include <zlib.h>


/// Reads/writes data from/to a zlib/gzip (deflate) compressed file.
class zlib_file_rdwr_stream_t : public rdwr_stream_t
{
public:
	zlib_file_rdwr_stream_t(const std::string &filename, bool writing, int compression);
	~zlib_file_rdwr_stream_t();

public:
	/// @copydoc rdwr_stream_t::read
	size_t read(void *buf, size_t len) OVERRIDE;

	/// @copydoc rdwr_stream_t::write
	size_t write(const void *buf, size_t len) OVERRIDE;

private:
	gzFile gzfp;
};


#endif
