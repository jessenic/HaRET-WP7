/* Monitor WinCE exceptions.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <windows.h> // for pkfuncs.h
#include "pkfuncs.h" // AllocPhysMem
#include <time.h> // time
#include <string.h> // memcpy

#include "xtypes.h"
#include "watch.h" // memcheck
#include "output.h" // Output
#include "memory.h" // memPhysMap
#include "script.h" // REG_CMD
#include "machines.h" // Mach
#include "arch-pxa.h" // MachinePXA
#include "lateload.h" // LATE_LOAD
#include "winvectors.h" // findWinCEirq
#include "irq.h"

LATE_LOAD(AllocPhysMem, "coredll")
LATE_LOAD(FreePhysMem, "coredll")

/*
 * Theory of operation:
 *
 * This code allows one to obtain a log of ARM exceptions (interrupts,
 * memory faults, and code execution faults).  It also allows one to
 * catch exceptions (see the pxatrace.cpp and l1trace.cpp code).
 * Tracing exceptions is useful when trying to understand how wince
 * interacts with the hardware.
 *
 * The code works by allocating a continuous area of physical memory
 * which is populated with exception handling code and space for a
 * "trace buffer".  The ARM exception handling table is then modified
 * so that these new haret exception handlers are called instead of
 * the normal wince ones.
 *
 * The haret exception handlers basically log the event that occurred
 * (in the trace buffer) and then hand the exception over to wince for
 * normal processing.  (More advanced processing is also available -
 * haret can inspect memory on each fault and sometimes entirely
 * handle a fault.)
 *
 * The main haret process periodically checks the trace buffer for new
 * events.  Upon finding a new event, it reports it to the user.
 * (Note that the exception handlers can not directly report an event
 * to the user because they don't run in the haret "context" and they
 * can't access files, sockets, or even any library code.)
 *
 * Upon completion of the tracing session, the code returns the wince
 * exception table to its original state.
 */


/****************************************************************
 * C part of exception handlers
 ****************************************************************/

static void
report_memPoll(uint32 msecs, irqData *data, traceitem *item)
{
    watchListVar *w = (watchListVar*)item->d0;
    memcheck *mc = &w->watchlist[item->d1];
    uint32 clock=item->d2, val=item->d3, changed=item->d4;
    reportWatch(msecs, clock, mc, val, changed);
}

// Perform a set of memory polls and add to trace buffer.
int __irq
checkPolls(struct irqData *data, pollinfo *info, uint32 clock)
{
    int foundcount = 0;
    for (uint i=0; i<info->count; i++) {
        memcheck *mc = &info->list[i];
        uint32 val, changed;
        int ret = testMem(mc, &val, &changed);
        if (!ret)
            continue;
        foundcount++;
        ret = add_trace(data, report_memPoll, (uint32)info->cls, i
                        , clock, val, changed);
        if (ret)
            // Couldn't add trace - reset compare function.
            mc->trySuppress = 0;
    }
    return foundcount;
}

// Handler for interrupt events.  Note that this is running in
// "Modified Virtual Address" mode, so avoid reading any global
// variables or calling any non local functions.
extern "C" void __irq
irq_handler(struct irqData *data, struct irqregs *regs)
{
    data->irqCount++;
    if (data->isPXA) {
        // Separate routine for PXA chips
        PXA_irq_handler(data, regs);
        return;
    }

    // Irq time memory polling.
    checkPolls(data, &data->irqpoll);
    // Trace time memory polling.
    checkPolls(data, &data->tracepoll);
}

extern "C" int __irq
abort_handler(struct irqData *data, struct irqregs *regs)
{
    data->abortCount++;
    if (data->isPXA) {
        // Separate routine for PXA chips
        int ret = PXA_abort_handler(data, regs);
        if (ret)
            return ret;
    }
    return L1_abort_handler(data, regs);
}

extern "C" int __irq
prefetch_handler(struct irqData *data, struct irqregs *regs)
{
    data->prefetchCount++;
    if (data->isPXA) {
        // Separate routine for PXA chips
        int ret = PXA_prefetch_handler(data, regs);
        if (ret)
            return ret;
    }
    return L1_prefetch_handler(data, regs);
}

static void
report_resume(uint32 msecs, irqData *, traceitem *item)
{
    Output("%06d: WinCE resume", msecs);
}

extern "C" void __irq
resume_handler(struct irqData *data, struct irqregs *regs)
{
    add_trace(data, report_resume);
    checkPolls(data, &data->resumepoll);
    if (data->max_l1trace_after_resume)
        data->max_l1trace = data->max_l1trace_after_resume;
}


/****************************************************************
 * Code to report feedback from exception handlers
 ****************************************************************/

// Commands and variables are only applicable if AllocPhysMem is
// available.
int testWirqAvail() {
    return late_AllocPhysMem && late_FreePhysMem;
}

