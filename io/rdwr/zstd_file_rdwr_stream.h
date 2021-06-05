/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RDWR_ZSTD_FILE_RDWR_STREAM_H
#define IO_RDWR_ZSTD_FILE_RDWR_STREAM_H


#include "raw_file_rdwr_stream.h"

#include <zstd.h>


#if !USE_ZSTD
#pragma message( "warning: Cannot use zstd_file_rdwr_stream_t: zstd not enabled")
#endif


/// Reads/writes data data from/to a zstd compressed file.
class zstd_file_rdwr_stream_t : public raw_file_rdwr_stream_t
{
public:
	zstd_file_rdwr_stream_t(const std::string &filename, bool writing, int compression);
	~zstd_file_rdwr_stream_t();

public:
	/// @copydoc rdwr_stream_t::read
	size_t read(void *buf, size_t len) OVERRIDE;

	/// @copydoc rdwr_stream_t::write
	size_t write(const void *buf, size_t len) OVERRIDE;

private:
	void *zbuff; // buffer for compressed data, i.e. file <-> zbuff

	ZSTD_inBuffer zin;
	ZSTD_outBuffer zout;
	ZSTD_CCtx *compression_context;
	ZSTD_DCtx *decompression_context;
};

#endif
