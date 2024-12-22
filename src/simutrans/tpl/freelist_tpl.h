/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef TPL_FREELIST_TPL_H
#define TPL_FREELIST_TPL_H

#include <typeinfo>

#include "../simmem.h"
#include "../simdebug.h"
#include "../simconst.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
#endif

// define USE_VALGRIND_MEMCHECK to make
// valgrind aware of the freelist memory pool
#ifdef USE_VALGRIND_MEMCHECK
#include <valgrind/memcheck.h>
#endif


/**
  * A template class for const sized memory pool
  * Must be a static member! Does not use exceptions
  */
class freelist_size_t
{
private:
	struct nodelist_node_t
	{
#ifdef DEBUG_FREELIST
		char canary[4];
#endif
		nodelist_node_t* next;
	};

	char canary_free[4] = "\xAA\x55\xAA";
	char canary_used[4] = "\x55\xAA\x55";

	size_t NODE_SIZE;
	size_t new_chuck_size;

	// next free node (or NULL)
	nodelist_node_t* freelist;

	// number of currently allocated node
	size_t nodecount;

	// list of all allocated memory
	nodelist_node_t* chunk_list;

#ifdef MULTI_THREAD
	pthread_mutex_t freelist_mutex = PTHREAD_MUTEX_INITIALIZER;;
#endif

public:
	freelist_size_t(size_t size) :
		freelist(0),
		nodecount(0),
		chunk_list(0)
	{
		NODE_SIZE = (size + sizeof(nodelist_node_t) - sizeof(nodelist_node_t*));
		new_chuck_size = ((32768 - sizeof(void*)) / NODE_SIZE);
		canary_free[3] = canary_used[3] = NODE_SIZE;
	}

	~freelist_size_t()
	{
		free_all_nodes();
	}

	void *gimme_node()
	{
#ifdef MULTI_THREAD
		pthread_mutex_lock(&freelist_mutex);
#endif
		nodelist_node_t *tmp;
		if (freelist == NULL) {
			char* p = (char*)xmalloc(new_chuck_size*NODE_SIZE + sizeof(nodelist_node_t));

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
			for (size_t i = 0; i < new_chuck_size; i++) {
				nodelist_node_t* tmp = (nodelist_node_t*)(p + i*NODE_SIZE);
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
				tmp->canary[3] = canary_free[3];
#endif
				tmp->next = freelist;
				freelist = tmp;
			}
		}

		// return first node of list
		tmp = freelist;
		freelist = tmp->next;

#ifdef DEBUG_FREELIST
		assert(tmp->canary[0] == canary_free[0] && tmp->canary[1] == canary_free[1] && tmp->canary[2] == canary_free[2] && tmp->canary[3] == canary_free[3]);
		tmp->canary[0] = canary_used[0];
		tmp->canary[1] = canary_used[1];
		tmp->canary[2] = canary_used[2];
#endif
		nodecount++;

#ifdef MULTI_THREAD
		pthread_mutex_unlock(&freelist_mutex);
#endif

#ifdef USE_VALGRIND_MEMCHECK
		// tell valgrind that we now have access to a chunk of size bytes
		VALGRIND_MEMPOOL_CHANGE(tmp, tmp, tmp, sizeof(T));
		VALGRIND_MAKE_MEM_UNDEFINED(tmp, sizeof(T));
#endif

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
		VALGRIND_MAKE_MEM_NOACCESS(p, sizeof(T));
		VALGRIND_MAKE_MEM_UNDEFINED(p, sizeof(nodelist_node_t));
#endif

#ifdef MULTI_THREAD
		pthread_mutex_unlock(&freelist_mutex);
#endif

		// putback to first node
		nodelist_node_t* tmp = (nodelist_node_t*)p;
#ifdef DEBUG_FREELIST
		size_t min_size = sizeof(nodelist_node_t) - sizeof(void*);
		tmp = (nodelist_node_t*)((char*)p - min_size);
		assert(tmp->canary[0] == canary_used[0] && tmp->canary[1] == canary_used[1] && tmp->canary[2] == canary_used[2] && tmp->canary[3] == canary_used[3]);
		tmp->canary[0] = canary_free[0];
		tmp->canary[1] = canary_free[1];
		tmp->canary[2] = canary_free[2];
#endif
		tmp->next = freelist;
		freelist = tmp;

		nodecount--;

		if (nodecount == 0) {
			free_all_nodes();
		}
#ifdef MULTI_THREAD
		pthread_mutex_unlock(&freelist_mutex);
#endif
	}

};



template<class T> class freelist_tpl
{
private:
	freelist_size_t fli;
public:
	freelist_tpl() : fli(sizeof(T)) {}
	T *gimme_node() { return (T *)fli.gimme_node(); }
	void putback_node(void* p) { return fli.putback_node(p); }
	void free_all_nodes() { fli.free_all_nodes(); }
};



#endif
