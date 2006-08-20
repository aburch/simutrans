// since all blocks are derived from schiene_t, we can use this helper function
static schiene_t *get_block_way(const grund_t *gr)
{
	schiene_t *sch = dynamic_cast<schiene_t *>(gr->gib_weg_nr(0));
	if(!sch) {
		sch = dynamic_cast<schiene_t *>(gr->gib_weg_nr(1));
	}
//	if(!sch) DBG_MESSAGE("blockmanager::get_block_way()","no block at %i,%i,%i",gr->gib_pos().x,gr->gib_pos().y,gr->gib_pos().z);
	return sch;
}
