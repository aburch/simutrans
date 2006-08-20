#ifndef nodelist_t_h
#define nodelist_t_h


/**
 * Helper class to organize small memory objects i.e. nodes for linked lists
 * and such.
 *
 * @author Hanjsjörg Malthaner
 */
class nodelist_t
{
 private:

  class node_t;
  class list_stats_t;

  unsigned int size;     // node size
  node_t * freelist;     // the list of free nodes
  list_stats_t * stats;  // statistics data

 public:

  /**
   * Creates a new list of nodes for the given size
   * @param size the node size
   * @param initial initial number of nodes in freelist
   * @param user a free-form string giving some information about
   *             the user of this list (debugging/logging mostly)
   * @author Hj. Malthaner
   */
  nodelist_t(unsigned int size,
	     unsigned int initial,
	     const char * user,
	     const char * comment);


  /**
   * Gets a node from the freelist or allocates a new one
   * if the freelist is empty
   * @author Hj. Malthaner
   */
  void * gimme_node();


  /**
   * Puts a node back into the freelist for reuse later on
   *
   * @author Hj. Malthaner
   */
  void putback_node(void *tmp);


  /**
   * Puts a node list back into the freelist for reuse later on
   *
   * @author Hj. Malthaner
   */
  void putback_nodes(void *tmp);

};

#endif // nodelist_t_h
