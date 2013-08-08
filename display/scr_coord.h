#ifndef scr_coord_h
#define scr_coord_h

#include "../simtypes.h"

// Screen coordinate type
typedef sint16 scr_coord_val;

// Two dimensional point type
typedef struct tag_scr_coord_t {

	scr_coord_val x;
	scr_coord_val y;

	tag_scr_coord_t() {
		tag_scr_coord_t(0,0);
	}

	tag_scr_coord_t(scr_coord_val x_par, scr_coord_val y_par) {
		x = x_par;
		y = y_par;
	}

/*	tag_scr_coord_t(koord pos_par) {
		tag_scr_coord_t(pos_par.x,pos_par.y);
	}
*/
	bool operator ==(const tag_scr_coord_t& point_par) const {
		return (x == point_par.x) && (y == point_par.y);
	}

	bool operator !=(const tag_scr_coord_t& point_par) const {
		return !(point_par == *this);
	}

	tag_scr_coord_t operator +(const tag_scr_coord_t& point_par) const {
		return tag_scr_coord_t(point_par.x + this->x, point_par.y + this->y);
	}

	tag_scr_coord_t operator -(const tag_scr_coord_t& point_par) const {
		return tag_scr_coord_t(this->x - point_par.x, this->y - point_par.y );
	}

	tag_scr_coord_t& operator +=(const tag_scr_coord_t& point_par) {
		this->x += point_par.x;
		this->y += point_par.y;
		return *this;
	}

	tag_scr_coord_t& operator -=(const tag_scr_coord_t& point_par) {
		this->x -= point_par.x;
		this->y -= point_par.y;
		return *this;
	}

	void add_offset(scr_coord_val delta_x, scr_coord_val delta_y) {
		x += delta_x;
		y += delta_y;
	}

	void set(scr_coord_val x_par, scr_coord_val y_par) {
		x = x_par;
		y = y_par;
	}

	void set(const tag_scr_coord_t& point_par) {
		x = point_par.x;
		y = point_par.y;
	}

} scr_coord;

// Rectangle type
typedef struct tag_scr_rect_t {

	scr_coord_val left;
	scr_coord_val top;
	scr_coord_val right;
	scr_coord_val bottom;

	tag_scr_rect_t() {
		init(0,0,0,0);
	}

	tag_scr_rect_t(const scr_coord& point_par) {
		init(point_par.x, point_par.y, point_par.x, point_par.y);
	}

	tag_scr_rect_t(const scr_coord& point_par, scr_coord_val width_par, scr_coord_val height_par) {
		init (point_par.x, point_par.y, point_par.x + width_par, point_par.y + height_par);
	}

	tag_scr_rect_t(scr_coord_val x_par, scr_coord_val y_par, scr_coord_val xx_par, scr_coord_val yy_par) {
		init(x_par, y_par, xx_par, yy_par);
	}

	tag_scr_rect_t(const scr_coord& point1_par, const scr_coord& point2_par) {
		init(point1_par.x, point1_par.y, point2_par.x, point2_par.y);
	}

	void init(scr_coord_val x_par, scr_coord_val y_par, scr_coord_val xx_par, scr_coord_val yy_par) {
		left = x_par;
		top = y_par;
		right = xx_par;
		bottom = yy_par;
		normalize();
	}

	scr_coord& top_left() {
		return *((scr_coord*)this);
	}

	scr_coord& bottom_right() {
		return *((scr_coord*)this+1);
	}

	const scr_coord& top_left() const {
		return *((scr_coord const *)this);
	}

	const scr_coord& bottom_right() const {
		return *((scr_coord const *)this+1);
	}

	scr_coord_val get_width() const {
		return right - left;
	}

	void set_width(scr_coord_val width) {
		right = left + width;
	}

	scr_coord_val get_height() const {
		return bottom - top;
	}

	void set_height(scr_coord_val height) {
		bottom = top + height;
	}

	scr_coord get_size(void) {
		return scr_coord(get_width(),get_height());
	}

	void set_size(scr_coord_val width, scr_coord_val height) {
		set_width(width);
		set_height(height);
	}

	void set_size(scr_coord size) {
		set_size(size.x,size.y);
	}

	void set_size(tag_scr_rect_t rect) {
		set_size(rect.get_size());
	}

	scr_coord get_pos() const {
		return scr_coord(left, top);
	}

	void normalize() {
		if (top > bottom) {
			top    = top ^ bottom;
			bottom = top ^ bottom;
			top    = top ^ bottom;
		}
		if (left > right) {
			left  = left ^ right;
			right = left ^ right;
			left  = left ^ right;
		}
	}

	bool operator ==(const tag_scr_rect_t& rect_par) const {
		return ( (left == rect_par.left)  &&  (top == rect_par.top)  &&  (right == rect_par.right)  &&  (bottom == rect_par.bottom) );
	}

	bool operator !=(const tag_scr_rect_t& rect_par) const {
		return !(rect_par==*this);
	}

	bool is_empty() const {
		return ( (right == left) || (bottom == top) );
	}

	bool contains(const scr_coord& point_par) const {
		return ( (point_par.x >= left)  &&  (point_par.y >= top)  &&  (point_par.x < right)  &&  (point_par.y < bottom) );
	}

	bool contains(const tag_scr_rect_t& rect_par) const {
		return contains(rect_par.top_left()) && contains(rect_par.bottom_right());
	}

	bool is_overlapping(const tag_scr_rect_t &rect_par) const {
		return !( (bottom_right().x < rect_par.top_left().x) ||
		(bottom_right().y < rect_par.top_left().y) ||
		(rect_par.bottom_right().x < top_left().x) ||
		(rect_par.bottom_right().y < top_left().y) );
	}

	void merge(const tag_scr_rect_t &rect_par) {
		left = min(left,rect_par.left);
		right = max(right,rect_par.right);
		top = min(top,rect_par.top);
		bottom = max(bottom,rect_par.bottom);
	}

	void add_offset(scr_coord_val delta_x_par, scr_coord_val delta_y_par) {
		left   += delta_x_par;
		right  += delta_x_par;
		top    += delta_y_par;
		bottom += delta_y_par;
	}

	void move_to(scr_coord_val coord_x_par, scr_coord_val coord_y_par) {
		add_offset(coord_x_par - left, coord_y_par - top);
	}

	void move_to(const scr_coord& point_par) {
		add_offset(point_par.x - left, point_par.y - top);
	}

	void expand(scr_coord_val delta_x_par, scr_coord_val delta_y_par) {
		left   -= delta_x_par;
		right  += delta_x_par;
		top    -= delta_y_par;
		bottom += delta_y_par;
	}

	void expand(scr_coord_val left_par, scr_coord_val top_par, scr_coord_val right_par, scr_coord_val bottom_par) {
		left   -= left_par;
		right  += right_par;
		top    -= top_par;
		bottom += bottom_par;
	}

} scr_rect;
#endif
