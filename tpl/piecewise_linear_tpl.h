/**
 * Copyright 2013 Nathanael Nerode
 *
 * This file is licensed under the Artistic License as part of the Simutrans project. (See license.txt)
 * This file is also licensed under the Artistic License 2.0 and the GNU General Public License 3.0.
 *
 * Implements a piecewise linear function
 *   value = f(key)
 * Value and key must both be numbers, roughly speaking.
 *
 * Specifically, enough operators must be defined so that
 *  value1 + (value2 - value1) * (key3 - key1) / (key2 - key1)
 * will give a result of type value. (The operations will be done in this specific order.)
 * Also it must be possible to compare keys with "<".
 *
 * This is intended to be used with simple POD types and may be very inefficient for non-POD types.
 *
 * Piecewise linear functions work by specifying output values
 * at a number of specified "key" points, and interpolating linearly for keys
 * between the key points.
 *
 * In this implementation, f is constant for keys less than the smallest specified key,
 * and constant for keys greater than the largest specified key.
 *
 * The data is stored internally as an array of pairs, sorted by key:
 * key0, value0
 * key1, value1
 * key2, value2
 * ...
 *
 * The table starts out empty and the pairs must be entered one at a time.  Duplicate keys are
 * prohibited.
 * The keys must be sorted from smallest (in entry 0) to largest; they are automatically entered
 * in the right place by insert_unique.
 *
 * This is designed to be built once, not modified, hence the lack of "remove" functions.
 * It is used to implement various economic game logic functions configured by paks.
 * As such it is instantiated with small, constant numbers of elements; although these are not
 * known at compile time, they are known early in runtime during settings loading.
 *
 * @author neroden (Nathanael Nerode)
 */

#ifndef TPL_PIECEWISE_LINEAR_TPL_H
#define TPL_PIECEWISE_LINEAR_TPL_H


// Integer types
#include "../simtypes.h"
// Data structure is from vector_tpl
#include "vector_tpl.h"

// This is a pretty complicated construction.
// The third argument is the type to use for internal computations.
template < class key_t, class value_t, class internal_t = value_t > class piecewise_linear_tpl
{
public:
	// Access to keys and values
	struct data_pair {
		key_t key;
		value_t value;
	};

protected:
	// Data: vector...
	// which also handles the memory managment, so we don't have to!
	// Direct containment: avoid adding pointer indirection
	// This is the only data in the class
	vector_tpl< data_pair > vec;

public:
	/*
	 * Constructor.  Data starts out blank.
	 */
	piecewise_linear_tpl() : vec( 0 )
	{
	}
	/*
	 * Explicit constructor: specify size in advance.
	 */
	explicit piecewise_linear_tpl(uint32 my_size) : vec(my_size)
	{
	}
	/*
	 * Clear the table, don't resize it.
	 */
	void clear() {
		vec.clear();
	}
	/*
	 * Clear the table, resize to new size.
	 */
	void clear(uint32 my_size) {
		clear();
		vec.resize(my_size);
	}
	/*
	 * We do not define the "big three"
	 * (copy constructor, assignment operator, destructor)
	 * as we expect vector_tpl to take care of the
	 * data handling.  So the defaults will work for us.
	 *
	 * This is incidentally why we can't use minivec_tpl, which doesn't implement them.
	 * If it turns out we don't need copy construction or copy assignment,
	 * we can switch over to minivec_tpl and refuse to implement them.
	 */

private:
	// Binary search on the keys.  Gets the index of the key next greater than (or equal to) the target key.
	// In use, these tables have between 2 and 7 entries, so this may seem like overkill.
	// But the list must be sorted already, so why not?  It is just as fast as a for loop.
	uint32 binary_search(key_t target) const {
		uint32 low = 0; // bottom element
		uint32 high = vec.get_count(); // top element + 1
		while (low < high)
		{
			// Note: this could cause overflow if used with large counts (>127).
			// This is not expected to happen.
			uint32 i = (low + high) / 2;
			if ( vec[i].key < target ) {
				low = i + 1;
			}
			else {
				high = i;
			}
		}
		return high;
	}

public:
	/*
	 * Insert a key-value pair.
	 * This is the only way of putting data into the table,
	 * other than copy-constructing or copy-assigning.
	 *
	 * If a key is specified which is already present it will be *replaced*.
	 */
	void insert(key_t target_key, value_t target_value) {
		uint32 right_idx = binary_search(target_key);
		if (right_idx < vec.get_count() && vec[right_idx].key == target_key) {
			// Replace a key
			vec[right_idx].value = target_value;
		}
		else {
			// insert key-value pair before right_idx
			// underlying vector appends when necessary
			data_pair tmp;
			tmp.key = target_key;
			tmp.value = target_value;
			vec.insert_at(right_idx, tmp);
		}
	}

	/**
	 * Compute the linear interpolation.
	 */
	value_t compute_linear_interpolation(key_t target) const {
		// This is nonsense if there are no entries
		assert(vec.get_count() > 0);
		if (vec.get_count() == 1) {
			// If there's only one entry, avoid the comparison code;
			// the regular algorithm will work but it's a complicated computation.
			// And we have to get the count for the binary search anyway.
			// This degenerate case is likely to happen for paksets which
			// choose not to implement some of the fancy options, so make it fast.
			return vec[0].value;
		}
		// Find the entry equal to or next larger than the target.
		const uint32 right = binary_search(target);
		if ( right == 0 ) {
			// Target was less than or equal to the smallest entry.  Use constant.
			return vec[0].value;
		}
		else if ( right == vec.get_count() ) {
			// Target was larger than the largest entry.  Use constant.
			return vec[right - 1].value;
		}
		else if (vec[right].key == target) {
			// This is a shortcut for speed; the general case would work.
			return vec[right].value;
		}
		else {
			// Main case: linear interpolation
			const uint32 left = right - 1; // makes following code clearer; will be optimized out
			// We may want to use a wider type for this computation, hence the ability to specify this with the third parameter which becomes internal_t
			internal_t right_val = vec[right].value;
			internal_t left_val  = vec[left].value;
			internal_t numerator = (right_val - left_val) * (target - vec[left].key);
			internal_t result = left_val + numerator / (vec[right].key - vec[left].key);
			return (value_t) result;
			// The above return statement relies on operator precedence!
			// Note that the math will be done in the type of *value*,
			// with the exception of the differences in keys
			// This means that the value type should be wide enough for computation..
		}
	}
	/**
	 * This class is a "functor" or "functional".
	 * Implement function call syntax.
	 */
	value_t operator () (key_t key) const {
		return compute_linear_interpolation(key);
	}
};

#endif
