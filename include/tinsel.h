#ifndef _TINSEL_H_
#define _TINSEL_H_

#include <stdint.h>
#include <config.h>
#include <io.h>

// Control/status registers
#define CSR_INSTR_ADDR  "0x800"
#define CSR_INSTR       "0x801"
#define CSR_ALLOC       "0x802"
#define CSR_CAN_SEND    "0x803"
#define CSR_HART_ID     "0xf14"
#define CSR_CAN_RECV    "0x805"
#define CSR_SEND_LEN    "0x806"
#define CSR_SEND_PTR    "0x807"
#define CSR_SEND        "0x808"
#define CSR_RECV        "0x809"
#define CSR_WAIT_UNTIL  "0x80a"
#define CSR_FROM_UART   "0x80b"
#define CSR_TO_UART     "0x80c"
#define CSR_NEW_THREAD  "0x80d"
#define CSR_KILL_THREAD "0x80e"
#define CSR_EMIT        "0x80f"
#define CSR_CYCLE       "0xc00"
#define CSR_FLUSH       "0xc01"

// Get globally unique thread id of caller
inline int tinselId()
{
  int id;
  asm ("csrrw %0, " CSR_HART_ID ", zero" : "=r"(id));
  return id;
}

// Read cycle counter
inline uint32_t tinselCycleCount()
{
  uint32_t n;
  asm volatile ("csrrw %0, " CSR_CYCLE ", zero" : "=r"(n));
  return n;
}

// Flush cache line
inline void tinselFlushLine(uint32_t lineNum, uint32_t way)
{
  uint32_t arg = (lineNum << TinselDCacheLogNumWays) | way;
  asm volatile("csrrw zero, " CSR_FLUSH ", %0" : : "r"(arg));
}

// Write a word to instruction memory
inline void tinselWriteInstr(uint32_t addr, uint32_t word)
{
  asm volatile("csrrw zero, " CSR_INSTR_ADDR ", %0\n"
               "csrrw zero, " CSR_INSTR ", %1\n"
               : : "r"(addr >> 2), "r"(word));
}

// Emit word to console (simulation only)
inline void tinselEmit(uint32_t x)
{
  asm volatile("csrrw zero, " CSR_EMIT ", %0\n" : : "r"(x));
}

// Send byte to host (over DebugLink UART)
// (Returns non-zero on success)
inline uint32_t tinselUartTryPut(uint8_t x)
{
  uint32_t ret;
  asm volatile("csrrw %0, " CSR_TO_UART ", %1\n" : "=r"(ret) : "r"(x));
  return ret;
}

// Receive byte from host (over DebugLink UART)
// (Byte present in bits [7:0]; bit 8 indicates validity)
inline uint32_t tinselUartTryGet()
{
  uint32_t x;
  asm volatile("csrrw %0, " CSR_FROM_UART ", zero\n" : "=r"(x));
  return x;
}

// Insert new thread (with given id) into run queue
inline void tinselCreateThread(uint32_t id)
{
  asm volatile("csrrw zero, " CSR_NEW_THREAD ", %0\n" : : "r"(id));
}

// Do not insert currently running thread back in to run queue
inline void tinselKillThread()
{
  asm volatile("csrrw zero, " CSR_KILL_THREAD ", zero\n");
}

// Get pointer to message-aligned slot in mailbox scratchpad
inline volatile void* tinselSlot(int n)
{
  const volatile char* mb_scratchpad_base =
    (char*) (1 << (TinselLogBytesPerMsg + TinselLogMsgsPerThread));
  return (void*) (mb_scratchpad_base + (n << TinselLogBytesPerMsg));
}

// Give mailbox permission to use given address to store an message
inline void tinselAlloc(volatile void* addr)
{
  asm volatile("csrrw zero, " CSR_ALLOC ", %0" : : "r"(addr));
}

// Determine if calling thread can send a message
inline int tinselCanSend()
{
  int ok;
  asm volatile("csrrw %0, " CSR_CAN_SEND ", zero" : "=r"(ok));
  return ok;
}

// Determine if calling thread can receive a message
inline int tinselCanRecv()
{
  int ok;
  asm volatile("csrrw %0, " CSR_CAN_RECV ", zero" : "=r"(ok));
  return ok;
}

// Set message length for send operation
// (A message of length N is comprised of N+1 flits)
inline void tinselSetLen(int n)
{
  asm volatile("csrrw zero, " CSR_SEND_LEN ", %0" : : "r"(n));
}

// Send message at addr to dest
inline void tinselSend(int dest, volatile void* addr)
{
  asm volatile("csrrw zero, " CSR_SEND_PTR ", %0" : : "r"(addr));
  asm volatile("csrrw zero, " CSR_SEND ", %0" : : "r"(dest));
}

// Receive message
inline volatile void* tinselRecv()
{
  volatile void* ok;
  asm volatile("csrrw %0, " CSR_RECV ", zero" : "=r"(ok));
  return ok;
}

// Thread can be woken by a logical-OR of these events
typedef enum {TINSEL_CAN_SEND = 1, TINSEL_CAN_RECV = 2} TinselWakeupCond;

// Suspend thread until wakeup condition satisfied
inline void tinselWaitUntil(TinselWakeupCond cond)
{
  asm volatile("csrrw zero, " CSR_WAIT_UNTIL ", %0" : : "r"(cond));
}

#ifdef __cplusplus
inline TinselWakeupCond operator|(TinselWakeupCond a, TinselWakeupCond b)
{
  return (TinselWakeupCond) (((uint32_t) a) | ((uint32_t) b));
}
#endif

// Get globally unique thread id of host
// (Host board has X coordinate of 0 and Y coordinate on mesh rim)
inline uint32_t tinselHostId()
{
  return TinselMeshYLen << (TinselMeshXBits +
                              TinselLogCoresPerBoard +
                                TinselLogThreadsPerCore);
}

// Return pointer to base of thread's DRAM partition
inline void* tinselHeapBase()
{
  uint32_t me = tinselId();
  uint32_t partId = me & (TinselThreadsPerDRAM-1);
  uint32_t addr = TinselBytesPerDRAM -
                    ((partId+1) << TinselLogBytesPerDRAMPartition);
  return (void*) addr;
}

// Cache flush
inline void tinselCacheFlush()
{
  for (uint32_t i = 0; i < (1<<TinselDCacheLogSetsPerThread); i++)
    for (uint32_t j = 0; j < (1<<TinselDCacheLogNumWays); j++)
      tinselFlushLine(i, j);
  // Load from each off-chip RAM to ensure that flushes have fully propagated
  volatile uint8_t* base = tinselHeapBase();
  base[0];
  base[1 << TinselLogBytesPerLine];
}

#endif
