/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef IO_RDWR_RDWR_STREAM_H
#define IO_RDWR_RDWR_STREAM_H


#include "../../simtypes.h"

#include <string>


/// Either reads or writes data (but not both at the same time).
class rdwr_stream_t
{
public:
	/// Note: On failure, this does not throw. Instead, call @ref get_status() to check if
	/// @ref read() or @ref write() can be called. Derived classes must update @ref status accordingly.
	rdwr_stream_t(bool writing);
	virtual ~rdwr_stream_t() {}

public:
	enum status_t
	{
		STATUS_INVALID = -1,     ///< Not initialized
		STATUS_OK = 0,
		STATUS_EOF,              ///< (when reading) end of file/buffer reached
		STATUS_ERR_NOT_EXISTING, ///< (when reading) File does not exist
		STATUS_ERR_FULL,         ///< (when writing) No space left in buffer or hard drive
		STATUS_ERR_NO_VERSION,
		STATUS_ERR_FUTURE_VERSION,
		STATUS_ERR_DEPRECATED,   ///< Version too old
		STATUS_ERR_CORRUPT       ///< ex.: file malformed, file no longer accessible
	};

	status_t get_status() const { return status; }
	bool is_writing() const { return writing; }
	bool is_reading() const { return !writing; }

public:
	/// Read at most @p len bytes into @p buf; must not be called when writing.
	/// If this function fails, call @ref get_status() to get information about the type of error.
	///
	/// @param buf Must not be NULL.
	/// @param len Must not be 0.
	///
	/// @returns @p len, if successful.
	/// @returns The number of bytes successfully read, if end-of-data occurs.
	/// @returns Undefined (but not @p len), if an error occurred.
	virtual size_t read(void *buf, size_t len) = 0;

	/// Write at most @p len bytes from @p buf; must not be called when reading.
	/// If this function fails, call @ref get_status() to get information about the type of error.
	///
	/// @param buf Must not be NULL.
	/// @param len Must not be 0.
	///
	/// @returns @p len, if successful.
	/// @returns Undefined (but not @p len), if an error occurred.
	virtual size_t write(const void *buf, size_t len) = 0;

protected:
	/// @warning This must be updated to the correct value when @p read() or @p write() or the constructor fails.
	status_t status;

private:
	const bool writing;
};


#endif
