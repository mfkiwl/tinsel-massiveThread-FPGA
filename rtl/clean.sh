#!/bin/bash

rm -f *.cxx *.o *.h *.ba *.bo *.so *.ipinfo
rm -f InstrMem.hex DataMem.hex RunQueue.hex
rm -f testMem de5Top testMailbox testArrayOfQueue
rm -f de5Top.v mkCore.v mkDCache.v
rm -rf test-mem-log
rm -rf test-mailbox-log
rm -rf test-array-of-queue-log
