/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RDWR_BZIP2_FILE_RDWR_STREAM_H
#define IO_RDWR_BZIP2_FILE_RDWR_STREAM_H


#include "rdwr_stream.h"

#include <bzlib.h>


/// Reads/writes data from/to a bzip2 compressed file.
class bzip2_file_rdwr_stream_t : public rdwr_stream_t
{
public:
	bzip2_file_rdwr_stream_t(const std::string &filename, bool writing);
	~bzip2_file_rdwr_stream_t();

public:
	/// @copydoc rdwr_stream_t::read
	size_t read(void *buf, size_t len) OVERRIDE;

	/// @copydoc rdwr_stream_t::write
	size_t write(const void *buf, size_t len) OVERRIDE;

private:
	FILE *fp;
	BZFILE *bzfp;
	int bse;
};


#endif
