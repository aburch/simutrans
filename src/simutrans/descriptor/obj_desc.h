/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_OBJ_DESC_H
#define DESCRIPTOR_OBJ_DESC_H


#include <cstddef>
#include "../simtypes.h"

/**
 * Basis of all desc_t classes, which are loaded from the .pak files.
 * No virtual methods are allowed!
 */
class obj_desc_t
{
	friend class pakset_manager_t;

public:
	obj_desc_t() : children() {}

	~obj_desc_t() { delete [] children; }

	void* operator new(size_t size)
	{
		return ::operator new(size);
	}

	void* operator new(size_t size, size_t extra)
	{
		return ::operator new(size + extra);
	}

	/*
	* Only support basic delete operator.
	* Prevents C++14 and newer compilers from implicitly adding a sized delete operator.
	* The sized delete operator conflicts with the definiton of the placement new operator.
	*/
	void operator delete(void* ptr)
	{
		return ::operator delete(ptr);
	}

protected:
	template<typename T> T const* get_child(int const i) const { return static_cast<T const*>(children[i]); }

private:
	/*
	 * Internal Node information - the derived class knows,
	 * how many node child nodes really exist.
	 */
	obj_desc_t** children;

	friend class factory_field_group_reader_t;
	friend class obj_reader_t;
};

#endif
