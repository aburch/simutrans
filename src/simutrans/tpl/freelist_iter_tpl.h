/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_FREELIST_ITER_TPL_H
#define TPL_FREELIST_ITER_TPL_H

#include <typeinfo>

#include "../simmem.h"
#include <bitset>

#ifdef MULTI_THREADx
#include "../utils/simthread.h"
#endif

#include "../obj/sync_steppable.h"

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
		unsigned magic : 15;
		unsigned free : 1;
#endif
		nodelist_node_t* next;
	};


#ifdef MULTI_THREADx
	pthread_mutex_t freelist_mutex = PTHREAD_MUTEX_INITIALIZER;;
#endif

	// next free node (or NULL)
	nodelist_node_t* freelist;

	// number of currently allocated node
	size_t nodecount;

	// we aim for near 32 kB chunks, hoping that the system will allocate them on each page
	// and they fit the L1 cache
	const size_t new_chuck_size = (32250*8) / (sizeof(T)*8+1);

	struct chunklist_node_t {
		chunklist_node_t *chunk_next;
		// marking empty and allocated tiles for fast interation
		std::bitset<(32250 * 8) / (sizeof(T) * 8 + 1)> allocated_mask;
	};

	// list of all allocated memory
	chunklist_node_t* chunk_list;

	void change_obj(char *p,bool b)
	{
		char *c_list = (char *)chunk_list;
		const size_t chunk_mem_size = sizeof(chunklist_node_t) + sizeof(T) * new_chuck_size;
		while (c_list && (p<c_list || c_list+chunk_mem_size <p)) {
			// not in this chunk => continue
			c_list = (char *)((chunklist_node_t *)c_list)->chunk_next;
		}
		// we have found us (or we crash on error)
		size_t index = ((p - c_list) - sizeof(chunklist_node_t)) / sizeof(T);
		assert(index < new_chuck_size);
		((chunklist_node_t*)c_list)->allocated_mask.set(index, b);
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
			T  *p = (T *)(((char *)c_list)+sizeof(chunklist_node_t));
			for (unsigned i = 0; i < new_chuck_size; i++) {
				if (c_list->allocated_mask.test(i)) {
					// is active object
					if (sync_result result = p[i].sync_step(delta_t)) {
						// remove from sync
						c_list->allocated_mask.set(i, false);
						// and maybe delete
						if (result == SYNC_DELETE) {
							delete (p+i);
							if (nodecount == 0) {
								return; // since even the main chunk list became invalid
							}
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
			char* p = (char*)xmalloc(new_chuck_size *sizeof(T)+sizeof(chunklist_node_t));
			memset(p, 0, sizeof(chunklist_node_t)); // clear allocation bits and next pointer

#ifdef USE_VALGRIND_MEMCHECK
			// tell valgrind that we still cannot access the pool p
			VALGRIND_MAKE_MEM_NOACCESS(p, new_chuck_size * sizeof(T) + sizeof(chunklist_node_t));
#endif
			// put the memory into the chunklist for free it
			chunklist_node_t* chunk = (chunklist_node_t *)p;

#ifdef USE_VALGRIND_MEMCHECK
			// tell valgrind that we reserved space for one nodelist_node_t
			VALGRIND_CREATE_MEMPOOL(chunk, 0, false);
			VALGRIND_MEMPOOL_ALLOC(chunk, chunk, sizeof(*chunk));
			VALGRIND_MAKE_MEM_UNDEFINED(chunk, sizeof(*chunk));
#endif
			chunk->chunk_next = chunk_list;
			chunk_list = chunk;
			p += sizeof(chunklist_node_t);
			// then enter nodes into nodelist
			for (size_t i = 0; i < new_chuck_size; i++) {
				nodelist_node_t* tmp = (nodelist_node_t*)(p + i * sizeof(T));
#ifdef USE_VALGRIND_MEMCHECK
				// tell valgrind that we reserved space for one nodelist_node_t
				VALGRIND_CREATE_MEMPOOL(tmp, 0, false);
				VALGRIND_MEMPOOL_ALLOC(tmp, tmp, sizeof(*tmp));
				VALGRIND_MAKE_MEM_UNDEFINED(tmp, sizeof(*tmp));
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
		tmp->magic = 0x5555;
		tmp->free = 0;
#endif
		nodecount++;

		return (void *)(&(tmp->next));
	}

	void putback_node(void* p)
	{
#ifdef USE_VALGRIND_MEMCHECK
		// tell valgrind that we keep access to a nodelist_node_t within the memory chunk
		VALGRIND_MEMPOOL_CHANGE(p, p, p, sizeof(nodelist_node_t));
		VALGRIND_MAKE_MEM_NOACCESS(p, size);
		VALGRIND_MAKE_MEM_UNDEFINED(p, sizeof(nodelist_node_t));
#endif

#ifdef MULTI_THREADx
		pthread_mutex_unlock(&freelist_mutex);
#endif

		// putback to first node
		nodelist_node_t* tmp = (nodelist_node_t*)p;
#ifdef DEBUG_FREELIST
		tmp = (nodelist_node_t*)((char*)p - min_size);
		assert(tmp->magic == 0x5555 && tmp->free == 0 && tmp->size == size / 4);
		tmp->free = 1;
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

#endif
