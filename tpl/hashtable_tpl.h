/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_HASHTABLE_TPL_H
#define TPL_HASHTABLE_TPL_H


#include "slist_tpl.h"
#include "../macros.h"

#define STHT_BAGSIZE 101
#define STHT_BAG_COUNTER_T uint8


/*
 * Generic hashtable, which maps key_t to value_t. key_t depended functions
 * like the hash generation is implemented by the third template parameter
 * hash_t (see ifc/hash_tpl.h)
 */
template<class key_t, class value_t, class hash_t>
class hashtable_tpl
{
protected:
	struct node_t {
	public:
		key_t   key;
		value_t value;

		int operator == (const node_t &x) const { return key == x.key; }
	};

	// the entires in the lists are sorted according to their keys
	slist_tpl <node_t> bags[STHT_BAGSIZE];
	uint32 count;

/*
 * assigning hashtables seems also not sound
 */
private:
	hashtable_tpl(const hashtable_tpl&);
	hashtable_tpl& operator=( hashtable_tpl const&);

public:
	hashtable_tpl() { count = 0; }

public:
	STHT_BAG_COUNTER_T get_hash(const key_t key) const
	{
		return (STHT_BAG_COUNTER_T)(hash_t::hash(key) % STHT_BAGSIZE);
	}

	class iterator
	{
		friend class hashtable_tpl;
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef node_t                    value_type;
		typedef ptrdiff_t                 difference_type;
		typedef node_t*                   pointer;
		typedef node_t&                   reference;

		iterator() : bag_i(), bag_end(), node_i() {}

		iterator(slist_tpl<node_t>* const bag_i,  slist_tpl<node_t>* const bag_end, typename slist_tpl<node_t>::iterator const& node_i) :
			bag_i(bag_i),
			bag_end(bag_end),
			node_i(node_i)
		{}

		pointer   operator ->() const { return &*node_i; }
		reference operator *()  const { return  *node_i; }

		iterator& operator ++()
		{
			if (++node_i == bag_i->end()) {
				for (;;) {
					if (++bag_i == bag_end) {
						node_i = typename slist_tpl<node_t>::iterator();
						break;
					}
					if (!bag_i->empty()) {
						node_i = bag_i->begin();
						break;
					}
				}
			}
			return *this;
		}

		bool operator ==(iterator const& o) const { return bag_i == o.bag_i && node_i == o.node_i; }
		bool operator !=(iterator const& o) const { return !(*this == o); }

	private:
		slist_tpl<node_t>*                   bag_i;
		slist_tpl<node_t>*                   bag_end;
		typename slist_tpl<node_t>::iterator node_i;
	};

	/* Erase element at pos
	 * pos is invalid after this method
	 * An iterator pointing to the successor of the erased element is returned */
	iterator erase(iterator old)
	{
		iterator pos(old);
		pos.bag_end = old.bag_end;
		pos.bag_i = old.bag_i;
		pos.node_i = old.bag_i->erase( old.node_i );
		if(  pos.node_i ==  pos.bag_i->end()  ) {
			for (;;) {
				if (++pos.bag_i == pos.bag_end) {
					pos.node_i = typename slist_tpl<node_t>::iterator();
					break;
				}
				if (!pos.bag_i->empty()) {
					pos.node_i = pos.bag_i->begin();
					break;
				}
			}
		}
		count --;
		return pos;
	}

	class const_iterator
	{
	public:
		typedef std::forward_iterator_tag iterator_category;
		typedef node_t                    value_type;
		typedef ptrdiff_t                 difference_type;
		typedef node_t const*             pointer;
		typedef node_t const&             reference;

		const_iterator() : bag_i(), bag_end(), node_i() {}

		const_iterator(slist_tpl<node_t> const* const bag_i,  slist_tpl<node_t> const* const bag_end, typename slist_tpl<node_t>::const_iterator const& node_i) :
			bag_i(bag_i),
			bag_end(bag_end),
			node_i(node_i)
		{}

		pointer   operator ->() const { return &*node_i; }
		reference operator *()  const { return  *node_i; }

		const_iterator& operator ++()
		{
			if (++node_i == bag_i->end()) {
				for (;;) {
					if (++bag_i == bag_end) {
						node_i = typename slist_tpl<node_t>::const_iterator();
						break;
					}
					if (!bag_i->empty()) {
						node_i = bag_i->begin();
						break;
					}
				}
			}
			return *this;
		}

		bool operator ==(const_iterator const& o) const { return bag_i == o.bag_i && node_i == o.node_i; }
		bool operator !=(const_iterator const& o) const { return !(*this == o); }

	private:
		slist_tpl<node_t> const*                   bag_i;
		slist_tpl<node_t> const*                   bag_end;
		typename slist_tpl<node_t>::const_iterator node_i;
	};

	iterator begin()
	{
		for (slist_tpl<node_t>* i = bags; i != endof(bags); ++i) {
			if (!i->empty()) {
				return iterator(i, endof(bags), i->begin());
			}
		}
		return end();
	}

	iterator end()
	{
		return iterator(endof(bags), endof(bags), typename slist_tpl<node_t>::iterator());
	}

	const_iterator begin() const
	{
		for (slist_tpl<node_t> const* i = bags; i != endof(bags); ++i) {
			if (!i->empty()) {
				return const_iterator(i, endof(bags), i->begin());
			}
		}
		return end();
	}

