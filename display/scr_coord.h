/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_SCR_COORD_H
#define DISPLAY_SCR_COORD_H


#include <assert.h>
#include "../dataobj/loadsave.h"
#include "../simtypes.h"

class koord;

// Screen coordinate type
typedef sint32 scr_coord_val;

// Rectangle relations
enum rect_relation_t {
	RECT_RELATION_INSIDE,
	RECT_RELATION_OVERLAP,
	RECT_RELATION_OUTSIDE
};

// Two dimensional coordinate type
class scr_coord
{
public:
	scr_coord_val x;
	scr_coord_val y;

	// Constructors
	scr_coord(  ) { x = y = 0; }
	scr_coord( scr_coord_val x_, scr_coord_val y_ ) { x = x_; y=y_; }

	bool operator ==(const scr_coord& other) const { return ((x-other.x) | (y-other.y)) == 0; }
	bool operator !=(const scr_coord& other) const { return !(other == *this ); }
	const scr_coord operator +(const scr_coord& other ) const { return scr_coord( other.x + x, other.y + y); }
	const scr_coord operator -(const scr_coord& other ) const { return scr_coord( x - other.x, y - other.y ); }

	void rdwr(loadsave_t *file)
	{
		xml_tag_t k( file, "koord" );
		file->rdwr_long(x);
		file->rdwr_long(y);
	}

	const scr_coord& operator +=(const scr_coord& other ) {
		x += other.x;
		y += other.y;
		return *this;
	}

	const scr_coord& operator -=(const scr_coord& other ) {
		x -= other.x;
		y -= other.y;
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

	inline void clip_lefttop( scr_coord scr_lefttop )
	{
		if (x < scr_lefttop.x) {
			x = scr_lefttop.x;
		}
		if (y < scr_lefttop.y) {
			y = scr_lefttop.y;
		}
	}

	inline void clip_rightbottom( scr_coord scr_rightbottom )
	{
		if (x > scr_rightbottom.x) {
			x = scr_rightbottom.x;
		}
		if (y > scr_rightbottom.y) {
			y = scr_rightbottom.y;
		}
	}

	static const scr_coord invalid;

private:
	// conversions to/from koord not allowed anymore
	scr_coord( const koord);
	operator koord() const;
};


static inline scr_coord operator * (const scr_coord &k, const sint16 m)
{
	return scr_coord(k.x * m, k.y * m);
}


static inline scr_coord operator / (const scr_coord &k, const sint16 m)
{
	return scr_coord(k.x / m, k.y / m);
}


// Very simple scr_size struct.
class scr_size
{
public:
	scr_coord_val w;
	scr_coord_val h;

public:
	// Constructors
	scr_size(  ) { w = h = 0; }
	scr_size( scr_coord_val w_par, scr_coord_val h_par) { w = w_par; h = h_par; }

	operator scr_coord() const { return scr_coord(w,h); }

	bool operator ==(const scr_size& other) const { return ((w-other.w) | (h-other.h)) == 0; }
	bool operator !=(const scr_size& other) const { return !(other == *this ); }
	const scr_size operator +(const scr_size& other ) const { return scr_size( other.w + w, other.h + h); }
	const scr_size operator -(const scr_size& other ) const { return scr_size( w - other.w, h - other.h ); }
	const scr_size operator +(const scr_coord& other ) const { return scr_size( other.x + w, other.y + h); }
	const scr_size operator -(const scr_coord& other ) const { return scr_size( w - other.x, h - other.y ); }

	void rdwr(loadsave_t *file)
	{
		xml_tag_t k( file, "koord" );
		file->rdwr_long(w);
		file->rdwr_long(h);
	}

	inline void clip_lefttop( scr_coord scr_lefttop )
	{
		if (w < scr_lefttop.x) {
			w = scr_lefttop.x;
		}
		if (h < scr_lefttop.y) {
			h = scr_lefttop.y;
		}
	}

	inline void clip_rightbottom( scr_coord scr_rightbottom )
	{
		if (w > scr_rightbottom.x) {
			w = scr_rightbottom.x;
		}
		if (h > scr_rightbottom.y) {
			h = scr_rightbottom.y;
		}
	}

	const scr_size& operator +=(const scr_size& other ) {
		w += other.w;
		h += other.h;
		return *this;
	}

	const scr_size& operator -=(const scr_size& other ) {
		w -= other.w;
		h -= other.h;
		return *this;
	}

