include("hm_lib/hm_global")

class hm_rotate_building_tl extends hm_base_tl {
  pos = null
  constructor(p) {
    pos = coord3d(p[0],p[1],p[2])
    hm_commands.append(this)
  }
  
  function exec(player, origin) {
    local tl = command_x(tool_rotate_building)
    return tl.work(player, origin+pos)
  }
}

class hm_error_message_tl extends hm_base_tl {
  text = null
  constructor(s) {
    text = s
    hm_commands.append(this)
  }
  function exec(player, origin) {
    return text
  }
}

class hm_chat_message_tl extends hm_base_tl {
  text = null
  pos = null
  constructor(s, p) {
    text = s
    pos = coord3d(p[0], p[1], p[2])
    hm_commands.append(this)
  }
  function exec(player, origin) {
    gui.add_message_at(player, text, origin+pos);
    return null
  }
}

class hm_exec_func_tl extends hm_base_tl {
  func = null
  param = null
  constructor(f, p) {
    func = f
    param = p
    hm_commands.append(this)
  }
  function exec(player, origin) {
    return func(player, origin, param)
  }
}
