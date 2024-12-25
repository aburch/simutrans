include("hm_lib/hm_global")

class hm_city_tl extends hm_base_tl {
  population = 0
  desc_name  = 0
  pos        = null
  rotation   = 0

  constructor(pop, dn, p, r) {
    population = pop
    desc_name  = dn
    pos        = coord3d(p[0],p[1],p[2])
    rotation   = r
    hm_commands.append(this)
  }

  function exec(player, origin) {
    return command_x(tool_add_city).work(player, pos+origin, format("%d,%s,%d", population, desc_name, rotation))
  }
}


class hm_city_set_population_tl extends hm_base_tl {
  pos         = null
  population  = 0

  constructor(pop, p) {
    pos        = coord3d(p[0],p[1],p[2])
    population = pop
    hm_commands.append(this)
  }

  function exec(player, origin) {
    //return command_x(tool_change_city_size).work(player, pos+origin, format("%d", population))
  }
}


class hm_set_owner_tl extends hm_base_tl {
  pos     = null
  ownerid = 0

  constructor(o, p) {
    pos     = coord3d(p[0],p[1],p[2])
    ownerid = o
    hm_commands.append(this)
  }

  function exec(player, origin) {
    local tp = origin+pos
    local tile = square_x(tp.x, tp.y).get_tile_at_height(tp.z)
    if(tile.has_ways()  &&  ownerid == 15) {
      // no way to convert to pavement roads right now
    }
    return null
  }
}
