/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_FREELIST_TPL_H
#define TPL_FREELIST_TPL_H

struct nodelist_node_t
{
#ifdef DEBUG_FREELIST
	unsigned magic : 15;
	unsigned free : 1;
#endif
	nodelist_node_t* next;
};

#include <typeinfo>

#include "../simmem.h"

#ifdef MULTI_THREADx
#include "../utils/simthread.h"
#endif


/**
  * A template class for const sized memory pool
  * Must be a static member! Does not use exceptions
  */
template<class T> class freelist_tpl
{
private:
	// next free node (or NULL)
	nodelist_node_t* freelist;

	// number of currently allocated node
	size_t nodecount;

	// list of all allocated memory
	nodelist_node_t* chunk_list;

	const size_t new_chuck_size = (32768 - sizeof(void*)) / sizeof(T);

#ifdef MULTI_THREADx
	pthread_mutex_t freelist_mutex = PTHREAD_MUTEX_INITIALIZER;;
#endif

public:
	freelist_tpl() : freelist(0), nodecount(0), chunk_list(0) {}

	freelist_tpl(size_t ncs) : freelist(0), nodecount(0), chunk_list(0), new_chuck_size(ncs) {}

	void *gimme_node()
	{
#ifdef MULTI_THREADx
		pthread_mutex_lock(&freelist_mutex);
#endif
		nodelist_node_t *tmp;
		if (freelist == NULL) {
			int chunksize = 0x1000;
			char* p = (char*)xmalloc(chunksize *sizeof(T) + sizeof(nodelist_node_t));

#ifdef USE_VALGRIND_MEMCHECK
			// tell valgrind that we still cannot access the pool p
			VALGRIND_MAKE_MEM_NOACCESS(p, chunksize *sizeof(T) + sizeof(nodelist_node_t));
#endif
			// put the memory into the chunklist for free it
			nodelist_node_t* chunk = (nodelist_node_t *)p;

#ifdef USE_VALGRIND_MEMCHECK
			// tell valgrind that we reserved space for one nodelist_node_t
			VALGRIND_CREATE_MEMPOOL(chunk, 0, false);
			VALGRIND_MEMPOOL_ALLOC(chunk, chunk, sizeof(*chunk));
			VALGRIND_MAKE_MEM_UNDEFINED(chunk, sizeof(*chunk));
#endif
			chunk->next = chunk_list;
			chunk_list = chunk;
			p += sizeof(nodelist_node_t);
			// then enter nodes into nodelist
			for (int i = 0; i < chunksize; i++) {
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

	// clears all list memories
	void free_all_nodes()
	{
		while (chunk_list) {
			nodelist_node_t* p = chunk_list;
			chunk_list = chunk_list->next;

			// now release memory
#ifdef USE_VALGRIND_MEMCHECK
			VALGRIND_DESTROY_MEMPOOL(p);
#endif
			free(p);
		}
		freelist = 0;
		nodecount = 0;
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