REG_VAR_WATCHLIST(0, "IRQS", IRQS,
                  "List of IRQs to watch (see var GPIOS for format)");
REG_VAR_WATCHLIST(testWirqAvail, "TRACES", TRACES,
                  "List of memory addresses to trace during wirq"
                  " (see var GPIOS for format)");
REG_VAR_WATCHLIST(testWirqAvail, "RESUMETRACES", RESUMETRACES,
                  "Physical memory addresses to check during a wince resume"
                  " (see var GPIOS)");

static uint32 LastOverflowReport;

// Pull a trace event from the trace buffer and print it out.  Returns
// 0 if nothing was available.
static int
printTrace(uint32 msecs, struct irqData *data)
{
    uint32 writePos = data->writePos;
    if (data->readPos == writePos)
        return 0;
    uint32 tmpoverflow = data->overflows;
    if (tmpoverflow != LastOverflowReport) {
        Output("overflowed %d traces"
               , tmpoverflow - LastOverflowReport);
        LastOverflowReport = tmpoverflow;
    }
    struct traceitem *cur = &data->traces[data->readPos % NR_TRACE];
    cur->reporter(msecs, data, cur);
    data->readPos++;
    return 1;
}

static void
prepPoll(pollinfo *info, watchListVar *var, int first=1)
{
    memcpy(info->list, var->watchlist, sizeof(info->list));
    info->count = min(var->watchcount, ARRAY_SIZE(info->list));
    info->cls = var;
    var->beginWatch(first);
}

// Called before exceptions are taken over.
static void
preLoop(struct irqData *data)
{
    LastOverflowReport = 0;

    // Setup memory tracing.
    prepPoll(&data->irqpoll, &IRQS, 1);
    prepPoll(&data->tracepoll, &TRACES, 0);
    prepPoll(&data->resumepoll, &RESUMETRACES, 0);
}

// Code called while exceptions are rerouted - should return after
// 'seconds' amount of time has passed.  This is called from normal
// process context, and the full range of input/output functions and
// variables are available.
static void
mainLoop(struct irqData *data, int seconds)
{
    uint32 start_time = GetTickCount();
    uint32 cur_time = start_time;
    uint32 fin_time = cur_time + seconds * 1000;
    int tmpcount = 0;
    for (;;) {
        int ret = printTrace(cur_time - start_time, data);
        if (ret) {
            // Processed a trace - try to process another without
            // sleeping.
            tmpcount++;
            if (tmpcount < 100)
                continue;
            // Hrmm.  Recheck the current time so that we don't run
            // away reporting traces.
        } else {
            // Nothing to report; yield the cpu.
            if (data->exitEarly)
                break;
            late_SleepTillTick();
        }
        cur_time = GetTickCount();
        tmpcount = 0;
        if (cur_time >= fin_time)
            break;
    }
}

// Called after exceptions are restored to wince - may be used to
// report additional data or cleanup structures.
static void
postLoop(struct irqData *data)
{
    // Flush trace buffer.
    for (;;) {
        int ret = printTrace(0, data);
        if (! ret)
            break;
    }
    Output("Handled %d irq, %d abort, %d prefetch, %d lost, %d errors"
           , data->irqCount, data->abortCount, data->prefetchCount
           , data->overflows, data->errors);
}


/****************************************************************
 * Binding of "chained" irq handler
 ****************************************************************/

// Layout of memory in physically continuous ram.
struct irqChainCode {
    // Stack for C prefetch code.
    char stack_prefetch[PAGE_SIZE];
    // Stack for C abort code.
    char stack_abort[PAGE_SIZE];
    // Stack for C irq code.
    char stack_irq[PAGE_SIZE];
    // Data for C code.
    struct irqData data;

    // Variable length array storing asm/C exception handler code.
    char cCode[1] PAGE_ALIGNED;
};

// Low level information to be passed to the assembler part of the
// chained exception handler.  DO NOT CHANGE HERE without also
// upgrading the assembler code.
struct irqAsmVars {
    // Modified Virtual Address of irqData data.
    uint32 dataMVA;
    // Standard WinCE interrupt handler.
    uint32 winceIrqHandler;
    // Standard WinCE abort handler.
    uint32 winceAbortHandler;
    // Standard WinCE prefetch handler.
    uint32 wincePrefetchHandler;
};

