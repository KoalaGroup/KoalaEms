
VOID domist_1()
{
emit_s("das ist mist\n");
}

NORMAL_CODE(mist_1_entry, forth, "mist_1", domist_1)

VOID dos0()
{
spush((PTR8)s0, PTR8);
}

NORMAL_CODE(s0_entry, mist_1_entry, "s0", dos0)

VOID dosp()
{
spush((PTR8)sp, PTR8);
}

NORMAL_CODE(sp_entry, s0_entry, "sp", dosp)
