/**
 * Base file for AI players
 */


/**
 * initialization, called when AI player starts
 * @param pl player number of our AI player
 */
function start(pl)
{
}

/**
 * the heart-beat of the AI player,
 * called regularly, does not need to finish
 */
function step()
{
}

/**
 * initialization, called when savegame is loaded
 * default behavior: calls start()
 * @param pl player number of our AI player
 */
function resume_game(pl)
{
	start(pl)
}

/**
 * Happy New Month and Year!
 */
function new_month()
{
}
function new_year()
{
}