extern "C" {
    // Symbols added by linker.
    extern char irq_start;
    extern char irq_end;

    // Assembler linkage.
    extern char asmIrqVars;
    extern void irq_chained_handler();
    extern void abort_chained_handler();
    extern void prefetch_chained_handler();
}
#define offset_asmIrqVars() (&asmIrqVars - &irq_start)
#define offset_asmIrqHandler() ((char *)irq_chained_handler - &irq_start)
#define offset_asmAbortHandler() ((char *)abort_chained_handler - &irq_start)
#define offset_asmPrefetchHandler() ((char *)prefetch_chained_handler - &irq_start)
#define offset_cResumeHandler() ((char *)resume_handler - &irq_start)
#define size_cHandlers() (&irq_end - &irq_start)
#define size_handlerCode() (uint)(&((irqChainCode*)0)->cCode[size_cHandlers()])

// Main "watch irq" command entry point.
static void
cmd_wirq(const char *cmd, const char *args)
{
    uint32 seconds;
    if (!get_expression(&args, &seconds)) {
        ScriptError("Expected <seconds>");
        return;
    }

    // Locate position of wince exception handlers.
    uint32 *irq_loc = findWinCEirq(VADDR_IRQOFFSET);
    if (!irq_loc)
        return;
    uint32 *abort_loc = findWinCEirq(VADDR_ABORTOFFSET);
    if (!abort_loc)
        return;
    uint32 *prefetch_loc = findWinCEirq(VADDR_PREFETCHOFFSET);
    if (!prefetch_loc)
        return;
    uint32 newIrqHandler, newAbortHandler, newPrefetchHandler;

    // Allocate space for the irq handlers in physically continuous ram.
    void *rawCode = 0;
    ulong dummy;
    rawCode = late_AllocPhysMem(size_handlerCode()
                                , PAGE_EXECUTE_READWRITE, 0, 0, &dummy);
    irqChainCode *code = (irqChainCode *)cachedMVA(rawCode);
    int ret;
    struct irqData *data = &code->data;
    struct irqAsmVars *asmVars = (irqAsmVars*)&code->cCode[offset_asmIrqVars()];
    if (!rawCode) {
        Output(C_INFO "Can't allocate memory for irq code");
        goto abort;
    }
    if (!code) {
        Output(C_INFO "Can't find vm addr of alloc'd physical ram.");
        goto abort;
    }
    memset(code, 0, size_handlerCode());

    // Copy the C handlers to alloc'd space.
    memcpy(code->cCode, &irq_start, size_cHandlers());

    // Setup the assembler links
    asmVars->dataMVA = (uint32)data;
    asmVars->winceIrqHandler = *irq_loc;
    asmVars->winceAbortHandler = *abort_loc;
    asmVars->wincePrefetchHandler = *prefetch_loc;
    newIrqHandler = (uint32)&code->cCode[offset_asmIrqHandler()];
    newAbortHandler = (uint32)&code->cCode[offset_asmAbortHandler()];
    newPrefetchHandler = (uint32)&code->cCode[offset_asmPrefetchHandler()];

    Output("irq:%08x@%p=%08x abort:%08x@%p=%08x"
           " prefetch:%08x@%p=%08x"
           " data=%08x sizes=c:%08x,t:%08x"
           , asmVars->winceIrqHandler, irq_loc, newIrqHandler
           , asmVars->winceAbortHandler, abort_loc, newAbortHandler
           , asmVars->wincePrefetchHandler, prefetch_loc, newPrefetchHandler
           , asmVars->dataMVA
           , size_cHandlers(), size_handlerCode());

    preLoop(data);

    ret = prepPXAtraps(data);
    if (ret)
        goto abort;

    ret = prepL1traps(data);
    if (ret)
        goto abort;

    ret = hookResume(
        memVirtToPhys((uint32)&code->cCode[offset_cResumeHandler()])
        , memVirtToPhys((uint32)data)
        , memVirtToPhys((uint32)data)
        , data->resumepoll.count || data->max_l1trace_after_resume);
    if (ret)
        goto abort;

    // Replace old handler with new handler.
    Output("Replacing windows exception handlers...");
    take_control();
    startPXAtraps(data);
    startL1traps(data);
    Mach->flushCache();
    *irq_loc = newIrqHandler;
    *abort_loc = newAbortHandler;
    *prefetch_loc = newPrefetchHandler;
    return_control();
    Output("Finished installing exception handlers.");

    // Loop till time up.
    mainLoop(data, seconds);

    // Restore wince handler.
    Output("Restoring windows exception handlers...");
    take_control();
    stopPXAtraps(data);
    stopL1traps(data);
    Mach->flushCache();
    *irq_loc = asmVars->winceIrqHandler;
    *abort_loc = asmVars->winceAbortHandler;
    *prefetch_loc = asmVars->wincePrefetchHandler;
    return_control();
    Output("Finished restoring windows exception handlers.");

    unhookResume();

    postLoop(data);
abort:
    if (rawCode)
        late_FreePhysMem(rawCode);
}
REG_CMD(testWirqAvail, "WI|RQ", cmd_wirq,
        "WIRQ <seconds>\n"
        "  Watch which IRQ occurs for some period of time and report them.")
