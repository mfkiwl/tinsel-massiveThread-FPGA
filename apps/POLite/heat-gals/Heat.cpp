#include "Heat.h"

#include <tinsel.h>
#include <POLite.h>

typedef PThread<
          HeatDevice,
          None,         // Accumulator (small state)
          HeatState,    // State
          None,         // Edge label
          HeatMessage   // Message
        > HeatThread;

int main()
{
  // Point thread structure at base of thread's heap
  HeatThread* thread = (HeatThread*) tinselHeapBaseSRAM();
  
  // Invoke interpreter
  thread->run();

  return 0;
}