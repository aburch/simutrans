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


// this is to prepare an area with the correct slopes at curretn height
class hm_ground_tl extends hm_base_tl {
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
    local err = null
    if(!tile.is_ground()) {
      return format(translate("Tile %s is not a valid ground!"), coord3d_to_string(tp))
    }
    while(tile.z<tp.z  &&  err == null) {
      err = command_x.set_slope(player, tile, hm_slope.UP)
      tile = square_x(tp.x, tp.y).get_ground_tile()
    }
    while(tile.z>tp.z  &&  err == null) {
      err = command_x.set_slope(player, tile, hm_slope.DOWN)
      tile = square_x(tp.x, tp.y).get_ground_tile()
    }
    if(err) {
      return err
    }
    return command_x.set_slope(player, tp, slope)
  }
}

/**
 *
 *
 */
class hm_test_area_tl extends hm_base_tl {
  start     = null
  ziel      = null
  terraform = null

  constructor(s, z, t = null) {
    start     = coord3d(s[0],s[1],s[2])
    ziel      = coord3d(z[0],z[1],z[2])
    terraform = t
    hm_commands.append(this)
  }

  function exec(player, origin) {
    // ground check
    local s_tile = origin+start
    local e_tile = origin+ziel

    local count_x = abs(s_tile.x - e_tile.x) + 1
    local count_y = abs(s_tile.y - e_tile.y) + 1

    local check_z = false
    if ( terraform == 0 ) {
      check_z = true
    }

    //gui.add_message_at(player_x(1), "count_x " + count_x, world.get_time())
    //gui.add_message_at(player_x(1), "count_y " + count_y, world.get_time())

    local test_st = s_tile
    if ( s_tile.x >= e_tile.x && s_tile.y >= e_tile.y ) {
      test_st = e_tile
    }

    local err = null
    for ( local i = 0; i < count_x; i++ ) {
      local t = square_x(test_st.x+i, test_st.y).get_ground_tile()
      //gui.add_message_at(player, "tile  " + coord3d_to_string(t), world.get_time())
      for ( local j = 0; j < count_y; j++ ) {
        local tile = square_x(t.x, test_st.y+j).get_ground_tile()

        local tile_tree = tile.find_object(mo_tree)
        local tile_groundobj = tile.find_object(mo_groundobj)
        local tile_moving_object = tile.find_object(mo_moving_object)

        if ( tile.is_empty() ) {
          // tile is empthy
          //err = null
        } else if ( tile_tree != null || tile_groundobj != null || tile_moving_object != null ) {
          // tile have tree, groundobj or moving_obj
          // These are optical objects and do not prevent construction.
          //err = null
        } else {
          // The tile has objects that prevent construction.
          err = "not_free"
          gui.add_message_at(player_x(1), "tile is not free " + coord3d_to_string(tile), world.get_time())
          break
        }

        if ( check_z ) {
          gui.add_message_at(player_x(1), "tile terraform test " + coord3d_to_string(tile), world.get_time())
          if ( s_tile.z != tile.z || tile.get_slope() > 0) {
            err = "terraform"
            break
          }
        }

      }
      if ( err != null ) {
        break
      }
    }

    if ( err == "not_free" ) {
      // tile is not free
      local message = translate("Selected area not free for build!")
      return message
    } else if ( err == "terraform" ) {
      // terraform necessary
      local message = translate("The terrain is not suitable for construction.")
      return message
    } else {
      return null
    }
  }
}