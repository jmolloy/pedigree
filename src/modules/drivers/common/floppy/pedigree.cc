#include <Module.h>
#include <Log.h>

extern "C" {
    void init_floppy(int argc, char *argv[]);
};

/// Wrapper for the actual driver's code
void pedigree_init()
{
    init_floppy(0, 0);
}

void pedigree_destroy()
{
    ERROR("No MODULE_EXIT function call that we can use");
}

MODULE_NAME("floppy");
MODULE_ENTRY(&pedigree_init);
MODULE_EXIT(&pedigree_destroy);
MODULE_DEPENDS("cdi");