	const scr_size& operator +=(const scr_coord& other ) {
		w += other.x;
		h += other.y;
		return *this;
	}

	const scr_size& operator -=(const scr_coord& other ) {
		w -= other.x;
		h -= other.y;
		return *this;
	}

	static const scr_size invalid;

	static const scr_size inf;
private:
	// conversions to/from koord not allowed anymore
	operator koord() const;
};


// Rectangle type
class scr_rect
{
public:
	scr_coord_val x;
	scr_coord_val y;
	scr_coord_val w;
	scr_coord_val h;

	// to set it in one line ...
	void set( scr_coord_val x_par, scr_coord_val y_par, scr_coord_val w_par, scr_coord_val h_par ) {
		x = x_par;
		y = y_par;
		w = w_par;
		h = h_par;
	}

	// Constructors
	scr_rect() { set(0,0,0,0); }
	scr_rect( const scr_coord& pt ) { set( pt.x, pt.y, 0, 0 ); }
	scr_rect( const scr_coord& pt, scr_coord_val w, scr_coord_val h ) { set( pt.x, pt.y, w, h ); }
	scr_rect( const scr_coord& pt, const scr_size& sz ) { set( pt.x, pt.y, sz.w, sz.h ); }
	scr_rect( scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h ) { set( x, y, w, h ); }
	scr_rect( scr_size size ) { w = size.w; h=size.h; x=0; y=0; }
	scr_rect( const scr_coord& point1, const scr_coord& point2 ) { set( point1.x, point1.y, point2.x-point1.x, point2.y-point1.y ); }

	// Type cast operators
	operator scr_size()  const { return scr_size (w,h); }
	operator scr_coord() const { return scr_coord(x,y); }

	// Unary operators
	const scr_rect operator +(const scr_coord& other ) const { scr_rect rect(x + other.x, y + other.y, w, h ); return rect; }
	const scr_rect operator -(const scr_coord& other ) const { scr_rect rect(x - other.x, y - other.y, w, h ); return rect; }

	const scr_rect operator +(const scr_size& sz ) const { return scr_rect(x,y, w+sz.w, h+sz.h ); }
	const scr_rect operator -(const scr_size& sz ) const { return scr_rect(x,y, w-sz.w, h-sz.h ); }

	// Validation functions
	bool is_empty() const { return (w|h) == 0;  }
	bool is_valid() const { return !is_empty(); }

	// Helper functions
	scr_coord get_pos() const {
		return scr_coord( x, y );
	}

	void set_pos( const scr_coord point ) {
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
	const scr_size get_size() const { return scr_size( w, h ); }

	void set_size( const scr_size &sz ) {
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
	// maybe surrounds would be a better name, but contains seems more consistent
	bool contains( const scr_rect& rect ) const {
		return (  x <= rect.x  &&  x+w >= rect.x+rect.w  &&  y <= rect.y  &&  y+h >= rect.y+rect.h  );
	}

	bool is_overlapping( const scr_rect &rect ) const {
		return !( (get_bottomright().x < rect.x) ||
		(get_bottomright().y < rect.y) ||
		(rect.get_bottomright().x < x) ||
		(rect.get_bottomright().y < y) );
	}

	rect_relation_t relation( const scr_rect& rect ) const {
		if( contains(rect) ) {
			return RECT_RELATION_INSIDE;
		}
		else if( is_overlapping( rect ) ) {
			return RECT_RELATION_OVERLAP;
		}
		return RECT_RELATION_OUTSIDE;
	}

	/**
	 * reduces the current rect to the intersection area of two rects
	 * in case of no overlap the new size is negative
	 * @note maybe this could rather return a new rect
	 */
	void clip( const scr_rect clip_rect ) {
		x = max(x, clip_rect.x);
		set_right( min(x+w, clip_rect.x+clip_rect.w) );
		y = max(y, clip_rect.y);
		set_bottom( min(y+h, clip_rect.y+clip_rect.h) );
	}

	bool operator ==(const scr_rect& rect) const {
		return ( (x-rect.x) | (y-rect.y) | (w-rect.w) | (h-rect.h) ) == 0;
	}

	bool operator !=(const scr_rect& rect_par) const {
		return !(rect_par==*this);
	}
private:
	// conversions to/from koord not allowed anymore
	scr_rect( const koord&);
	operator koord() const;
};
#endif
