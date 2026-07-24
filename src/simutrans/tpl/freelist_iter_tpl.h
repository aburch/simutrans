/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_FREELIST_ITER_TPL_H
#define TPL_FREELIST_ITER_TPL_H


#include "../simmem.h"
#include "../simconst.h"
#include "../simtypes.h"

#include <typeinfo>

#ifdef MULTI_THREADx
#include "../utils/simthread.h"
#endif

// define USE_VALGRIND_MEMCHECK to make
// valgrind aware of the freelist memory pool
#ifdef USE_VALGRIND_MEMCHECK
#include <valgrind/memcheck.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#endif


// index of the lowest set bit; x must not be zero (portable across GCC/Clang/MSVC, 32/64 bit)
static inline uint32 freelist_ctz64(uint64 x)
{
#if defined(_MSC_VER)
	unsigned long idx;
#	if defined(_WIN64) || defined(_M_X64) || defined(_M_ARM64)
	_BitScanForward64(&idx, x);
	return (uint32)idx;
#	else
	if(  (uint32)x  ) {
		_BitScanForward(&idx, (uint32)x);
		return (uint32)idx;
	}
	_BitScanForward(&idx, (uint32)(x >> 32));
	return (uint32)idx + 32u;
#	endif
#elif defined(__clang__) || defined(__GCC__)
	return (uint32)__builtin_ctzll(x);
#else
	// just in case compatibility code
	if (x == 0) return 64;
	int n = 0;
	if (!(x & 0x0000FFFFFFFFFFFFULL)) { n += 48; x >>= 48; }
	if (!(x & 0x00000000FFFFFFFFULL)) { n += 32; x >>= 32; }
	if (!(x & 0x000000000000FFFFULL)) { n += 16; x >>= 16; }
	if (!(x & 0x00000000000000FFULL)) { n += 8;  x >>= 8; }
	if (!(x & 0x000000000000000FULL)) { n += 4;  x >>= 4; }
	if (!(x & 0x0000000000000003ULL)) { n += 2;  x >>= 2; }
	if (!(x & 0x0000000000000001ULL)) { n += 1; }
	return n;
#endif
}


/**
 * Fixed-size occupancy bitmap with a fast "next set bit" scan, so freelist
 * iteration can jump from one live slot to the next instead of testing every
 * (mostly empty) slot. Trivially zero-initialisable via memset (POD).
 */
template<size_t N> struct freelist_bitmap_t {
	static const size_t WORDS = (N + 63u) / 64u;
	uint64 words[WORDS];

	inline void set(size_t i) {
		words[i >> 6] |= ((uint64)1 << (i & 63));
	}

	inline void unset(size_t i) {
		words[i >> 6] &= ~((uint64)1 << (i & 63));
	}

	inline bool test(size_t i) const {
		return (words[i >> 6] >> (i & 63)) & 1;
	}

	// lowest set-bit index >= from, or N if none
	size_t first_from(size_t from) const {
		if(  from >= N  ) {
			return N;
		}
		size_t wi = from >> 6;
		uint64 bits = words[wi] & ((~(uint64)0) << (from & 63));
		while(  bits == 0  ) {
			if(  ++wi >= WORDS  ) {
				return N;
			}
			bits = words[wi];
		}
		// at this point, bits!=0 as ctz64 is undefined on 0
		const size_t idx = (wi << 6) + freelist_ctz64(bits);
		return idx < N ? idx : N;
	}
	size_t find_first() const {
		return first_from(0);
	}

	size_t find_next(size_t i) const {
		return first_from(i + 1);
	}
};


/**
  * A template class for const sized memory pool
  * Must be a static member! Does not use exceptions
  */
