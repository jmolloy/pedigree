#include <Module.h>
#include <Log.h>

extern "C" {
    void init_pcnet(int argc, char *argv[]);
};

/// Wrapper for the actual driver's code
void pedigree_init_pcnet()
{
    init_pcnet(0, 0);
}

void pedigree_destroy_pcnet()
{
    ERROR("No MODULE_EXIT function call that we can use for pcnet");
}

MODULE_NAME("pcnet");
MODULE_ENTRY(&pedigree_init_pcnet);
MODULE_EXIT(&pedigree_destroy_pcnet);
MODULE_DEPENDS("cdi");
