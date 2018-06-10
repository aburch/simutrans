// return codes from step
// enum return_code {
const RT_DONE_NOTHING    = 1	// Done nothing.
const RT_PARTIAL_SUCCESS = 2	// Done something, want to continue next
const RT_SUCCESS         = 4	// Done something.
const RT_ERROR           = 8	// Some error occured.
const RT_KILL_ME         = 16 // Can be destroyed by parent.
const RT_TOTAL_SUCCESS   = 20 // RT_SUCCESS | RT_KILL_ME
const RT_TOTAL_FAIL      = 24 // RT_ERROR | RT_KILL_ME
const RT_READY           = 29 // RT_DONE_NOTHING | RT_SUCCESS | RT_ERROR | RT_KILL_ME
// }


// basic return codes / data from steps
class r_t
{
	// return code composed of return_code constants above
	code = 0
	// reports have to be handled
	report = null
	// run node immediately
	node = null

	constructor(c, r=null, n=null) { code = c; report = r; node = n }

	function is_ready()       { return (code & RT_READY);}
	function can_be_deleted() { return (code & RT_KILL_ME); }
	function has_failed()      { return (code & RT_ERROR);}
}

// basic node
class node_t {
	name = null
	debug = false

	function step() { return r_t(RT_TOTAL_FAIL)}

	constructor(n = "node_t")
	{
		name = n
	}
	function dbgprint(t)
	{
		if (debug) print(name + ": " + t)
	}
	function _save()
	{
		return ::saveinstance(name, this)
	}

}

/**
 * Has array of nodes that will be executed / stepped sequentially.
 */
class node_seq_t extends node_t
{
	nodes = null
	next_to_step = 0

	constructor(n = "node_seq_t")
	{
		base.constructor(n)
		nodes = []
	}

	function append_child(c) {
		nodes.append(c)
	}

	/// @returns value of type r_t
	function step()
	{
		if (nodes.len() == 0) return r_t(RT_SUCCESS);

		if (next_to_step >= nodes.len()) {
			next_to_step = 0;
			return r_t(RT_DONE_NOTHING);
		}

		local ret = nodes[next_to_step].step()
		// take nodes report
		if (ret.report) {
			append_report(ret.report)
		}
		// delete child if ready
		if (ret.can_be_deleted()) {
			nodes.remove(next_to_step);
		}
		else if (ret.is_ready()) {
			next_to_step ++
		}
		// successor node
		if (ret.node) {
			append_child(ret.node)
		}
		// our return code
		local rc = RT_PARTIAL_SUCCESS // want next call, too
		if (ret.has_failed()) {
			rc = RT_ERROR // propagate error
		}
		else if (next_to_step > nodes.len()) {
			rc = RT_SUCCESS // done with stepping all nodes
		}
		return r_t(rc)
	}
}

class report_t
{
	// tree that will be executed
	action = null
	// costs
	cost_fix = 0
	cost_monthly = 0
	// expected gain per month
	gain_per_m = 0

	function merge_report(r)
	{
		action.append_child( r.action )
		cost_fix     += r.cost_fix
		cost_monthly += r.cost_monthly
		gain_per_m   += r.gain_per_m
	}

	function _save()
	{
		return ::saveinstance("report_t", this)
	}
}

class manager_t extends node_seq_t
{
	reports = null

	constructor(n = "manager_t")
	{
		base.constructor(n)
		reports = []
	}

	function step()
	{
		dbgprint("stepping a child")
		local r = base.step()
		if (r.code == RT_DONE_NOTHING  ||  r.code == RT_SUCCESS) {
			// all nodes were stepped
			dbgprint("doing some work")
			return work()
		}
		dbgprint("stepped a child")
		return r
	}

	function work()
	{
	}

	function append_report(r) { reports.append(r); }

	function get_report()
	{
		if (reports.len() == 0) return null
		// find report that maximizes gain_per_m / cost_fix
		local i = -1
		local best = null

		for(local j=0; j<reports.len(); j++) {
			local test = reports[j]
			// check account balance
			if (!is_cash_available(test.cost_fix)) {
				// too expensive
				continue
			}

			if ( best == null
				|| (best.gain_per_m * test.cost_fix < test.gain_per_m * best.cost_fix)
				|| (test.cost_fix == 0  &&  best.cost_fix == 0  &&   best.gain_per_m < test.gain_per_m) )
			{
				best = test
				i = j
			}
		}
		if (best) {
			reports.remove(i)
		}
		return best
	}
}
