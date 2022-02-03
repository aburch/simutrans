/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "zstd_file_rdwr_stream.h"

#include "../../dataobj/environment.h"
#include "../../simdebug.h"
#include "../../simmem.h"

#include <zstd.h>

#define ZSTD_FILE_BUF_SIZE (1 << 20) // 1MiB


zstd_file_rdwr_stream_t::zstd_file_rdwr_stream_t(const std::string &filename, bool writing, int compression_level) :
	raw_file_rdwr_stream_t(filename, writing),
	zbuff(NULL)
{
	if (status != STATUS_OK) {
		return; // Could not open file
	}

	if (writing) {
		// compressing
		compression_context = ZSTD_createCCtx();
		if(  compression_context == NULL  ) {
			// zstd could not init
			status = STATUS_ERR_CORRUPT;
			return;
		}
#if ZSTD_VERSION_MAJOR<=1  &&  ZSTD_VERSION_MINOR<=3
		ZSTD_initCStream( compression_context, compression_level );
#else
		size_t ret1 = ZSTD_CCtx_setParameter( compression_context, ZSTD_c_compressionLevel, compression_level );
		if (ZSTD_isError(ret1)) {
			dbg->error("zstd_file_rdwr_stream_t::zstd_file_rdwr_stream_t", "Cannot set compression level: %s", ZSTD_getErrorName(ret1));
			status = STATUS_ERR_CORRUPT;
			return;
		}

// 		const size_t ret2 = ZSTD_CCtx_setParameter( compression_context, ZSTD_c_checksumFlag, 1 );
// 		if (ZSTD_isError(ret2)) {
// 			dbg->error("zstd_file_rdwr_stream_t::zstd_file_rdwr_stream_t", "Cannot enable file checksumming: %s", ZSTD_getErrorName(ret2));
// 			status = STATUS_ERR_CORRUPT;
// 			return;
// 		}

#ifdef MULTI_THREAD
		ret1 = ZSTD_CCtx_setParameter( compression_context, ZSTD_c_nbWorkers, env_t::num_threads );
		if (ZSTD_isError(ret1)) {
			// since compression should continue anyway, do not stop!
			dbg->warning("zstd_file_rdwr_stream_t::zstd_file_rdwr_stream_t", "Cannot set workers: %s", ZSTD_getErrorName(ret1));
		}
#endif

#endif
		// the additional magic for zstd
		if (raw_file_rdwr_stream_t::write("ZD", 2) != 2) {
			return;
		}
	}
	else {
		// decompressing
		decompression_context = ZSTD_createDCtx();

		if (decompression_context == NULL) {
			status = STATUS_ERR_CORRUPT;
			return;
		}

		char buf[2];
		if (raw_file_rdwr_stream_t::read(buf, 2) != 2) {
			status = STATUS_ERR_CORRUPT;
			return;
		}
		else if (buf[0] != 'Z' || buf[1] != 'D') {
			status = STATUS_ERR_CORRUPT;
			return;
		}
	}

	zbuff = xmalloc(ZSTD_FILE_BUF_SIZE);

	if (writing) {
		zin.src = NULL;
		zin.size = 0;
		zin.pos = 0;

		zout.dst = zbuff;
		zout.size = ZSTD_FILE_BUF_SIZE;
		zout.pos = 0;
	}
	else {
		zin.src = zbuff;
		zin.size = ZSTD_FILE_BUF_SIZE;
		zin.pos = ZSTD_FILE_BUF_SIZE;

		zout.dst = NULL;
		zout.size = 0;
		zout.pos = 0;
	}

	status = STATUS_OK;
}


zstd_file_rdwr_stream_t::~zstd_file_rdwr_stream_t()
{
	if (is_writing()) {
		// write zero length dummy to indicate end of data
		zin.src = "";
		zin.size = 0;
		zin.pos = 0;

		zout.dst = zbuff;
		zout.size = ZSTD_FILE_BUF_SIZE;
		zout.pos = 0;

		size_t ret;
		do {
			zout.pos = 0;
			ret = ZSTD_endStream( compression_context, &zout );
			if (ZSTD_isError(ret)) {
				dbg->error("zstd_file_rdwr_stream_t::~zstd_file_rdwr_stream_t", "Error flushing stream: %s", ZSTD_getErrorName(ret));
			}

			raw_file_rdwr_stream_t::write( zout.dst, zout.pos );
		} while( ret>0 );

		ZSTD_freeCCtx( compression_context );
	}
	else {
		ZSTD_freeDCtx( decompression_context );
	}

	free( zbuff );
}


size_t zstd_file_rdwr_stream_t::read(void *buf, size_t len)
{
	zout.dst = buf;
	zout.size = len;
	zout.pos = 0;

	do {
		// first decompress from remaining input buffer
		while(  zin.pos < zin.size  &&  zout.pos < zout.size  ) {
			const size_t ret = ZSTD_decompressStream( decompression_context, &zout, &zin );
			if (ZSTD_isError(ret)) {
				dbg->error("zstd_file_rdwr_stream_t::read", "Error during decompression: %s", ZSTD_getErrorName(ret));
				status = STATUS_ERR_CORRUPT;
				return 0;
			}

			if(  ret == 0  ) {
				zout.size = zout.pos;
			}
		}

		// not enough data to fill output buffer => read more data from file
		if(  zout.pos < zout.size  ) {
			const size_t bytes_read = raw_file_rdwr_stream_t::read(zbuff, ZSTD_FILE_BUF_SIZE);
			zin.pos = 0;
			zin.size = bytes_read;

			if (status != rdwr_stream_t::STATUS_OK && status != rdwr_stream_t::STATUS_EOF) {
				// an error occurred, status is already set to the appropriate value
				return 0;
			}

			// reset status since EOF is indicated by end of decompressed data,
			// not end of compressed data
			status = STATUS_OK;
		}
	} while(  zin.pos < zin.size  &&  zout.pos < zout.size  );


	if (zout.pos < len) {
		// end of decompressed data reached
		status = STATUS_EOF;
	}

	return zout.pos;
}


size_t zstd_file_rdwr_stream_t::write(const void *buf, size_t len)
{
	// compress the next data
	zin.src = buf;
	zin.size = len;
	zin.pos = 0;

	while(  zin.pos < zin.size  ) {
		zout.pos = 0;

		const size_t ret = ZSTD_compressStream( compression_context, &zout, &zin );
		if (ZSTD_isError(ret)) {
			dbg->error("zstd_file_rdwr_stream_t::write", "Error during compression: %s", ZSTD_getErrorName(ret));
			status = STATUS_ERR_CORRUPT;
			return 0;
		}

		if (raw_file_rdwr_stream_t::write( zout.dst, zout.pos ) != zout.pos) {
			status = STATUS_ERR_FULL;
			return 0;
		}
	}

	return zin.pos;
}
