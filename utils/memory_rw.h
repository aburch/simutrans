#ifndef _MEMORY_RW_H
#define _MEMORY_RW_H

/* This is a class to write data to memory in intel byte order
 * on every architecture
 */


#include "../simtypes.h"

class memory_rw_t {
private:
	char *ptr;
	bool saving:1;
	bool overflow:1;
	uint32 index;
	uint32 max_size;

public:
	memory_rw_t( void *p, uint32 max, bool save ) { init(p,max,save); }

	void init( void *ptr, uint32 max, bool saving );

	void set_max_size( uint32 new_max_size ) { max_size = new_max_size; }
	uint32 get_current_size() const { return index; }
	void set_index(uint32 new_index) { index = new_index; }


	// Low-Level Read / Write function
	void rdwr(void *data, uint32 len);

	bool is_saving() const { return saving; }
	bool is_loading() const { return !saving; }

	bool is_overflow() const { return overflow; }
	bool is_finished() const { return index<max_size; }

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
};
#endif
