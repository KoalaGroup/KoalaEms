/*Definitionen fuer vicvsb*/
/*08.09.92  M.Drochner*/

struct vic_opts
{
    char DTyp,
    LangeLeitung,
    NormalTyp,
    Platzhalter[125];
};

enum
{
    ss_vicvsbstat=0x80,
    ss_vicintstat,
    ss_getbase,
    ss_singleTransfer,
    ss_BLA,
    ss_CSR,
    ss_setSpace
};
