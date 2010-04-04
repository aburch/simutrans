/*
 * Copyright (c) 2010 Bernd Gabriel
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#pragma once

#ifndef list_tpl_h
#define list_tpl_h

#include "../macros.h"
#include "../simtypes.h"
#include "../simdebug.h"

/**
 * 
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
template<class item_t> class list_tpl 
{
private:
	item_t **data;
	uint32 capacity;
	uint32 count;
	bool owns_items;

	void set_capacity(uint32 value) {
		assert(value <= (uint32)(UINT32_MAX_VALUE >> 1));
		if (capacity > value) {
			for (; capacity > value; ) {
				if (data[--capacity]) {
					delete_item(data[capacity]);
					data[capacity] = NULL;
				}
			}
			if (count > value)
			{
				count = value;
			}
		}
		else if (capacity < value) {
			data = (item_t**) realloc(data, sizeof(item_t*) * value);
			for (; capacity < value; capacity++) {
				data[capacity] = NULL;
			}
		}
	}

	void grow() {
		// resulting capacities: 16, 24, 36, 54, 81, 121, 181, 271, 406, 609, 913, ...
		set_capacity(capacity < 16 ? 16 : capacity + capacity / 2);
	}
	void qsort(int L, int R);
protected:
	virtual int compare_items(item_t *item1, item_t *item2) const {
		return (int) (item1 - item2);
	}
	virtual item_t *create_item() { 
		return NULL;
	}
	virtual void delete_item(item_t *item) { 
		if (item && owns_items) {
			delete [] item; 
		}
	}
public:
	explicit list_tpl() { data = NULL; count = capacity = 0; this->owns_items = false; }
	explicit list_tpl(uint32 capacity) { list_tpl(); set_capacity(capacity); }

	// administration

	bool get_owns_items() const { return owns_items; }
	void set_owns_items(bool value) { owns_items = value; }

	uint32 get_capacity() const { return capacity; }
	uint32 get_count() const { return count; }
	void resize(uint32 new_count);

	void move(uint32 from, uint32 to);
	void sort(); 

	// getting data

	sint32 index_of(item_t *item) const;
	item_t *get(uint32 index) const { return data[index]; }
	item_t *operator [] (uint32 index) const {	return data[index];	}

	// adding items

	item_t *set(uint32 index, item_t *item);
	sint32 add(item_t *item);
	void insert(uint32 index, item_t *item);

	// removing items

	item_t *extract(uint32 index);
	void remove(uint32 index) {	delete_item(extract(index)); }
	void remove(item_t *) {	delete_item(extract(index)); }
};


// BG, 04.04.2010
template<class item_t> item_t *list_tpl<item_t>::set(uint32 index, item_t *item) 
{
	//assert(index < count);
	item_t *old = data[index];
	data[count] = item;
	if (owns_items)
	{
		delete_item(old);
		old = NULL;
	}
	return old;
}


// BG, 04.04.2010
template<class item_t> sint32 list_tpl<item_t>::add(item_t *item) 
{
	if (count == capacity) {
		grow();
	}
	data[count] = item;
	return (sint32) count++;
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::insert(uint32 index, item_t *item) 
{
	assert(index <= count);
	if (count == capacity) {
		grow();
	}
	for (uint32 i = count++; i > index; i--) {
		data[i] = data[i-1];
	}
	data[index] = item;
}


// BG, 04.04.2010
template<class item_t> sint32 list_tpl<item_t>::index_of(item_t *item) const 
{
	sint32 index = (sint32) count; 
	while (--index >= 0){
		if (!compare_items(data[index], item)){
			break;
		}
	}
	return index;
}


// BG, 04.04.2010
template<class item_t> item_t *list_tpl<item_t>::extract(uint32 index) 
{
	assert(index < count);
	item_t *item = data[index];
	while (++index < count)
	{
		data[index-1] = data[index];
	}
	data[--count] = NULL;
	return item;
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::move(uint32 from, uint32 to) 
{
	assert(from < count);
	assert(to < count);
	item_t *item = data[from];
	if (from < to) {
		while (++from <= to)
		{
			data[from-1] = data[from];
		}			
	}
	else {
		while (--from >= to) {
			data[from+1] = data[from];
		}
	}
	data[to] = item;
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::qsort(int L, int R)
{
	int I, J, P;
	while(true)
	{
		I = L;
		J = R;
		P = (L + R) / 2;
		item_t *pivot = data[P];
		while(true)
		{
			while (compare_items(data[I], pivot) < 0) 
				I++;
			while (compare_items(data[J], pivot) > 0)
				J--;
			if (I <= J)
			{
			  item_t *T = data[I];
			  data[I++] = data[J];
			  data[J--] = T;
			};
			if (I > J)
				break;
		}
		if (L < J) 
			qsort(L, J);
		L = I;
		if (I >= R)
			break;
	}
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::resize(uint32 new_count) 
{
	set_capacity(new_count);
	while (count < new_count) {
		data[count++] = create_item();
	}
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::sort() 
{ 
	if (count > 1)
	{
		qsort(0, count - 1);
	}
}

#endif