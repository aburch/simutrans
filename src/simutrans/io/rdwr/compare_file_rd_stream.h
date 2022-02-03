/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RDWR_COMPARE_FILE_RD_STREAM_H
#define IO_RDWR_COMPARE_FILE_RD_STREAM_H


#include "rdwr_stream.h"

#include <cstdio>


/// Compares two streams: reads from both, throws error if streams are different.
class compare_file_rd_stream_t : public rdwr_stream_t
{
	size_t our_len;
	char  *our_buf;
public:
	/// Takes two streams, does not take ownership
	compare_file_rd_stream_t(rdwr_stream_t *s1, rdwr_stream_t* s2);

	~compare_file_rd_stream_t();

public:
	/// @copydoc rdwr_stream_t::read
	size_t read(void *buf, size_t len) OVERRIDE;

	// not implemented
	size_t write(const void *, size_t) OVERRIDE { return -1; }

private:
	rdwr_stream_t *stream1, *stream2;
};


#endif
