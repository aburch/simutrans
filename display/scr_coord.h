#ifndef scr_coord_h
#define scr_coord_h

#include <assert.h>
#include "../dataobj/koord.h"
#include "../simtypes.h"



// Screen coordinate type
typedef sint16 scr_coord_val;

// Rectangle relations
enum rect_relation_t { RECT_RELATION_INSIDE, RECT_RELATION_OVERLAP, RECT_RELATION_OUTSIDE };

// Two dimensional coordinate type
class scr_coord
{
public:
	scr_coord_val x;
	scr_coord_val y;

	// Constructors
	scr_coord( void ) { x = y = 0; }
	scr_coord( scr_coord_val x_, scr_coord_val y_ ) { x = x_; y=y_; }
	scr_coord( const koord pos_par) { x = pos_par.x; y = pos_par.y; }

	// temporary until koord has been replaced by scr_coord.
	operator koord() { return koord(x,y); }

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



// Very simple scr_size struct.
class scr_size
{
public:
	scr_coord_val w;
	scr_coord_val h;

	// Constructors
	scr_size( void ) { w = h = 0; }
	scr_size( scr_coord_val w_par, scr_coord_val h_par) { w = w_par; h = h_par; }
	scr_size( const scr_size& size ) { w = size.w; h=size.h; }

	operator koord() const { return koord(w,h); }
	operator scr_coord() const { return scr_coord(w,h); }

	bool operator ==(const scr_size& other) const { return ((w-other.w) | (h-other.h)) == 0; }
	bool operator !=(const scr_size& other) const { return !(other == *this ); }
	const scr_size operator +(const scr_size& other ) const { return scr_size( other.w + w, other.h + h); }
	const scr_size operator -(const scr_size& other ) const { return scr_size( w - other.w, h - other.h ); }

	const scr_size& operator +=(const scr_size& other ) {
		w += other.w;
		h += other.h;
		return *this;
	}

	const scr_size& operator -=(const scr_size& other ) {
		w += other.w;
		h += other.h;
		return *this;
	}

};


// Rectangle type
class scr_rect
{
protected:
	void init( scr_coord_val x_par, scr_coord_val y_par, scr_coord_val w_par, scr_coord_val h_par ) {
		x = x_par;
		y = y_par;
		w = w_par;
		h = h_par;
	}

public:
	scr_coord_val x;
	scr_coord_val y;
	scr_coord_val w;
	scr_coord_val h;

	// Constructors
	scr_rect( void ) { init(0,0,0,0); }
	scr_rect( const scr_coord& pt ) { init( pt.x, pt.y, 0, 0 ); }
	scr_rect( const scr_coord& pt, scr_coord_val w, scr_coord_val h ) { init( pt.x, pt.y, w, h ); }
	scr_rect( scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h ) { init( x, y, w, h ); }
	scr_rect( scr_size size ) { w = size.w; h=size.h; }
	scr_rect( const scr_coord& point1, const scr_coord& point2 ) { init( point1.x, point1.y, point2.x-point1.x, point2.y-point1.y ); }

	// Type cast operators
	operator scr_size()   { return scr_size (w,h); }
	operator scr_coord()  { return scr_coord(x,y); }

	// Unary operators
	const scr_rect operator +(const scr_coord& other ) const { scr_rect rect(x + other.x, y + other.y, w, h ); return rect; }
	const scr_rect operator -(const scr_coord& other ) const { scr_rect rect(x - other.x, y - other.y, w, h ); return rect; }

	// for now still accepted, will be removed
	// when we convert all relevant koord to scr_coord.
	scr_rect( const koord& pt ) { init( pt.x, pt.y, 0, 0 ); }
	scr_rect( const koord& pt, const koord& size ) { init( pt.x, pt.y, size.x, size.y ); }
	scr_rect( const koord& pt, scr_coord_val w, scr_coord_val h ) { init( pt.x, pt.y, w, h ); }

	// Validation functions
	bool is_empty(void) const { return (w|h) == 0;  }
	bool is_valid(void) const { return !is_empty(); }

