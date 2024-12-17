include("hm_lib/hm_global")

class hm_slope_tl extends hm_base_tl {
  slope = 0
  pos = null
  constructor(slp, p) {
    slope = slp
    pos = coord3d(p[0],p[1],p[2])
    hm_commands.append(this)
  }
  function exec(player, origin) {
    // check if the designated pos refers a ground tile.
    local tp = origin+pos
    local tile = square_x(tp.x, tp.y).get_ground_tile()
    if(!tile.is_ground()) {
      return format(translate("Tile %s is not a valid ground!"), coord3d_to_string(tp))
    }
    if(tile.z!=tp.z) {
      if(slope==hm_slope.DOWN  &&  tile.z==tp.z-1) {
        // already a matching lower tile
        return;
      }
      if(slope==hm_slope.UP  &&  tile.z==tp.z+1) {
        // already a matching lower tile
        return;
      }
      return format(translate("Tile %s is not a valid ground!"), coord3d_to_string(tp))
    }
    return command_x.set_slope(player, tp, slope)
  }
}
