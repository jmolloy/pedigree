#include "BootstrapInfo.h"

extern "C" {
void _main(BootstrapStruct_t&);

extern void init_stacks();

int start(BootstrapStruct_t *bs) __attribute__((naked));

extern "C" int start(BootstrapStruct_t *bs)
{
    init_stacks();

    _main(*bs);

    return 0x13371337;
}

}
