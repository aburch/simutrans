/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef UTILS_FOR_H
#define UTILS_FOR_H


#include <iterator>

/* FOR(type, elem, container)
 * FORX(type, elem, container, step)
 *
 * Iterate over container, which has type type, and optionally do step on every iteration.
 * The current element is put into elem.
 * elem can be
 * - a plain variable name (x), which makes a copy of the element
 * - a const copy (const x)
 * - a reference (& x) or
 * - a const reference (const& x).
 *
 * This simplifies the usual pattern
 *   for (vector<int>::iterator i = container.begin(), end = container.end(); i != end; ++i) {
 *     int& elem = *i;
 *     ...
 *   }
 * to
 *   FOR(vector<int>, & elem, container) {
 *     ...
 *   }
 *
 * FORT()/FORTX() need to be used, if type is a dependent name.
 */

template<typename T, typename SEL> struct for_sel_ref   { typedef T const& ref; };
template<typename T> struct for_sel_ref<T, void (int&)> { typedef T&       ref; };

template<typename T, typename SEL> struct for_sel_iter   { typedef typename T::const_iterator iter; };
template<typename T> struct for_sel_iter<T, void (int&)> { typedef typename T::iterator       iter; };

#define LNAME__(name, line) name##__##line
#define LNAME_(name, line)  LNAME__(name, line)
#define LNAME(name)         LNAME_(name, __LINE__)

#define FORX_(typename, type, elem, container, step) \
	if (bool LNAME(once0) = false) {} else \
		for (typename for_sel_ref<type, void(int elem)>::ref LNAME(container_) = (container); !LNAME(once0); LNAME(once0) = true) \
			if (bool LNAME(break) = false) {} else \
				for (typename for_sel_iter<type, void(int elem)>::iter LNAME(iter) = LNAME(container_).begin(), LNAME(end) = LNAME(container_).end(); !LNAME(break) && LNAME(iter) != LNAME(end); LNAME(break) ? (void)0 : (void)((step), ++LNAME(iter))) \
					if (bool LNAME(once1) = (LNAME(break) = true, false)) {} else \
						for (typename std::iterator_traits<typename for_sel_iter<type, void(int elem)>::iter>::value_type elem = *LNAME(iter); !LNAME(once1); LNAME(break) = false, LNAME(once1) = true)

#define FORX(type, elem, container, step) FORX_(, type, elem, container, (step))
#define FOR(type, elem, container)        FORX(type, elem, container, (void)0)

#define FORTX(type, elem, container, step) FORX_(typename , type, elem, container, (step))
#define FORT(type, elem, container)        FORTX(type, elem, container, (void)0)

#endif
