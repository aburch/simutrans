
include("hm_lib/hm_way_tl")
include("hm_lib/hm_wayobj_tl")
include("hm_lib/hm_bridge_tl")
include("hm_lib/hm_tunnel_tl")
include("hm_lib/hm_slope_tl")
include("hm_lib/hm_sign_tl")
include("hm_lib/hm_station_tl")
include("hm_lib/hm_depot_tl")
include("hm_lib/hm_remove_tl")
include("hm_lib/hm_find_obj")
include("hm_lib/hm_misc_tl")
include("hm_lib/hm_obj_selector")
include("hm_lib/hm_house_tl")
include("hm_lib/hm_city_tl")

function init(pl) {
  this.player = pl
  hm_build()
  return true
}

function work(pl, pos) {
  local os = hm_obj_selector()
  if(os.need_selection()) {
    return os.select_obj(pl, pos)
  }

  // obj selection eneded or is not needed.
  foreach (cmd in hm_commands) {
    local err = cmd.exec(pl, pos)
    if(err!=null && err.len()>0) {
      return err
    }
  }
  return null
}

function exit(pl)
{
  return true
}
