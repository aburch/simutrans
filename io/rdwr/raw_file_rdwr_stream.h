/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RDWR_RAW_FILE_RDWR_STREAM_H
#define IO_RDWR_RAW_FILE_RDWR_STREAM_H


#include "rdwr_stream.h"

#include <cstdio>


/// Reads/writes raw data from/to a file.
class raw_file_rdwr_stream_t : public rdwr_stream_t
{
public:
	raw_file_rdwr_stream_t(const std::string &filename, bool writing);

	/// Takes ownership of an already open file.
	/// @p writing must match the mode with which the file was opened.
	raw_file_rdwr_stream_t(FILE *f, bool writing);

	~raw_file_rdwr_stream_t();

public:
	/// @copydoc rdwr_stream_t::read
	size_t read(void *buf, size_t len) OVERRIDE;

	/// @copydoc rdwr_stream_t::write
	size_t write(const void *buf, size_t len) OVERRIDE;

private:
	FILE *file;
};


#endif
