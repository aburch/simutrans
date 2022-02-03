/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_RECT_H
#define DATAOBJ_RECT_H


#include "koord.h"
#include <stddef.h>


class rect_t {
public:
	/**
	 * @brief Map index is prepared value.
	 */
	static size_t const MAX_FRAGMENT_DIFFERENCE_COUNT = 4;

	/**
	 * @brief Origin coordinates of this rect.
	 *
	 * The position of the lower left corner of this rect.
	 */
	koord origin;

	/**
	 * @brief Size of dimensions of this rect.
	 *
	 * The size of the dimensions of this rect. This is the number of elements
	 * covered by each dimension of this rect starting from the origin. Must
	 * only ever be greater than or equal to 0.
	 */
	koord size;

public:
	rect_t();

	rect_t(koord const origin, koord const size);

	rect_t(koord const origin, sint16 const x, sint16 const y);

	/**
	 * @brief Fragment this rect so as to remove area from another rect.
	 *
	 * Fragments this rect such that the resulting rects cover all area
	 * contained by this rect that is not contained in the remove rect. The
	 * returned value is the number of valid rect elements in the result array
	 * that make up the requested area. Result length specifies the space
	 * available in the result array to store fragmented rects and must be at
	 * least MAX_FRAGMENT_DIFFERENCE_COUNT long. The fragmentation is baised
	 * towards wide rects.
	 */
	size_t fragment_difference(rect_t const &remove, rect_t *const result, size_t const result_length) const;

	/**
	 * @brief Mask part of this rect using a masking rect.
	 *
	 * Transforms this rect into the rect containing the intersection area
	 * between this rect and the mask rect. Useful to clamp rects to map bounds.
	 */
	void mask(rect_t const &mask_rect);

	/**
	 * @brief Returns true if this rect represents no area.
	 *
	 * Returns true of the size of area is 0. Such rects can occur after masking
	 * operations or as place holder rects representing no map area.
	 */
	bool has_no_area() const;

	/**
	 * @brief discard the current area of the rect.
	 *
	 * Set the size of this rect to 0. Useful to invalidate a rect. The origin
	 * is not changed.
	 */
	void discard_area();

	bool operator==(rect_t const &rhs) const;
	bool operator!=(rect_t const &rhs) const;
};

#endif
