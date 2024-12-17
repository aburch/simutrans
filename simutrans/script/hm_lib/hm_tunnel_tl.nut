include("hm_lib/hm_global")
include("hm_lib/hm_functions")
include("hm_lib/hm_find_obj")
include("hm_lib/hm_obj_selector")

class hm_tunnel_tl extends hm_base_tl {
  desc_name = null
  start = null
  ziel = null
  
  constructor(d_name, s, z) {
    desc_name = d_name
    start = coord3d(s[0],s[1],s[2])
    ziel = coord3d(z[0],z[1],z[2])
    hm_commands.append(this)
  }
  
  function exec(player, origin) {
    local tool = command_x(tool_build_tunnel)
    local sp = origin+start
    local tile = tile_x(sp.x, sp.y, sp.z);
    local moWay = tile.find_object(mo_tunnel);
    local err = null
    if(!moWay) {
      // build tunnel entrance
      tool.set_flags(2)
      err = tool.work(player, sp, desc_name)
    }
    if(!err) {
      err = tool.work(player, origin+start, origin+ziel, desc_name)
    }
    
    if(err!=null) {
      //calc_route() failed to find a path.
      return "Tunnel building path from (" + (origin+start).tostring() + ") to (" + (origin+ziel).tostring() + ") is not found!"
    } else {
      return null
    }
  }
}