	const_iterator end() const
	{
		return const_iterator(endof(bags), endof(bags), typename slist_tpl<node_t>::iterator());
	}

	void clear()
	{
		for(STHT_BAG_COUNTER_T i=0; i<STHT_BAGSIZE; i++) {
			bags[i].clear();
		}
		count = 0;
	}

	// the elements are inserted with increasing key
	// => faster retrieval (we only have to check half of the lists)
	const value_t &get(const key_t key) const
	{
		static value_t nix;
		FORT(slist_tpl<node_t>, const& node, bags[get_hash(key)]) {
			typename hash_t::diff_type diff = hash_t::comp(node.key, key);
			if(  diff == 0  ) {
				return node.value;
			}
			if(  diff > 0  ) {
				// not contained
				break;
			}
		}
		return nix;
	}

	// the elements are inserted with increasing key
	// => faster retrieval, but never ever change a key later!!!
	value_t *access(const key_t key)
	{
		slist_tpl<node_t>& bag = bags[get_hash(key)];
		FORT(slist_tpl<node_t>, & node, bag) {
			typename hash_t::diff_type diff = hash_t::comp(node.key, key);
			if(  diff == 0  ) {
				return &node.value;
			}
			if(  diff > 0  ) {
				// not contained
				break;
			}
		}
		return NULL;
	}

	/// Inserts a new value - failure if key exists in table
	bool put(const key_t key, value_t object)
	{
		slist_tpl<node_t>& bag = bags[get_hash(key)];

		/* Duplicate values are hard to debug, so better check here.
		 * we also enter it sorted, saving lookout time for large lists ...
		 */
		for(  typename slist_tpl<node_t>::iterator iter = bag.begin(), end = bag.end();  iter != end;  ++iter  ) {
			typename hash_t::diff_type diff = hash_t::comp(iter->key, key);
			if(  diff>0  ) {
				node_t n;
				n.key   = key;
				n.value = object;
				bag.insert( iter, n );
				count ++;
				return true;
			}
			if(  diff == 0  ) {
				dbg->error( "hashtable_tpl::put", "Duplicate hash!" );
				return false;
			}
		}
		// here only for empty lists or everything was smaller
		node_t n;
		n.key = key;
		n.value = object;
		bag.append( n );
		count ++;
		return true;
	}

	//
	// Inserts a new instantiated value - failure, if key exists in table
	// mostly used with value_t = slist_tpl<F>
	//
	bool put(const key_t key)
	{
		slist_tpl<node_t>& bag = bags[get_hash(key)];

		/* Duplicate values are hard to debug, so better check here.
		 * we also enter it sorted, saving lookout time for large lists ...
		 */
		for(  typename slist_tpl<node_t>::iterator iter = bag.begin(), end = bag.end();  iter != end;  ++iter  ) {
			typename hash_t::diff_type diff = hash_t::comp(iter->key, key);
			if(  diff>0  ) {
				iter = bag.insert( iter );
				iter->key = key;
				count ++;
				return true;
			}
			if(  diff == 0  ) {
				// already initialized
				return false;
			}
		}
		// here only for empty lists or everything was smaller
		bag.append();
		bag.back().key = key;
		count ++;
		return true;
	}

	//
	// Insert or replace a value - if a value is replaced, the old value is
	// returned, otherwise a nullvalue. This may be useful if you need to delete it
	// afterwards.
	//
	value_t set(const key_t key, value_t object)
	{
		slist_tpl<node_t>& bag = bags[get_hash(key)];
		for(  typename slist_tpl<node_t>::iterator iter = bag.begin(), end = bag.end();  iter != end;  ++iter  ) {
			typename hash_t::diff_type diff = hash_t::comp(iter->key, key);
			if(  diff == 0  ) {
				value_t value = iter->value;
				iter->value = object;
				return value;
			}
			if(  diff > 0  ) {
				node_t node;
				node.key   = key;
				node.value = object;
				bag.insert( iter, node );
				count ++;
				return value_t();
			}
		}
		// empty list or really last one ...
		node_t node;
		node.key   = key;
		node.value = object;
		bag.append(node);
		count ++;
		return value_t();
	}

	// Remove an entry - if the entry is not there, return a nullvalue
	// otherwise the value that was associated to the key.
	value_t remove(const key_t key)
	{
		slist_tpl<node_t>& bag = bags[get_hash(key)];
		for(  typename slist_tpl<node_t>::iterator iter = bag.begin(), end = bag.end();  iter != end;  ++iter  ) {
			typename hash_t::diff_type diff = hash_t::comp(iter->key, key);
			if(  diff == 0  ) {
				value_t v = iter->value;
				bag.erase(iter);
				count --;
				return v;
			}
			if(  diff > 0  ) {
				// not in list
				break;
			}
		}
		return value_t();
	}

	value_t remove_first()
	{
		for(STHT_BAG_COUNTER_T i = 0; i < STHT_BAGSIZE; i++) {
			if(  !bags[i].empty()  ) {
				count --;
				return bags[i].remove_first().value;
			}
		}
		dbg->fatal( "hashtable_tpl::remove_first()", "Hashtable already empty!" );
		return value_t();
	}

	uint32 get_count() const
	{
		return count;
	}

	bool empty() const
	{
		return get_count()==0;
	}
};

#endif