	// Helper functions
	const scr_coord get_pos() const {
		return scr_coord( x, y );
	}

	void set_pos( const scr_coord& point ) {
		w = (x+w) - point.x;
		h = (y+h) - point.y;
		x = point.x;
		y = point.y;
	}

	const scr_coord get_bottomright() const {
		return scr_coord( x+w, y+h );
	}

	void set_bottomright( const scr_coord& point ) {
		w = point.x - x;
		h = point.y - y;
	}

	scr_coord_val get_width() const { return w; }

	scr_coord_val get_right() const { return x+w; }

	scr_coord_val get_height() const { return h; }

	scr_coord_val get_bottom() const { return y+h; }

	void set_right(scr_coord_val right) {
		w = right-x;
	}

	void set_bottom(scr_coord_val bottom) {
		h = bottom-y;
	}

	// now just working on two of the four coordinates
	void move_to( scr_coord_val x_par, scr_coord_val y_par ) {
		x = x_par;
		y = y_par;
	}

	void move_to( const scr_coord& point ) {
		move_to( point.x, point.y );
	}

	// and the other two with the size
	const scr_size get_size(void) const { return scr_size( w, h ); }

	const scr_size set_size( const scr_size &sz ) {
		w = sz.w;
		h = sz.h;
	}

	void set_size( scr_coord_val width, scr_coord_val height) {
		w = width;
		h = height;
	}

	// add to left, right and top, bottom
	void expand( scr_coord_val delta_x_par, scr_coord_val delta_y_par ) {
		x -= delta_x_par;
		y -= delta_y_par;
		w += (delta_x_par<<1);
		h += (delta_y_par<<1);
	}

	// Adjust rect to the bounding rect of this and rect.
	void outer_bounds( const scr_rect &rect ) {
		scr_coord pt( min(x,rect.x), min(y,rect.y) );
		w = max(x+w,rect.x+rect.w)-pt.x;
		h = max(y+h,rect.y+rect.h)-pt.y;
		x = pt.x;
		y = pt.y;
	}

	// Enforce a positive width and height
	void normalize() {
		if(  w<0  ) {
			x -= w;
			w = -w;
		}
		if(  h<0  ) {
			y -= h;
			h = -h;
		}
	}

	// point in rect; border (x+w) is still counting as contained
	bool contains( const scr_coord& pt ) const {
		return (  x <= pt.x  &&  x+w >= pt.x  &&  y <=pt.y  &&  y+h >= pt.y  );
	}

	// Now rect/rect functions: First check if rect is completely surrounded by this
	// maybe surrounds woudl be a better name, but contains seems more consitent
	bool contains( const scr_rect& rect ) const {
		return (  x <= rect.x  &&  x+w >= rect.x+rect.w  &&  y <= rect.y  &&  y+h >= rect.y+rect.h  );
	}

	bool is_overlapping( const scr_rect &rect ) const {
		return !( (get_bottomright().x < rect.x) ||
		(get_bottomright().y < rect.y) ||
		(rect.get_bottomright().x < x) ||
		(rect.get_bottomright().y < y) );
	}

	rect_relation_t relation( const scr_rect& rect ) {
		if( contains(rect) ) {
			return RECT_RELATION_INSIDE;
		}
		else if( is_overlapping( rect ) ) {
			return RECT_RELATION_OVERLAP;
		}
		return RECT_RELATION_OUTSIDE;
	}

	/* gets the overlapping area; in case of no overlap the area the size is negative
	 * imho this should rather return a nwe rect
	 */
	void reduce_to_overlap( const scr_rect rect ) {
		x = max(x, rect.x);
		set_right( min(x+w, rect.x+rect.w) );
		y = max(y, rect.y);
		set_bottom( min(y+h, rect.y+rect.h) );
	}

	bool operator ==(const scr_rect& rect) const {
		return ( (x-rect.x) | (y-rect.y) | (w-rect.w) | (h-rect.h) ) == 0;
	}

	bool operator !=(const scr_rect& rect_par) const {
		return !(rect_par==*this);
	}
};
#endif
