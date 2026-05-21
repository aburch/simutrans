# Black-box e2e tests for the multiplayer wire protocol in
# src/simutrans/network/network_cmd*.cc.  Each test spawns a headless
# server, sends one hand-rolled packet, and asserts on the parsed
# reply.  Two purposes: pin reply shapes (NWC_AUTH_PLAYER, NWC_STEP
# heartbeat) so a refactor that drops or reorders a field breaks CI,
# and trip on the recent network security fixes that manifest on the
# wire as "server consumes the packet then crashes".
#
# Run (from repo root):
#   python3 -m unittest discover -s tools/nwc_protocol_test -t .
#   python3 -m unittest discover -s tools/nwc_protocol_test -t . -k null
#   python3 -m unittest tools.nwc_protocol_test.test_auth_player
#
# Requires a built simutrans binary, a pak64-compatible pakset under
# simutrans/pak/, and tests/empty-16x16.sve (the starter map leaves
# player slots 2..14 unfilled, which the NULL-slot test relies on).
