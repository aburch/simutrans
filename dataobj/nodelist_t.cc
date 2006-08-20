#include <stdlib.h>
#include <stdio.h>
#include "nodelist_t.h"

#include "../simmem.h"

#undef STATS


class nodelist_t::node_t
{
public:
  node_t * next;
};


class nodelist_t::list_stats_t
{
public:
  unsigned int allocs;
  unsigned int count;

  list_stats_t() {
    allocs = count = 0;
  }
};


#ifdef STATS
static void dump(const char * op, int size, nodelist_t::list_stats_t *s)
{
  printf("Op: %s\t Size: %d\tAllocs: %d\tCount: %d\n",
	 op, size, s->allocs, s->count);
}
#endif



/**
 * Creates a new list of nodes for the given size
 * @param size the node size
 * @author Hj. Malthaner
 */
nodelist_t::nodelist_t(unsigned int s,
		       unsigned int initial,
		       const char *user,
		       const char *comment)
{
  size = s;
  freelist = 0;
  stats = 0;

#ifdef STATS
  stats = new list_stats_t ();
#endif


  char * block = (char *)guarded_malloc(s*initial);

  for(int i = initial-1; i>=0; i--) {
    putback_node(block + (i*s));
  }

  printf("nodelist_t::nodelist_t() : user='%s' comment='%s' size=%d, initial=%d\n",
	 user, comment, size, initial);
}


/**
 * Gets a node from the freelist or allocates a new one
 * if the freelist is empty
 * @author Hj. Malthaner
 */
void * nodelist_t::gimme_node()
{
  if(freelist) {

#ifdef STATS
    stats->count --;
    dump("Use", size, stats);
#endif

    node_t * tmp = freelist;
    freelist = tmp->next;

    return tmp;
  } else {

#ifdef STATS
    stats->allocs ++;
    dump("New", size, stats);
#endif

    return guarded_malloc(size);
  }
}


/**
 * Puts a node back into the freelist for reuse later on
 *
 * @author Hj. Malthaner
 */
void nodelist_t::putback_node(void *tmp)
{

#ifdef STATS
  stats->count ++;
  dump("Put", size, stats);
#endif

  node_t *p = (node_t *)tmp;
  p->next = freelist;
  freelist = p;
}


/**
 * Puts a node list back into the freelist for reuse later on
 *
 * @author Hj. Malthaner
 */
void nodelist_t::putback_nodes(void *tmp)
{

#ifdef STATS
  stats->count ++;
  dump("Put", size, stats);
#endif

  node_t *p = (node_t *)tmp;
  // int i = 0;

  while(p->next) {
    p = p->next;
    // i++;
  }

  p->next = freelist;
  freelist = (node_t *)tmp;

  // printf("Handing back %d nodes of size %d\n", i, size);
}
