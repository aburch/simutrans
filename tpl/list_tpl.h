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
#include "../simmem.h"
#include "../simdebug.h"

/**
 * template<class item_t> list_tpl is a template for lists of polymorphic objects. 
 *
 * It hold pointers to objects of the common base class item_t and any classes derived from item_t.
 * It may be the owner of these objects, which causes, that the objects are deleted, when they are removed from the list.
 *
 * @since 22-MAR-2010
 * @author Bernd Gabriel
 */
template<class item_t> class list_tpl {

// data

private:
	item_t **data;
	uint32 capacity;
	uint32 count;
	bool owns_items : 1;
	bool is_sorted : 1;

// methods

private:
	void set_capacity(uint32 value);
	void grow() {
		// resulting capacities: 16, 24, 36, 54, 81, 121, 181, 271, 406, 609, 913, ...
		set_capacity(capacity < 16 ? 16 : capacity + capacity / 2);
	}
	void qsort(sint32 l, sint32 r);
	uint32 bsearch(const item_t *item) const;
	/**
	 * is_item_in_sort_order() returns true, if list is sorted and item is >= item at index - 1 and <= item at index + 1
	 */
	bool is_item_in_sort_order(uint32 index, const item_t *item);
protected:
	/**
	 * compare_items() is used for sorting.
	 *
	 * returns:
	 *	< 0: item1 is less than item2
	 *	= 0: item1 equals item2
	 *	> 0: item1 is greater than item2
	 */
	virtual int compare_items(const item_t *item1, const item_t *item2) const {
		return (int) (item1 - item2);
	}
	/**
	 * create_item() creates a new item, if list owns items.
	 *
	 * Descendant must override this method, as the template itself
	 * may run into compile errors, if item_t is an abstract class.
	 *
	 * create_item() is used by set_count(). 
	 * If you plan to use this template for a class that owns its items 
	 * and to use set_count(), you might want to override this method.
	 */
	virtual item_t *create_item() const { 
		return NULL;
	}
	/**
	 * delete_item() deletes the given item, if list owns items.
	 */
	virtual void delete_item(item_t *item) { 
		if (item && owns_items) {
			delete [] item; 
		}
	}
public:

// construction / destruction

	explicit list_tpl() : data(NULL), count(0), capacity(0), owns_items(false) {} 
	explicit list_tpl(bool owns_its_items) : data(NULL), count(0), capacity(0), owns_items(owns_its_items) {} 
	explicit list_tpl(bool owns_its_items, uint32 initial_capacity) : data(NULL), count(0), capacity(0), owns_items(owns_its_items) { set_capacity(initial_capacity); }
	explicit list_tpl(uint32 initial_capacity) : data(NULL), count(0), capacity(0), owns_items(false) { set_capacity(initial_capacity); }
	~list_tpl() { set_capacity(0); }

// administration

	/**
	 * If list owns its items, it automatically creates or deletes items, 
	 * whenever the size of the list changes or an item is replaced by another one.
	 */
	bool get_owns_items() const { return owns_items; }
	void set_owns_items(bool value) { owns_items = value; }

	bool get_is_sorted() const { return is_sorted; }
	void set_is_sorted(bool value) { is_sorted = value; }

	/**
	 * get_count() gets the number of items in the list.
	 */
	uint32 get_count() const { return count; }

	/**
	 * set_count() sets the number of items in the list.
	 *
	 * if count decreases, delete_item() is called for each item, that must leave the list.
	 * if count increases, create_item() is called for each added position in the list.
	 */
	void set_count(uint32 new_count);

	/**
	 * move() moves one item from index 'from' to index 'to'.
	 *
	 * All items between from and to are moved up/down one position accordingly.
	 */
	void move(uint32 from, uint32 to);

	/**
	 * swap() swaps the two items at index 'from' and index 'to'.
	 */
	void swap(uint32 from, uint32 to);

	/**
	 * sort() sorts the list using compare_items().
	 *
	 * Uses qsort(), a stack friendly implementation of the quicksort algorithm.
	 */
	void sort(); 

// querying data

	/**
	 * index_of() gets the position of the given item in the list. 
	 * Items are compared by their pointers.
	 *
	 * returns the position of the item or -1, if it is not in the list.
	 */
	sint32 index_of(const item_t *item) const;

	/**
	 * In a sorted list search() finds the position of an item like the given one
	 * or, if there is no such item in the list, the position, where to insert it.
	 * Items are compared by compare_item(). 
	 *
	 * Uses bsearch(), a fast binary search method, that works on sorted data.
	 */
	uint32 search(const item_t *item) const { return bsearch(item); }

// getting items

	/**
	 * get() gets the item at position 'index'.
	 */
	item_t *get(uint32 index) const { return data[index]; }

	/**
	 * operator[] gets the item at position 'index'.
	 */
	item_t *operator [] (uint32 index) const {	return data[index];	}

// putting items

	/**
	 * append() adds the given item at the end of the list.
	 * If the list owns its items, it owns the added item too now. 
	 *
	 * returns items's new position in the list.
	 */
	uint32 append(item_t *item);

	/**
	 * insert() inserts the given item at the given position into the list.
	 * If the list owns its items, it owns the inserted item too now. 
	 *
	 * All items at and above this position are moved up one position.
	 */
	void insert(uint32 index, item_t *item);

	/**
	 * add() inserts or appends the given item depending of list being sorted or not.
	 * Appends to unsorted list or inserts at appropriate position into sorted list.
	 */
	uint32 add(item_t *item);


	/**
	 * set() replaces the item at position 'index' with the given item.
	 * If the list owns its items, it owns the given item too now. 
	 *
	 * returns the item, that has been at this position before.
	 * or NULL, if there was none or the list owns items 
	 * (as in this case the list has deleted the old item).
	 */
	item_t *set(uint32 index, item_t *item);

// removing items

	/**
	 * extract() removes the item at position 'index' from the list and returns it. 
	 * The item is not deleted, even if the list owns its items. 
	 */
	item_t *extract(uint32 index);

	/**
	 * remove() removes the item at position 'index' from the list. 
	 * If the list owns its items, the item is deleted. 
	 */
	void remove(uint32 index) {	delete_item(extract(index)); }
};


