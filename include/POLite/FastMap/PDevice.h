// SPDX-License-Identifier: BSD-2-Clause
#ifndef _PDEVICE_H_
#define _PDEVICE_H_

#include <stdint.h>
#include <stdlib.h>
#include <type_traits>

#ifdef TINSEL
  #include <tinsel.h>
  #define PTR(t) t*
#else
  #include <tinsel-interface.h>
  #define PTR(t) uint32_t
#endif

// Use this to align on half-cache-line boundary
#define ALIGNED __attribute__((aligned(1<<(TinselLogBytesPerLine-1))))

// This is a static limit on the number of pins per device
#ifndef POLITE_NUM_PINS
#define POLITE_NUM_PINS 1
#endif

// Macros for performance stats
//   POLITE_DUMP_STATS - dump performance stats on termination
//   POLITE_COUNT_MSGS - include message counts of performance stats

// Thread-local device id
typedef uint16_t PLocalDeviceId;

// Thread id
typedef uint32_t PThreadId;

// Device address
// Bits 17->0: thread id
// Bit 18: invalid address
// Bits 31->19: thread-local device id
typedef uint32_t PDeviceAddr;

// Device address constructors
inline PDeviceAddr invalidDeviceAddr() { return 0x40000; }
inline PDeviceAddr makeDeviceAddr(PThreadId t, PLocalDeviceId d) {
  return (d << 19) | t;
}

// Device address deconstructors
inline bool isValidDeviceAddr(PDeviceAddr addr) { return !(addr & 0x40000); }
inline PThreadId getThreadId(PDeviceAddr addr) { return addr & 0x3ffff; }
inline PLocalDeviceId getLocalDeviceId(PDeviceAddr addr) { return addr >> 19; }

// What's the max allowed local device address?
inline uint32_t maxLocalDeviceId() { return 8192; }

// Pins
//   No      - means 'not ready to send'
//   HostPin - means 'send to host'
//   Pin(n)  - means 'send to application pin number n'
typedef uint8_t PPin;
#define No 0
#define HostPin 1
#define Pin(n) ((n)+2)

// For template arguments that are not used
struct None {};

// Generic device structure
// Type parameters:
//   S - State
//   E - Edge label
//   M - Message structure
template <typename S, typename E, typename M> struct PDevice {
  // State
  S* s;
  PPin* readyToSend;
  uint32_t numVertices;
  uint16_t time;

  // Handlers
  void init();
  void send(volatile M* msg);
  void recv(M* msg, E* edge);
  bool step();
  bool finish(volatile M* msg);
};

// Generic device state structure
template <typename S> struct ALIGNED PState {
  // Board-level routing key for each outgoing pin
  uint32_t pin[POLITE_NUM_PINS];
  // Ready-to-send status
  PPin readyToSend;
  // Custom state
  S state;
};

// Message structure
template <typename M> struct PMessage {
  // Destination thread-local device id
  uint16_t devId;
  // Id of incoming edge
  uint16_t edgeId;
  // Application message
  M payload;
};

// An incoming edge to a device
template <typename E> struct PInEdge {
  E edge;
};

