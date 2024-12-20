include("hm_lib/hm_global")

class hm_headquarter_tl extends hm_base_tl {
  level     = 0
  pos       = null

  constructor(l, p, r) {
    level = l
    pos   = coord3d(p[0],p[1],p[2])
    // rotation ignored for now
    hm_commands.append(this)
  }

  function exec(player, origin) {
    local hq_builder = command_x(tool_headquarter)
    if(player.get_headquarter_level() <= level) {
      return hq_builder.work(player, pos+origin, null)
    }
    return format("Headquarter already at higher level %d versus %d", player.get_headquarter_level(), level)
  }
}


class hm_house_tl extends hm_base_tl {
  desc_name = 0
  pos       = null
  rotation  = 0

  constructor(dn, p, r) {
    desc_name = dn
    pos       = coord3d(p[0],p[1],p[2])
    rotation  = r
    hm_commands.append(this)
  }

  function exec(player, origin) {
    return command_x(tool_build_house).work(player, pos+origin, format("%d,%s", rotation, desc_name))
  }
}