// method implementations


// BG, 05.04.2010
template<class item_t> uint32 list_tpl<item_t>::add(item_t *item) 
{
	uint32 index;
	if (is_sorted) {
		index = search(item);
		insert(index, item);
	}
	else {
		index = append(item);
	}
	return index;
}


// BG, 04.04.2010
template<class item_t> uint32 list_tpl<item_t>::append(item_t *item) 
{
	if (count == capacity) {
		grow();
	}
	uint32 index = count++;
	if (!is_item_in_sort_order(index, item)) {
		set_is_sorted(false);
	}
	data[index] = item;
	return index;
}


// BG, 05.04.2010
template<class item_t> uint32 list_tpl<item_t>::bsearch(const item_t *item) const
{
	uint32 l = 0;
	uint32 r = count;
	while (l < r) {
		uint32 p = (l + r) / 2;
		if (compare_items(data[p], item) < 0) {
			l = p + 1;
		}
		else {
			r = p;
		}
	}
	return r;
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
template<class item_t> void list_tpl<item_t>::insert(uint32 index, item_t *item) 
{
	assert(index <= count);
	if (count == capacity) {
		grow();
	}
	for (uint32 i = count++; i > index; i--) {
		data[i] = data[i-1];
	}
	if (!is_item_in_sort_order(index, item)) {
		set_is_sorted(false);
	}
	data[index] = item;
}


// BG, 04.04.2010
template<class item_t> sint32 list_tpl<item_t>::index_of(const item_t *item) const 
{
	sint32 index = (sint32) count; 
	while (--index >= 0) {
		if (data[index] == item) {
			break;
		}
	}
	return index;
}


// BG, 05.04.2010
template<class item_t> bool list_tpl<item_t>::is_item_in_sort_order(uint32 index, const item_t *item)
{
	return get_is_sorted() && 
		(index == 0 || compare_items(data[index - 1], item) <= 0) && 
		(index + 1 == count || compare_items(item, data[index + 1]) <= 0); 
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
	else if (from > to) {
		while (--from >= to) {
			data[from+1] = data[from];
		}
	}
	else {
		// from == to: nothing to do
		return;
	}

	if (!is_item_in_sort_order(to, item)) {
		set_is_sorted(false);
	}
	data[to] = item;
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::qsort(sint32 l, sint32 r)
{
	sint32 i = l;
	while (i < r)	{
		sint32 j = r;
		sint32 p = (l + r) / 2;
		item_t *pivot = data[p];
		while (true)	{
			while (compare_items(data[i], pivot) < 0) 
				i++;
			while (compare_items(data[j], pivot) > 0)
				j--;
			if (i <= j)	{
				swap(i++, j--);
			};
			if (i > j) {
				break;
			}
		}
		if (l < j) {
			qsort(l, j);
		}
		l = i;
	}
}


// BG, 04.04.2010
template<class item_t> item_t *list_tpl<item_t>::set(uint32 index, item_t *item) 
{
	//assert(index < count);
	item_t *old = data[index];
	if (!is_item_in_sort_order(index, item)) {
		set_is_sorted(false);
	}
	data[index] = item;
	if (owns_items)
	{
		delete_item(old);
		old = NULL;
	}
	return old;
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::set_capacity(uint32 value) {
	assert(value <= (uint32)(UINT32_MAX_VALUE >> 1));
	if (capacity > value) {
		for (; capacity > value; ) {
			if (data[--capacity]) {
				delete_item(data[capacity]);
				data[capacity] = NULL;
			}
		}
		if (count > value) {
			count = value;
		}
		if (!capacity) {
			delete [] data;
			data = NULL;
		}
	}
	else if (capacity < value) {
		data = (item_t**) REALLOC(data, item_t*, value);
		for (; capacity < value; capacity++) {
			data[capacity] = NULL;
		}
	}
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::set_count(uint32 new_count) 
{
	set_capacity(new_count);
	if (count < new_count) {
		set_is_sorted(false);
		while (count < new_count) {
			data[count++] = create_item();
		}
	}
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::sort() 
{ 
	qsort(0, (sint32)count - 1);
	set_is_sorted(true);
}


// BG, 04.04.2010
template<class item_t> void list_tpl<item_t>::swap(uint32 from, uint32 to) 
{
	assert(from < count);
	assert(to < count);
	item_t *item = data[from];
	data[from] = data[to];
	data[to] = item;
}

#endif