 #ifndef scr_coord_h
#define scr_coord_h

#include <assert.h>
#include "../dataobj/koord.h"
#include "../simtypes.h"

// Screen coordinate type
typedef sint16 scr_coord_val;

// Two dimensional point type
class scr_coord
{
public:
	scr_coord_val x;
	scr_coord_val y;

	scr_coord() { x=0; y=0; }

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


// Rectangle type
class scr_rect
{
private:
	enum { INVALID=-32109 };

	scr_coord_val x;
	scr_coord_val y;
	scr_coord_val w;
	scr_coord_val h;

	bool is_valid() {
		return x != INVALID;
	}

	// always enforce a positive width and height
	void normalize() {
		assert( is_valid() );
		if(  w<0  ) {
			x -= w;
			w = -w;
		}
		if(  h<0  ) {
			y -= h;
			h = -h;
		}
	}

	void init( scr_coord_val x_par, scr_coord_val y_par, scr_coord_val w_par, scr_coord_val h_par ) {
		x = x_par;
		y = y_par;
		w = w_par;
		h = h_par;
		normalize();
	}

public:
	scr_rect() { x = INVALID; y = w = h = 0; }

	scr_rect( const scr_coord& pt ) { init( pt.x, pt.y, 0, 0 ); }

	scr_rect( const scr_coord& pt, scr_coord_val w, scr_coord_val h ) { init( pt.x, pt.y, w, h ); }

	// for now still accepted ...
	scr_rect( const koord& pt ) { init( pt.x, pt.y, 0, 0 ); }
	scr_rect( const koord& pt, scr_coord_val w, scr_coord_val h ) { init( pt.x, pt.y, w, h ); }

	scr_rect(const scr_coord& point1, const scr_coord& point2 ) {
		init( point1.x, point1.y, point2.x-point1.x, point2.y-point1.y );
	}

	const scr_coord get_pos() const {
		return scr_coord( x, y );
	}

	const scr_coord get_topleft() const {
		return scr_coord( x, y );
	}

	void set_topleft( const scr_coord& point ) {
		w = (x+w) - point.x;
		h = (y+h) - point.y;
		x = point.x;
		y = point.y;
		normalize();
	}

	const scr_coord get_bottomright() const {
		return scr_coord( x+w, y+h );
	}

	void set_bottomright( const scr_coord& point ) {
		w = point.x - x;
		h = point.y - y;
		normalize();
	}

	scr_coord_val get_width() const { return w; }

	void set_width(scr_coord_val width) {
		w = width;
		normalize();
	}

	scr_coord_val get_height() const { return h; }

	void set_height(scr_coord_val height) {
		h = height;
		normalize();
	}

	const scr_coord get_size(void) const { return scr_coord( w, h ); }

	void set_size( scr_coord_val width, scr_coord_val height) {
		w = width;
		h = height;
		normalize();
	}

	bool operator ==(const scr_rect& rect) const {
		return ( (x-rect.x) | (y-rect.y) | (w-rect.w) | (h-rect.h) ) == 0;
	}

	bool operator !=(const scr_rect& rect_par) const {
		return !(rect_par==*this);
	}

	bool is_empty() const { return (w|h) == 0; }

	// point in rect; border (x+w) is still counting as contained
	bool contains( const scr_coord& pt ) const {
		return (  x <= pt.x  &&  x+w >= pt.x  &&  y <=pt.y  &&  y+h >= pt.y  );
	}

	bool contains( const scr_rect& rect ) const {
		return (  x <= rect.x  &&  x+w >= rect.x+rect.w  &&  y <= rect.y  &&  y+h >= rect.y+rect.h  );
	}

	bool is_overlapping( const scr_rect &rect ) const {
		return !( (get_bottomright().x < rect.get_topleft().x) ||
		(get_bottomright().y < rect.get_topleft().y) ||
		(rect.get_bottomright().x < x) ||
		(rect.get_bottomright().y < y) );
	}

	// will contain the
	void merge( const scr_rect &rect ) {
		if(  !is_valid()  ) {
			*this = rect;
		}
		else {
			scr_coord pt( min(x,rect.x), min(y,rect.y) );
			w = max(x+w,rect.x+rect.w)-pt.x;
			h = max(y+h,rect.y+rect.h)-pt.y;
			x = pt.x;
			y = pt.y;
		}
	}

	void move( scr_coord_val x_par, scr_coord_val y_par ) {
		x = x_par;
		y = y_par;
	}

	void move( const scr_coord& point ) {
		move( point.x, point.y );
	}

	void expand( scr_coord_val delta_x_par, scr_coord_val delta_y_par ) {
		w += delta_x_par;
		h += delta_y_par;
		normalize();
	}

};
#endif
