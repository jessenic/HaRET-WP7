#include "cpu.h" // DEF_GETCPR
#include "memory.h" // memPhysMap
#include "script.h" // runMemScript
#include "arch-pxa.h"
#define CONFIG_PXA27x
#include "pxa2xx.h" // pxaDMA

DEF_GETCPR(get_p15r0, 15, 0, c0, c0, 0)

MachinePXA27x::MachinePXA27x()
{
    name = "Generic Intel PXA27x";
    archname = "PXA27x";
    dcsr_count = 32;
}

int
MachinePXA27x::detect()
{
    uint32 p15r0 = get_p15r0();
    return ((p15r0 >> 24) == 'i'
            && ((p15r0 >> 13) & 0x7) == 2
            && ((p15r0 >> 4) & 0x3f) == 17);
}

void
MachinePXA27x::init()
{
    runMemScript("set ramaddr 0xa0000000\n"
                 // IRQs
                 "addlist IRQS p2v(0x40D00000) 0x480 32 0\n"
                 "addlist IRQS p2v(0x40D0009c) 0xfffffffc 32 0\n"
                 "addlist IRQS p2v(0x40E00048) 0 32 0\n"
                 "addlist IRQS p2v(0x40E0004c) 0 32 0\n"
                 "addlist IRQS p2v(0x40E00050) 0 32 0\n"
                 "addlist IRQS p2v(0x40E00148) 0 32 0\n"
                 // GPIO levels
                 "addlist GPIOS p2v(0x40E00000)\n"
                 "addlist GPIOS p2v(0x40E00004)\n"
                 "addlist GPIOS p2v(0x40E00008)\n"
                 "addlist GPIOS p2v(0x40E00100)\n"
                 // GPIO directions
                 "addlist GPIOS p2v(0x40E0000C)\n"
                 "addlist GPIOS p2v(0x40E00010)\n"
                 "addlist GPIOS p2v(0x40E00014)\n"
                 "addlist GPIOS p2v(0x40E0010C)\n");
}

int
MachinePXA27x::preHardwareShutdown()
{
    int ret = MachinePXA::preHardwareShutdown();
    if (ret)
        return ret;
    cken = (uint32 *)memPhysMap(CKEN);
    uhccoms = (uint32 *)memPhysMap(UHCCOMS);
    if (! cken || ! uhccoms)
        return -1;
    return 0;
}

// disable USB host.
static void
Reset27xUHC(volatile uint32 *uhccoms)
{
    // Reset usb host
    *uhccoms=1;
}

void
MachinePXA27x::hardwareShutdown()
{
    MachinePXA::hardwareShutdown();
    Reset27xUHC(uhccoms);

    *cken=(*cken)&(~(
//    CKEN5_STUART|
//    CKEN6_FFUART|
//    CKEN7_BTUART|
                       CKEN10_USBHOST|
                       CKEN11_USB|
                       CKEN12_MMC|
                       CKEN13_FICP|
                       CKEN19_KEYPAD|
                       CKEN23_SSP1|
                       CKEN24_CAMERA
                       ));
}

REGMACHINE(MachinePXA27x)
