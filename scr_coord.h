#ifndef scr_coord_h
#define scr_coord_h

#include "dataobj/koord.h"
#include "simtypes.h"

// Screen coordinate type
typedef sint16 scr_coord_val;

// Two dimensional point type
class scr_coord
{
public:
	scr_coord_val x;
	scr_coord_val y;

	scr_coord() { x=0; y=0; } // should this be rather undined constant or so!

	scr_coord( scr_coord_val x_, scr_coord_val y_ ) { x = x_; y=y_; }

	scr_coord(const koord pos_par) { x = pos_par.x; y = pos_par.y; }

	bool operator ==(const scr_coord& other) const { return ((x-other.x) | (y-other.y)) == 0; }

	bool operator !=(const scr_coord& other) const { return !(other == *this ); }

	const scr_coord operator +(const scr_coord& other ) const { return scr_coord( other.x + x, other.y + y); }

	const scr_coord operator -(const scr_coord& other ) const { return scr_coord( x - other.x, y - other.y ); }

	const scr_coord& operator +=(const scr_coord& other ) {
		x += other.x;
		y += other.y;
		return *this;
	}

	const scr_coord& operator -=(const scr_coord& other ) {
		x += other.x;
		y += other.y;
		return *this;
	}

	void add_offset( scr_coord_val delta_x, scr_coord_val delta_y ) {
		x += delta_x;
		y += delta_y;
	}

	void set( scr_coord_val x_par, scr_coord_val y_par ) {
		x = x_par;
		y = y_par;
	}
};

#endif
