#include "arch-omap.h"

MachineOMAP850::MachineOMAP850()
{
    name = "Generic TI OMAP";
    archname = "OMAP";
}

int
MachineOMAP850::detect()
{
    // TODO - need to implement detection system.
    return 0;
}
