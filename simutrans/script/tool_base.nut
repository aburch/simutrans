// stubs for functions needed by scripted tools
function init(player)
{
	return true
}

function exit(player)
{
	return true
}
// one-click tools
function work(player, pos)
{
	return null
}

// two-click tools
function do_work(player, start, end)
{
	return null
}

function mark_tiles(player, start, end)
{
}

/**
 * Can the tool start/end on pos? If it is the second click, start is the position of the first click
 * 0 = no
 * 1 = This tool can work on this tile (with single click)
 * 2 = On this tile can dragging start/end
 * 3 = Both (1 and 2)
 *
 */
function is_valid_pos(player, start, pos)
{
	return 2
}

// dummy auxiliary function, will be regularly called
function step()
{
	// do not implement
}

// check whether all functions support the flags parameter
function correct_missing_flags_argument()
{
	if (work.getinfos().parameters.len() == 3) {
		work_old <- work
		work = function(player, pos, flags) { return work_old(player, pos) }
	}
	if (do_work.getinfos().parameters.len() == 4) {
		do_work_old <- do_work
		do_work = function(player, pos, flags) { return do_work_old(player, pos) }
	}
	if (mark_tiles.getinfos().parameters.len() == 4) {
		mark_tiles_old <- mark_tiles
		mark_tiles = function(player, pos, flags) { mark_tiles_old(player, pos) }
	}
}
