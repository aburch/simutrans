/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "rect.h"


rect_t::rect_t() :
	origin(0, 0),
	size(0, 0)
{
}


rect_t::rect_t(koord const origin, koord const size) :
	origin(origin),
	size(size)
{
}


rect_t::rect_t(koord const origin, sint16 const x, sint16 const y) :
	origin(origin),
	size(x, y)
{
}


size_t rect_t::fragment_difference(rect_t const &remove, rect_t *const result, size_t const result_length) const
{
	if (result_length < rect_t::MAX_FRAGMENT_DIFFERENCE_COUNT) {
		assert("rect_t::fragment_difference : attempting to fragment when space in result array is less than MAX_FRAGMENT_DIFFERENCE_COUNT");
	}

	rect_t internal_remove = remove;
	internal_remove.mask(*this);

	rect_t *fragment = result;

	if (internal_remove == *this) {
		// area subset of remove so nothing returned
	} else if(internal_remove.has_no_area()) {
		// no overlap so entire area unchanged
		*fragment = *this;
		fragment++;
	} else {
		// partial overlap so fragmentation occurs

		koord const internal_remove_extent = internal_remove.origin + internal_remove.size;
		koord const extent = origin + size;

		if (origin.y < internal_remove.origin.y) {
			// fragment at bottom of remove rect
			fragment->origin = origin;
			fragment->size.x = size.x;
			fragment->size.y = internal_remove.origin.y - origin.y;
			fragment++;
		}

		if (origin.x < internal_remove.origin.x) {
			// fragment at left of remove rect
			fragment->origin.x = origin.x;
			fragment->origin.y = internal_remove.origin.y;
			fragment->size.x = internal_remove.origin.x - origin.x;
			fragment->size.y = internal_remove.size.y;
			fragment++;
		}

		if (extent.x > internal_remove_extent.x) {
			// fragment at right of remove rect
			fragment->origin.x = internal_remove_extent.x;
			fragment->origin.y = internal_remove.origin.y;
			fragment->size.x = extent.x - internal_remove_extent.x;
			fragment->size.y = internal_remove.size.y;
			fragment++;
		}

		if (extent.y > internal_remove_extent.y) {
			// fragment at top of remove rect
			fragment->origin.x = origin.x;
			fragment->origin.y = internal_remove_extent.y;
			fragment->size.x = size.x;
			fragment->size.y = extent.y - internal_remove_extent.y;
			fragment++;
		}
	}

	return (size_t) (fragment - result);
}


void rect_t::mask(rect_t const &mask_rect)
{
	koord extent = origin + size;
	koord const mask_extent = mask_rect.origin + mask_rect.size;

	if (origin.x >= mask_extent.x || extent.x <= mask_rect.origin.x ||
		origin.y >= mask_extent.y || extent.y <= mask_rect.origin.y) {
		// completly masked
		origin = size = koord(0, 0);
	} else {
		// partially masked
		origin.clip_min(mask_rect.origin);
		extent.clip_max(mask_extent);
		size = extent - origin;
	}
}


bool rect_t::has_no_area() const
{
	return size == koord(0, 0);
}


void rect_t::discard_area()
{
	size = koord(0, 0);
}


bool rect_t::operator==(rect_t const &rhs) const
{
	return origin == rhs.origin && size == rhs.size;
}


bool rect_t::operator!=(rect_t const &rhs) const
{
	return !(*this == rhs);
}
