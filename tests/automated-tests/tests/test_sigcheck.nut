function test_sigcheck()
{
    local signs = sign_desc_x.get_available_signs(wt_rail)
    foreach(idx, s in signs) {
        print("  SIGN " + s.get_name() + " pri=" + s.is_priority_signal() + " pre=" + s.is_pre_signal() + " lb=" + s.is_longblock_signal())
    }
    ASSERT_TRUE(true)
}
