/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_MEMORY_RW_H
#define NETWORK_MEMORY_RW_H


/* This is a class to write data to memory in intel byte order
 * on every architecture
 */


#include "../simtypes.h"

class plainstring;

class memory_rw_t {
private:
	// pointer to the buffer
	char *ptr;
	// actual read/write position
	uint32 index;
	// maximal buffer size
	uint32 max_size;

	bool saving:1;
	bool overflow:1;
public:
	memory_rw_t( void *ptr, uint32 max, bool saving );

protected:
	void set_max_size( uint32 new_max_size ) { max_size = new_max_size; }

	void set_index(uint32 new_index) { index = new_index; }

public:
	uint32 get_current_index() const { return index; }

	bool is_saving() const { return saving; }
	bool is_loading() const { return !saving; }

	bool is_overflow() const { return overflow; }

	void rdwr_byte(sint8 &c);
	void rdwr_byte(uint8 &c);
	void rdwr_short(sint16 &i);
	void rdwr_short(uint16 &i);
	void rdwr_long(sint32 &i);
	void rdwr_long(uint32 &i);
	void rdwr_longlong(sint64 &i);
	void rdwr_bool(bool &i);
	void rdwr_double(double &dbl);
	// s: pointer to a string allocated with malloc!
	void rdwr_str(char *&s);

	void rdwr_str(plainstring& s);

	/**
	 * appends the contents of the other buffer from [0 .. index-1]
	 * (only if saving)
	 */
	void append(const memory_rw_t &mem);

	/**
	 * appends the contents of the other buffer from [index .. max_size-1]
	 * (only if reading)
	 */
	void append_tail(const memory_rw_t &mem);

private:
	// Low-Level Read / Write function
	void rdwr(void *data, uint32 len);
};
#endif
