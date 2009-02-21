#ifndef tpl_hashtable_tpl_h
#define tpl_hashtable_tpl_h

#include <stdio.h>
#include "slist_tpl.h"


#define STHT_BAGSIZE 101
#define STHT_BAG_COUNTER_T uint8

template<class key_t, class value_t, class hash_t>  class hashtable_iterator_tpl;


/*
 * Generic hashtable, which maps key_t to value_t. key_t depended functions
 * like the hash generation is implemented by the third template parameter
 * hash_t (see ifc/hash_tpl.h)
 */
template<class key_t, class value_t, class hash_t>
class hashtable_tpl : public hash_t
{
	friend class hashtable_iterator_tpl<key_t, value_t, hash_t>;

	struct node_t {
	public:
		key_t	key;
		value_t	object;

		int operator == (const node_t &x) const { return key == x.key; }
	};

	slist_tpl <node_t> bags[STHT_BAGSIZE];

	STHT_BAG_COUNTER_T get_hash(const key_t key) const
	{
		return (STHT_BAG_COUNTER_T)(hash_t::hash(key) % STHT_BAGSIZE);
	}

public:
	void clear()
	{
		for(STHT_BAG_COUNTER_T i=0; i<STHT_BAGSIZE; i++) {
			bags[i].clear();
		}
	}

	const value_t get(const key_t key) const
	{
		slist_iterator_tpl<node_t> iter(bags[get_hash(key)]);
		while(iter.next()) {
			node_t node = iter.get_current();

			if(comp(node.key, key) == 0) {
				return node.object;
			}
		}
		return value_t();
	}

	value_t *access(const key_t key)
	{
		slist_iterator_tpl<node_t> iter(bags[get_hash(key)]);
		while(iter.next()) {
			node_t &node = iter.access_current();

			if(comp(node.key, key) == 0) {
				return &node.object;
			}
		}
		return NULL;
	}

	//
	// Inserts a new value - failure, if key exists in table
	// V. Meyer
	//
	bool put(const key_t key, value_t object)
	{
		const STHT_BAG_COUNTER_T code = get_hash(key);
		slist_iterator_tpl<node_t> iter(bags[code]);

		//
		// Duplicate values are hard to debug, so better check here.
		// ->exception? V.Meyer
		//
		while(iter.next()) {
			node_t &node = iter.access_current();

			if(comp(node.key, key) == 0) {
				// duplicate
				return false;
			}
		}
		node_t node;

		node.key = key;
		node.object = object;
		bags[code].insert(node);
		return true;
	}

	//
	// Insert or replace a value - if a value is replaced, the old value is
	// returned, otherwise a nullvalue. This may be useful, if You need to delete it
	// afterwards.
	// V. Meyer
	//
	value_t set(const key_t key, value_t object)
	{
		const STHT_BAG_COUNTER_T code = get_hash(key);

		slist_iterator_tpl<node_t> iter(bags[code]);

		while(iter.next()) {
			node_t &node = iter.access_current();
			if(comp(node.key, key) == 0) {
				value_t value = node.object;
				node.object = object;
				return value;
			}
		}
		node_t node;

		node.key = key;
		node.object = object;
		bags[code].insert(node);

		return value_t();
	}

	//
	// Remove an entry - if the entry is not there, return a nullvalue
	// otherwise the value that was associated to the key.
	// V. Meyer
	//
	value_t remove(const key_t key)
	{
		const STHT_BAG_COUNTER_T code = get_hash(key);
		slist_iterator_tpl<node_t> iter(bags[code]);

		while(iter.next()) {
			node_t node = iter.get_current();
			if(comp(node.key, key) == 0) {
				bags[code].remove( node );

				return node.object;
			}
		}
		return value_t();
	}

	void dump_stats()
	{
		for(STHT_BAG_COUNTER_T i = 0; i < STHT_BAGSIZE; i++) {
			uint32 count = bags[i].get_count();

			printf("Bag %d contains %ud elements\n", i, count);

			slist_iterator_tpl<node_t> iter ( bags[i] );

			while(iter.next()) {
				node_t node = iter.get_current();
				printf(" ");
				hash_t::dump(node.key);
				printf("\n");
			}
		}
	}

	uint32 get_count() const
	{
		uint32 count = 0;
		for(STHT_BAG_COUNTER_T i = 0; i < STHT_BAGSIZE; i++) {
			count += bags[i].get_count();
		}
		return count;
	}

	bool empty() const
	{
		for (STHT_BAG_COUNTER_T i = 0; i < STHT_BAGSIZE; i++) {
			if (!bags[i].empty()) {
				return false;
			}
		}
		return true;
	}
};


/*
 * Generic iterator for hashtable
 */
template<class key_t, class value_t, class hash_t>
class hashtable_iterator_tpl {
	const slist_tpl < typename hashtable_tpl<key_t, value_t, hash_t>::node_t> *bags;
	slist_iterator_tpl < typename hashtable_tpl<key_t, value_t, hash_t>::node_t> bag_iter;

	STHT_BAG_COUNTER_T current_bag;
public:
	hashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, hash_t> *hashtable) :
			bag_iter(hashtable->bags)
	{
		bags = hashtable->bags;
		current_bag = 0;
	}

	hashtable_iterator_tpl(const hashtable_tpl<key_t, value_t, hash_t> &hashtable) :
		bag_iter(hashtable.bags)
	{
		bags = hashtable.bags;
		current_bag = 0;
	}

	void begin()
	{
		bag_iter = slist_iterator_tpl < typename hashtable_tpl<key_t, value_t, hash_t >::node_t>(bags);
		current_bag = 0;
	}

	bool next()
	{
		while(!bag_iter.next()) {
			if(++current_bag == STHT_BAGSIZE) {
				return false;
			}
	    bag_iter = slist_iterator_tpl < typename hashtable_tpl<key_t, value_t, hash_t>::node_t > (bags + current_bag);
		}
		return true;
	}

	const key_t & get_current_key() const
	{
		return bag_iter.get_current().key;
	}

	const value_t & get_current_value() const
	{
		return bag_iter.get_current().object;
	}

	value_t & access_current_value()
	{
		return bag_iter.access_current().object;
	}
};

#endif
