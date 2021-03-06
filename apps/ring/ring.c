// SPDX-License-Identifier: BSD-2-Clause
#include <tinsel.h>

#define RING_LENGTH 70
#define NUM_TOKENS  2
#define NUM_LOOPS   2

int main()
{
  // Get thread id
  int me = tinselId();

  // Get pointer to mailbox message slot
  volatile int* msgOut = tinselSendSlot();

  // Mapping from thread id to ring id
  uint32_t id = me;

  // Next thread in ring
  uint32_t next = id == (RING_LENGTH-1) ? 0 : id+1;

  // Number of tokens to send
  uint32_t toSend = 0;
  if (id == 0) toSend = NUM_TOKENS;

  // Number of tokens to receive before finishing
  uint32_t toRecv = NUM_LOOPS*NUM_TOKENS;

  while (1) {
    // Termination condition
    if (id == 0 && toRecv == 0 && tinselCanSend()) {
      int host = tinselHostId();
      tinselSend(host, msgOut);
    }
    // Receive
    if (tinselCanRecv()) {
      volatile int* msgIn = tinselRecv();
      tinselFree(msgIn);
      if (toRecv > 0) {
        toRecv--;
        toSend++;
      }
    }
    // Send
    if (toSend > 0 && tinselCanSend()) {
      tinselSend(next, msgOut);
      toSend--;
    }
  }

  return 0;
}