template<class T> class freelist_iter_tpl
{
private:
	struct nodelist_node_t
	{
#ifdef DEBUG_FREELIST
		char canary[4];
#endif
		nodelist_node_t* next;
	};

	const char *canary_free = "\xAA\x55\xAA";
	const char *canary_used = "\x55\xAA\x55";


#ifdef MULTI_THREADx
	pthread_mutex_t freelist_mutex = PTHREAD_MUTEX_INITIALIZER;;
#endif

	// next free node (or NULL)
	nodelist_node_t* freelist;

	// number of currently allocated node
	size_t nodecount;

	// we aim for near 32 kB chunks, hoping that the system will allocate them on each page
	// and they fit the L1 cache
	static constexpr size_t NODE_SIZE = (sizeof(T) + sizeof(nodelist_node_t)-sizeof(nodelist_node_t *));
	static constexpr size_t new_chuck_size = (32250*8) / (NODE_SIZE*8+1);

	struct chunklist_node_t {
		chunklist_node_t *chunk_next;
		// marking empty and allocated tiles for fast interation
		freelist_bitmap_t<new_chuck_size> allocated_mask;
	};

	// list of all allocated memory
	chunklist_node_t* chunk_list;

	void change_obj(char *p,bool b)
	{
		char *c_list = (char *)chunk_list;
		const size_t chunk_mem_size = sizeof(chunklist_node_t) + (NODE_SIZE * new_chuck_size);
		while (c_list && (p<c_list || c_list+chunk_mem_size <p)) {
			// not in this chunk => continue
			c_list = (char *)(reinterpret_cast<chunklist_node_t *>(c_list))->chunk_next;
		}
		// we have found us (or we crash on error)
		size_t index = ((p - c_list) - sizeof(chunklist_node_t)) / NODE_SIZE;
		assert(index < new_chuck_size);
		if (b) {
			(reinterpret_cast<chunklist_node_t*>(c_list))->allocated_mask.set(index);
		}
		else {
			(reinterpret_cast<chunklist_node_t*>(c_list))->allocated_mask.unset(index);
		}
	}

	// clears all list memories
	void free_all_nodes()
	{
		while (chunk_list) {
			chunklist_node_t* p = chunk_list;
			chunk_list = chunk_list->chunk_next;

			// now release memory
#ifdef USE_VALGRIND_MEMCHECK
			VALGRIND_DESTROY_MEMPOOL(p);
#endif
			free(p);
		}
		freelist = 0;
		chunk_list = 0;
		nodecount = 0;
	}

public:
	freelist_iter_tpl() : freelist(0), nodecount(0), chunk_list(0) {}

	void sync_step(uint32 delta_t)
	{
		chunklist_node_t* c_list = chunk_list;
		while (c_list) {
			char *p = ((char *)c_list)+sizeof(chunklist_node_t);
			// jump from set bit to set bit instead of scanning every (mostly empty) slot
			for (size_t i = c_list->allocated_mask.find_first(); i < new_chuck_size; i = c_list->allocated_mask.find_next(i)) {
				// is active object
				T *obj = (T *)&((reinterpret_cast<nodelist_node_t*>(p + (i * NODE_SIZE)))->next);
				if (sync_result result = obj->sync_step(delta_t)) {
					// remove from sync
					c_list->allocated_mask.unset(i);
					// and maybe delete
					if (result == SYNC_DELETE) {
						delete obj;
						if (nodecount == 0) {
							return; // since even the main chunk list became invalid
						}
					}
				}
			}
			c_list = c_list->chunk_next;
		}
	}

	// switch on off sync handling
	void add_sync(T* p) { change_obj((char*)p,true); };
	void remove_sync(T* p) { change_obj((char*)p,false); };

	size_t get_nodecout() const { return nodecount; }

	void *gimme_node()
	{
#ifdef MULTI_THREADx
		pthread_mutex_lock(&freelist_mutex);
#endif
		nodelist_node_t *tmp;
		if (freelist == NULL) {
			char* p = (char*)xmalloc(new_chuck_size*NODE_SIZE + sizeof(chunklist_node_t));
			memset(p, 0, sizeof(chunklist_node_t)); // clear allocation bits and next pointer

#ifdef USE_VALGRIND_MEMCHECK
			// tell valgrind that we still cannot access the pool p
			VALGRIND_MAKE_MEM_NOACCESS(p, new_chuck_size * sizeof(T) + sizeof(chunklist_node_t));
#endif
			// put the memory into the chunklist for free it
			chunklist_node_t* chunk = reinterpret_cast<chunklist_node_t *>(p);

#ifdef USE_VALGRIND_MEMCHECK
			// tell valgrind that we reserved space for one nodelist_node_t
			VALGRIND_CREATE_MEMPOOL(chunk, 0, false);
			VALGRIND_MEMPOOL_ALLOC(chunk, chunk, sizeof(*chunk));
			VALGRIND_MAKE_MEM_DEFINED(chunk, sizeof(*chunk));
#endif
			chunk->chunk_next = chunk_list;
			chunk_list = chunk;
			p += sizeof(chunklist_node_t);
			// then enter nodes into nodelist
			for (size_t i = 0; i < new_chuck_size; i++) {
				nodelist_node_t* tmp = reinterpret_cast<nodelist_node_t *>(p + i * NODE_SIZE);
#ifdef USE_VALGRIND_MEMCHECK
				// tell valgrind that we reserved space for one nodelist_node_t
				VALGRIND_CREATE_MEMPOOL(tmp, 0, false);
				VALGRIND_MEMPOOL_ALLOC(tmp, tmp, sizeof(*tmp));
				VALGRIND_MAKE_MEM_UNDEFINED(tmp, sizeof(*tmp));
#endif
#ifdef DEBUG_FREELIST
				tmp->canary[0] = canary_free[0];
				tmp->canary[1] = canary_free[1];
				tmp->canary[2] = canary_free[2];
				tmp->canary[3] = sizeof(T);
#endif
				tmp->next = freelist;
				freelist = tmp;
			}
		}

		// return first node of list
		tmp = freelist;
		freelist = tmp->next;

		change_obj((char *)(&(tmp->next)),true);

#ifdef MULTI_THREADx
		pthread_mutex_unlock(&freelist_mutex);
#endif

#ifdef USE_VALGRIND_MEMCHECK
		// tell valgrind that we now have access to a chunk of size bytes
		VALGRIND_MEMPOOL_CHANGE(tmp, tmp, tmp, sizeof(T));
		VALGRIND_MAKE_MEM_UNDEFINED(tmp, sizeof(T));
#endif

#ifdef DEBUG_FREELIST
		assert(tmp->canary[0] == canary_free[0] && tmp->canary[1] == canary_free[1] && tmp->canary[2] == canary_free[2] && tmp->canary[3] == sizeof(T));
		tmp->canary[0] = canary_used[0];
		tmp->canary[1] = canary_used[1];
		tmp->canary[2] = canary_used[2];
#endif
		nodecount++;

		return (void *)(&(tmp->next));
	}

	void putback_node(void* p)
	{
#ifdef USE_VALGRIND_MEMCHECK
		// tell valgrind that we keep access to a nodelist_node_t within the memory chunk
		VALGRIND_MEMPOOL_CHANGE(p, p, p, sizeof(nodelist_node_t));
		VALGRIND_MAKE_MEM_NOACCESS(p, sizeof(T));
		VALGRIND_MAKE_MEM_UNDEFINED(p, sizeof(nodelist_node_t));
#endif

#ifdef MULTI_THREADx
		pthread_mutex_unlock(&freelist_mutex);
#endif

		// putback to first node
		nodelist_node_t* tmp = (nodelist_node_t*)p;
#ifdef DEBUG_FREELIST
		size_t min_size = sizeof(nodelist_node_t) - sizeof(void*);
		tmp = (nodelist_node_t*)((char*)p - min_size);
		assert(tmp->canary[0] == canary_used[0] && tmp->canary[1] == canary_used[1] && tmp->canary[2] == canary_used[2] && tmp->canary[3] == sizeof(T));
		tmp->canary[0] = canary_free[0];
		tmp->canary[1] = canary_free[1];
		tmp->canary[2] = canary_free[2];
#endif

		tmp->next = freelist;
		freelist = tmp;

		change_obj((char *)p,0);
		nodecount--;

		if (nodecount == 0) {
			free_all_nodes();
		}

#ifdef MULTI_THREADx
		pthread_mutex_unlock(&freelist_mutex);
#endif
	}
};

#undef NODE_SIZE

#endif