// Generic thread structure
template <typename DeviceType,
          typename S, typename E, typename M> struct PThread {

  // Number of devices handled by thread
  PLocalDeviceId numDevices;
  // Number of times step handler has been called
  uint16_t time;
  // Number of devices in graph
  uint32_t numVertices;
  // Pointer to array of device states
  PTR(PState<S>) devices;
  // Pointer to base of edge table
  PTR(PInEdge<E>) inTableBase;
  // Array of local device ids are ready to send
  PTR(PLocalDeviceId) senders;
  // This array is accessed in a LIFO manner
  PTR(PLocalDeviceId) sendersTop;

  // Count number of messages sent
  #ifdef POLITE_COUNT_MSGS
  // Total messages sent
  uint32_t msgsSent;
  // Total messages received
  uint32_t msgsReceived;
  // Number of times we wanted to send but couldn't
  uint32_t blockedSends;
  #endif

  #ifdef TINSEL

  // Helper function to construct a device
  INLINE DeviceType getDevice(uint32_t id) {
    DeviceType dev;
    dev.s           = &devices[id].state;
    dev.readyToSend = &devices[id].readyToSend;
    dev.numVertices = numVertices;
    dev.time        = time;
    return dev;
  }

  // Dump performance counter stats over UART
  void dumpStats() {
    tinselPerfCountStop();
    uint32_t me = tinselId();
    // Per-cache performance counters
    uint32_t cacheMask = (1 <<
      (TinselLogThreadsPerCore + TinselLogCoresPerDCache)) - 1;
    if ((me & cacheMask) == 0) {
      printf("H:%x,M:%x,W:%x\n",
        tinselHitCount(),
        tinselMissCount(),
        tinselWritebackCount());
    }
    // Per-core performance counters
    uint32_t coreMask = (1 << (TinselLogThreadsPerCore)) - 1;
    if ((me & coreMask) == 0) {
      printf("C:%x %x,I:%x %x\n",
        tinselCycleCountU(), tinselCycleCount(),
        tinselCPUIdleCountU(), tinselCPUIdleCount());
    }
    // Per-thread performance counters
    #ifdef POLITE_COUNT_MSGS
    uint32_t intraBoardId = me & ((1<<TinselLogThreadsPerBoard) - 1);
    uint32_t progRouterSent =
      intraBoardId == 0 ? tinselProgRouterSent() : 0;
    uint32_t progRouterSentInter =
      intraBoardId == 0 ? tinselProgRouterSentInterBoard() : 0;
    printf("MS:%x,MR:%x,PR:%x,PRI:%x,BL:%x\n",
      msgsSent, msgsReceived, progRouterSent,
        progRouterSentInter, blockedSends);
    #endif
  }

  // Invoke device handlers
  void run() {
    // Did last call to step handler request a new time step?
    bool active = true;

    // Reset performance counters
    tinselPerfCountReset();

    // Initialisation
    sendersTop = senders;
    for (uint32_t i = 0; i < numDevices; i++) {
      DeviceType dev = getDevice(i);
      // Invoke the initialiser for each device
      dev.init();
      // Device ready to send?
      if (*dev.readyToSend != No) {
        *(sendersTop++) = i;
      }
    }

    // Set number of flits per message
    tinselSetLen((sizeof(PMessage<M>)-1) >> TinselLogBytesPerFlit);

    // Event loop
    while (1) {
      // Try to send
      if (sendersTop != senders) {
        if (tinselCanSend()) {
          // Get next sender
          PLocalDeviceId src = *(--sendersTop);
          // Lookup device
          DeviceType dev = getDevice(src);
          PPin pin = *dev.readyToSend;
          // Invoke send handler
          PMessage<M>* m = (PMessage<M>*) tinselSendSlot();
          dev.send(&m->payload);
          // Reinsert sender, if it still wants to send
          if (*dev.readyToSend != No) sendersTop++;
          // Is it a send to the host pin or a user pin?
          if (pin == HostPin)
            tinselSend(tinselHostId(), m);
          else
            tinselKeySend(devices[src].pin[pin-2], m);
          #ifdef POLITE_COUNT_MSGS
            msgsSent++;
          #endif
        }
        else {
          #ifdef POLITE_COUNT_MSGS
            blockedSends++;
          #endif
          tinselWaitUntil(TINSEL_CAN_SEND|TINSEL_CAN_RECV);
        }
      }
      else {
        // Idle detection
        int idle = tinselIdle(!active);
        if (idle > 1)
          break;
        else if (idle) {
          active = false;
          for (uint32_t i = 0; i < numDevices; i++) {
            DeviceType dev = getDevice(i);
            // Invoke the step handler for each device
            active = dev.step() || active;
            // Device ready to send?
            if (*dev.readyToSend != No) {
              *(sendersTop++) = i;
            }
          }
          time++;
        }
      }

      // Step 2: try to receive
      while (tinselCanRecv()) {
        PMessage<M>* inMsg = (PMessage<M>*) tinselRecv();
        PInEdge<E>* inEdge = &inTableBase[inMsg->edgeId];
        // Lookup destination device
        PLocalDeviceId id = inMsg->devId;
        DeviceType dev = getDevice(id);
        // Was it ready to send?
        PPin oldReadyToSend = *dev.readyToSend;
        // Invoke receive handler
        dev.recv(&inMsg->payload, &inEdge->edge);
        // Insert device into a senders array, if not already there
        if (*dev.readyToSend != No && oldReadyToSend == No)
          *(sendersTop++) = id;
        #ifdef POLITE_COUNT_MSGS
          msgsReceived++;
        #endif
        tinselFree(inMsg);
      }
    }

    // Termination
    #ifdef POLITE_DUMP_STATS
      dumpStats();
    #endif

    // Invoke finish handler for each device
    for (uint32_t i = 0; i < numDevices; i++) {
      DeviceType dev = getDevice(i);
      tinselWaitUntil(TINSEL_CAN_SEND);
      PMessage<M>* m = (PMessage<M>*) tinselSendSlot();
      if (dev.finish(&m->payload)) tinselSend(tinselHostId(), m);
    }

    // Sleep
    tinselWaitUntil(TINSEL_CAN_RECV); while (1);
  }

  #endif

};

#endif