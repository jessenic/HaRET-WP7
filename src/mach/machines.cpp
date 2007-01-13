#include <windows.h> // SystemParametersInfo

#include "script.h" // REG_VAR_ROFUNC
#include "output.h" // Output
#include "machines.h"

// Global current machine setting.
class Machine *Mach;


/****************************************************************
 * Machine base class
 ****************************************************************/

Machine::Machine()
    : name("Default"), PlatformType(L"PocketPC")
    , machType(0)
{
    memset(OEMInfo, 0, sizeof(OEMInfo));
}

void
Machine::init()
{
}

int
Machine::preHardwareShutdown()
{
    return 0;
}

void
Machine::hardwareShutdown()
{
}

int
Machine::getBoardID()
{
    return -1;
}

const char *
Machine::getIrqName(uint)
{
    return "Unknown";
}

int
Machine::detect()
{
    return 0;
}

static Machine RefMachine;


/****************************************************************
 * Misc. commands
 ****************************************************************/

static uint32
cpuGetFamily(bool setval, uint32 *args, uint32 val)
{
    return (uint32)Mach->name;
}
REG_VAR_ROFUNC(0, "MACHNAME", cpuGetFamily, 0, "Autodetected machine name")

static uint32
machmtype(bool setval, uint32 *args, uint32 val)
{
    return Mach->machType;
}
REG_VAR_ROFUNC(0, "MACHMTYPE", machmtype, 0, "Autodetected machine mtype")


/****************************************************************
 * Machine auto-detection
 ****************************************************************/

// Symbols added by linker.
extern "C" {
    extern Machine *mach_start[];
    extern Machine *mach_end;
}

static Machine *
findMachineType()
{
    // Search for an exact match by SysParam stuff
    wchar_t oeminfo[128], platform[128];
    SystemParametersInfo(SPI_GETOEMINFO, sizeof(oeminfo), oeminfo, 0);
    SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(platform), platform, 0);
    Output("Trying to detect machine (Plat='%ls' OEM='%ls')"
           , platform, oeminfo);

    Machine **p = mach_start;
    while (p < &mach_end) {
        Machine *m = *p;
        p++;
        Output("Looking at machine %s", m->name);
        if (wcscmp(platform, m->PlatformType) != 0)
            continue;
        for (uint32 j=0; j<ARRAY_SIZE(m->OEMInfo) && m->OEMInfo[j]; j++) {
            int len = wcslen(m->OEMInfo[j]);
            if (wcsncmp(oeminfo, m->OEMInfo[j], len) == 0)
                // Match
                return m;
        }
    }

    // Couldn't find a machine - try by architecture.
    p = mach_start;
    while (p < &mach_end) {
        Machine *m = *p;
        p++;
        if (m->OEMInfo[0])
            // Not an architecture.
            continue;
        Output("Looking at arch %s", m->name);
        if (m->detect())
            // Match
            return m;
    }

    // Nothing matched - use generic default.
    return &RefMachine;
}

// Global detection mechanism
void
setupMachineType()
{
    if (Mach) {
        Output("Error: machine already defined to '%s'", Mach->name);
        return;
    }

    // Determine what the current machine type is.
    Output("Detecting current machine");
    Mach = findMachineType();
    Output("Initializing for machine '%s'", Mach->name);
    Mach->init();
}
